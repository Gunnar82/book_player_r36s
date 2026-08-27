#include "batocera_bluetooth.h"
#include "batocera_pair_agent.h"
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
#define PACTL "/usr/bin/pactl"

static pid_t live_pid=-1;
static int live_fd=-1;
static char live_input[4096];
static size_t live_input_len;
static BluetoothDevice live_cache[BT_MAX_DEVICES];
static int live_cache_count;
static BluetoothDevice saved_at_scan[BT_MAX_DEVICES];
static int saved_at_scan_count;

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

static int set_bluetooth_sink_volume_100(const char *mac)
{
    if(!mac_valid(mac)||access(PACTL,X_OK)!=0)return -1;

    char sink_mac[18];
    snprintf(sink_mac,sizeof(sink_mac),"%s",mac);
    for(char *p=sink_mac;*p;p++)if(*p==':')*p='_';

    for(int attempt=0;attempt<20;attempt++){
        FILE *fp=popen(PACTL " list short sinks 2>/dev/null","r");
        if(fp){
            char line[1024];
            while(fgets(line,sizeof(line),fp)){
                char *name=strchr(line,'\t');
                if(!name)continue;
                name++;
                char *end=strchr(name,'\t');
                if(!end)continue;
                *end='\0';
                if(strncmp(name,"bluez_output.",13)!=0||!strstr(name,sink_mac))continue;
                pclose(fp);

                pid_t pid=fork();
                if(pid<0)return -1;
                if(pid==0){execl(PACTL,PACTL,"set-sink-volume",name,"100%",(char*)NULL);_exit(127);}
                int status=0;
                if(waitpid(pid,&status,0)<0)return -1;
                if(WIFEXITED(status)&&WEXITSTATUS(status)==0){
                    app_logf("Bluetooth Batocera: Audio-Sink %s auf 100%% gesetzt",name);
                    return 0;
                }
                app_logf("Bluetooth Batocera: Audio-Sink %s konnte nicht auf 100%% gesetzt werden",name);
                return -1;
            }
            pclose(fp);
        }
        usleep(250000);
    }
    app_logf("Bluetooth Batocera: kein PipeWire-Sink fuer %s gefunden",mac);
    return -1;
}

