#include "batocera_bluetooth.h"
#include "app_log.h"

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BATOCERA_BLUETOOTH "/usr/bin/batocera-bluetooth"
#define BT_LISTING_FILE "/var/run/bt_listing"

static pid_t live_pid=-1;

static int mac_valid(const char *mac)
{
    if(!mac||strlen(mac)!=17)return 0;
    for(int i=0;i<17;i++){
        if(i%3==2){if(mac[i]!=':')return 0;}
        else if(!isxdigit((unsigned char)mac[i]))return 0;
    }
    return 1;
}

static int run_action(const char *action,const char *arg)
{
    pid_t pid=fork();
    if(pid<0)return -1;
    if(pid==0){
        if(arg)execl(BATOCERA_BLUETOOTH,BATOCERA_BLUETOOTH,action,arg,(char*)NULL);
        else execl(BATOCERA_BLUETOOTH,BATOCERA_BLUETOOTH,action,(char*)NULL);
        _exit(127);
    }
    int status=0;
    if(waitpid(pid,&status,0)<0)return -1;
    return WIFEXITED(status)&&WEXITSTATUS(status)==0?0:-1;
}

static int attr_value(const char *line,const char *key,char *out,size_t out_size)
{
    if(!line||!key||!out||out_size==0)return -1;
    char needle[64];
    snprintf(needle,sizeof(needle),"%s=\"",key);
    const char *p=strstr(line,needle);
    if(!p){out[0]='\0';return -1;}
    p+=strlen(needle);
    const char *end=strchr(p,'\"');
    if(!end){out[0]='\0';return -1;}
    size_t n=(size_t)(end-p);
    if(n>=out_size)n=out_size-1;
    memcpy(out,p,n);out[n]='\0';
    return 0;
}

static int device_index(BluetoothDevice *devices,int count,const char *mac)
{
    for(int i=0;i<count;i++)if(!strcasecmp(devices[i].mac,mac))return i;
    return -1;
}

int batocera_bluetooth_available(void)
{
    return access(BATOCERA_BLUETOOTH,X_OK)==0;
}

int batocera_bluetooth_enable(void){return run_action("enable",NULL);}
int batocera_bluetooth_disable(void){return run_action("disable",NULL);}
int batocera_bluetooth_connect(const char *mac){return mac_valid(mac)?run_action("connect",mac):-1;}
int batocera_bluetooth_disconnect(const char *mac){return mac_valid(mac)?run_action("disconnect",mac):-1;}
int batocera_bluetooth_remove(const char *mac){return mac_valid(mac)?run_action("remove",mac):-1;}
int batocera_bluetooth_pair(const char *mac){
    if(!mac_valid(mac))return -1;
    /* Batoceras trust speichert bei erfolgreichem Pairing bereits selbst.
       save aktualisiert anschliessend explizit das persistente Backup. */
    if(run_action("trust",mac)!=0)return -1;
    return run_action("save",NULL);
}

int batocera_bluetooth_list(BluetoothDevice *devices,int max_devices)
{
    if(!devices||max_devices<=0)return 0;
    FILE *fp=popen(BATOCERA_BLUETOOTH " list 2>/dev/null","r");
    if(!fp)return 0;
    char line[1024];int count=0;
    while(count<max_devices&&fgets(line,sizeof(line),fp)){
        char mac[18]="",name[128]="",connected[16]="";
        if(attr_value(line,"id",mac,sizeof(mac))!=0||!mac_valid(mac))continue;
        attr_value(line,"name",name,sizeof(name));
        attr_value(line,"connected",connected,sizeof(connected));
        snprintf(devices[count].mac,sizeof(devices[count].mac),"%s",mac);
        snprintf(devices[count].name,sizeof(devices[count].name),"%s",name[0]?name:mac);
        devices[count].connected=!strcasecmp(connected,"yes");
        count++;
    }
    pclose(fp);
    return count;
}

int batocera_bluetooth_start_live_devices(void)
{
    if(live_pid>0)return 0;
    unlink(BT_LISTING_FILE);
    pid_t pid=fork();
    if(pid<0)return -1;
    if(pid==0){
        int devnull=open("/dev/null",O_WRONLY);
        if(devnull>=0){
            dup2(devnull,STDOUT_FILENO);
            dup2(devnull,STDERR_FILENO);
            if(devnull>STDERR_FILENO)close(devnull);
        }
        execl(BATOCERA_BLUETOOTH,BATOCERA_BLUETOOTH,"start_live_devices",(char*)NULL);
        _exit(127);
    }
    live_pid=pid;
    app_logf("Bluetooth Batocera: Live-Suche gestartet");
    return 0;
}

void batocera_bluetooth_stop_live_devices(void)
{
    run_action("stop_live_devices",NULL);
    if(live_pid>0){
        for(int i=0;i<10;i++){
            int status=0;pid_t r=waitpid(live_pid,&status,WNOHANG);
            if(r==live_pid){live_pid=-1;break;}
            usleep(50000);
        }
        if(live_pid>0){
            kill(live_pid,SIGTERM);
            waitpid(live_pid,NULL,0);
            live_pid=-1;
        }
    }
    app_logf("Bluetooth Batocera: Live-Suche gestoppt");
}

int batocera_bluetooth_live_devices(BluetoothDevice *devices,int max_devices)
{
    if(!devices||max_devices<=0)return 0;
    FILE *fp=fopen(BT_LISTING_FILE,"r");
    if(!fp)return 0;
    int count=0;char line[1024];
    while(fgets(line,sizeof(line),fp)){
        char mac[18]="",name[128]="",status[16]="";
        if(attr_value(line,"id",mac,sizeof(mac))!=0||!mac_valid(mac))continue;
        attr_value(line,"name",name,sizeof(name));
        attr_value(line,"status",status,sizeof(status));
        int idx=device_index(devices,count,mac);
        if(!strcasecmp(status,"removed")){
            if(idx>=0){for(int i=idx;i<count-1;i++)devices[i]=devices[i+1];count--;}
            continue;
        }
        if(strcasecmp(status,"added"))continue;
        if(idx<0){
            if(count>=max_devices)continue;
            idx=count++;
        }
        snprintf(devices[idx].mac,sizeof(devices[idx].mac),"%s",mac);
        snprintf(devices[idx].name,sizeof(devices[idx].name),"%s",name[0]?name:mac);
        devices[idx].connected=0;
    }
    fclose(fp);
    return count;
}