static int connect_and_set_volume(const char *mac)
{
    if(!mac_valid(mac))return -1;
    int rc=run_action("connect",mac);
    if(rc==0)set_bluetooth_sink_volume_100(mac);
    return rc;
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
int batocera_bluetooth_connect(const char *mac){return connect_and_set_volume(mac);}
int batocera_bluetooth_disconnect(const char *mac){return mac_valid(mac)?run_action("disconnect",mac):-1;}
int batocera_bluetooth_remove(const char *mac){return mac_valid(mac)?run_action("remove",mac):-1;}
int batocera_bluetooth_pair(const char *mac)
{
    if(!mac_valid(mac))return -1;

    /* Batoceras bluetooth-agent kann bestimmte unpaired/connected-Zustaende
       nicht sauber behandeln und ist ausserdem als NoInputNoOutput registriert.
       Fuer die eigentliche Authentifizierung verwenden wir daher einen kleinen,
       nur waehrend dieses Pairings registrierten DisplayYesNo-Agenten. Alle
       persistenten Batocera-Schritte bleiben beim Systemkommando. */
    if(batocera_pair_agent_pair(mac)!=0)return -1;
    if(run_action("save",NULL)!=0)return -1;
    return connect_and_set_volume(mac);
}

int batocera_bluetooth_list(BluetoothDevice *devices,int max_devices)
{
    if(!devices||max_devices<=0)return 0;
    FILE *fp=popen(BATOCERA_BLUETOOTH " list 2>/dev/null","r");
    if(!fp)return 0;
    char line[1024];int count=0;
    while(count<max_devices&&fgets(line,sizeof(line),fp)){
        char mac[18]="",name[128]="",connected[16]="",type[16]="";
        if(attr_value(line,"id",mac,sizeof(mac))!=0||!mac_valid(mac))continue;
        attr_value(line,"name",name,sizeof(name));
        attr_value(line,"connected",connected,sizeof(connected));
        attr_value(line,"type",type,sizeof(type));
        snprintf(devices[count].mac,sizeof(devices[count].mac),"%s",mac);
        snprintf(devices[count].name,sizeof(devices[count].name),"%s",name[0]?name:mac);
        snprintf(devices[count].type,sizeof(devices[count].type),"%s",type[0]?type:"unknown");
        devices[count].connected=!strcasecmp(connected,"yes");
        count++;
    }
    pclose(fp);
    return count;
}

static void process_live_line(const char *line)
{
    char mac[18]="",name[128]="",status[16]="",type[16]="";
    if(attr_value(line,"id",mac,sizeof(mac))!=0||!mac_valid(mac))return;
    attr_value(line,"name",name,sizeof(name));
    attr_value(line,"status",status,sizeof(status));
    attr_value(line,"type",type,sizeof(type));

    int idx=device_index(live_cache,live_cache_count,mac);
    if(!strcasecmp(status,"removed")){
        if(idx>=0){
            for(int i=idx;i<live_cache_count-1;i++)live_cache[i]=live_cache[i+1];
            live_cache_count--;
        }
        return;
    }
    if(strcasecmp(status,"added"))return;
    if(device_index(saved_at_scan,saved_at_scan_count,mac)>=0)return;

    if(idx<0){
        if(live_cache_count>=BT_MAX_DEVICES)return;
        idx=live_cache_count++;
    }
    snprintf(live_cache[idx].mac,sizeof(live_cache[idx].mac),"%s",mac);
    snprintf(live_cache[idx].name,sizeof(live_cache[idx].name),"%s",
             (name[0]&&strcasecmp(name,"None"))?name:mac);
    snprintf(live_cache[idx].type,sizeof(live_cache[idx].type),"%s",type[0]?type:"unknown");
    live_cache[idx].connected=0;
}

static void pump_live_output(void)
{
    if(live_fd<0)return;
    char buf[1024];
    for(;;){
        ssize_t n=read(live_fd,buf,sizeof(buf));
        if(n>0){
            for(ssize_t i=0;i<n;i++){
                char ch=buf[i];
                if(ch=='\n'){
                    live_input[live_input_len]='\0';
                    process_live_line(live_input);
                    live_input_len=0;
                }else if(ch!='\r'){
                    if(live_input_len+1<sizeof(live_input))live_input[live_input_len++]=ch;
                    else live_input_len=0;
                }
            }
            continue;
        }
        break;
    }
}

int batocera_bluetooth_start_live_devices(void)
{
    if(live_pid>0)return 0;
    saved_at_scan_count=batocera_bluetooth_list(saved_at_scan,BT_MAX_DEVICES);

    int pipefd[2];
    if(pipe(pipefd)!=0)return -1;
    pid_t pid=fork();
    if(pid<0){close(pipefd[0]);close(pipefd[1]);return -1;}
    if(pid==0){
        close(pipefd[0]);
        dup2(pipefd[1],STDOUT_FILENO);
        int devnull=open("/dev/null",O_WRONLY);
        if(devnull>=0){dup2(devnull,STDERR_FILENO);if(devnull>STDERR_FILENO)close(devnull);}
        if(pipefd[1]!=STDOUT_FILENO)close(pipefd[1]);
        execl(BATOCERA_BLUETOOTH,BATOCERA_BLUETOOTH,"start_live_devices",(char*)NULL);
        _exit(127);
    }

    close(pipefd[1]);
    int flags=fcntl(pipefd[0],F_GETFL,0);
    if(flags>=0)fcntl(pipefd[0],F_SETFL,flags|O_NONBLOCK);
    live_fd=pipefd[0];
    live_pid=pid;
    live_input_len=0;
    live_cache_count=0;
    app_logf("Bluetooth Batocera: Live-Suche gestartet (pid=%ld, %d gespeicherte ausgeblendet)",
             (long)live_pid,saved_at_scan_count);
    return 0;
}

void batocera_bluetooth_stop_live_devices(void)
{
    if(live_pid<=0&&live_fd<0)return;
    run_action("stop_live_devices",NULL);
    if(live_pid>0){
        for(int i=0;i<20;i++){
            int status=0;
            pid_t r=waitpid(live_pid,&status,WNOHANG);
            if(r==live_pid){live_pid=-1;break;}
            if(r<0){live_pid=-1;break;}
            usleep(50000);
        }
        if(live_pid>0){
            app_logf("Bluetooth Batocera: stop_live_devices hat Prozess nicht beendet, SIGTERM");
            kill(live_pid,SIGTERM);
            waitpid(live_pid,NULL,0);
            live_pid=-1;
        }
    }
    if(live_fd>=0){close(live_fd);live_fd=-1;}
    live_input_len=0;
    live_cache_count=0;
    saved_at_scan_count=0;
    app_logf("Bluetooth Batocera: Live-Suche gestoppt");
}

int batocera_bluetooth_live_devices(BluetoothDevice *devices,int max_devices)
{
    if(!devices||max_devices<=0)return 0;
    pump_live_output();
    int count=live_cache_count;
    if(count>max_devices)count=max_devices;
    for(int i=0;i<count;i++)devices[i]=live_cache[i];
    return count;
}
