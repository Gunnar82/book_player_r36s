#include "bluetooth.h"
#include "storage.h"
#include "app_log.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <systemd/sd-bus.h>

int bluetooth_autoconnect=0;
char bluetooth_device_mac[18]="";

int bluetooth_adapter_present(void)
{
    return access("/sys/class/bluetooth/hci0", F_OK) == 0 ||
           access("/sys/class/bluetooth/hci1", F_OK) == 0;
}

static int bluetooth_mac_valid(const char *mac)
{
    if(!mac||strlen(mac)!=17)return 0;
    for(int i=0;i<17;i++){
        if(i%3==2){if(mac[i]!=':')return 0;}
        else if(!isxdigit((unsigned char)mac[i]))return 0;
    }
    return 1;
}

int bluetooth_service_available(void)
{
    sd_bus *bus=NULL;
    sd_bus_creds *creds=NULL;

    if(sd_bus_open_system(&bus)<0)
        return 0;

    int r=sd_bus_get_name_creds(bus,"org.bluez",0,&creds);

    sd_bus_creds_unref(creds);
    sd_bus_unref(bus);

    return r>=0;
}

int bluetooth_adapter_powered(void)
{
    if(!bluetooth_adapter_present())return 0;
    if(!bluetooth_service_available())return 0;

    sd_bus *bus=NULL;
    if(sd_bus_open_system(&bus)<0)return 0;

    int powered=0;
    const char *paths[]={"/org/bluez/hci0","/org/bluez/hci1"};
    for(size_t i=0;i<sizeof(paths)/sizeof(paths[0]);i++){
        int value=0;
        int r=sd_bus_get_property_trivial(
            bus,
            "org.bluez",
            paths[i],
            "org.bluez.Adapter1",
            "Powered",
            NULL,
            'b',
            &value
        );
        if(r>=0&&value){
            powered=1;
            break;
        }
    }

    sd_bus_unref(bus);
    return powered;
}

void bluetooth_load_config(void)
{
    bluetooth_autoconnect=0;bluetooth_device_mac[0]='\0';
    FILE *fp=fopen(get_storage_config_path(),"r");
    if(!fp)return;
    char line[1200];int in_section=0;
    while(fgets(line,sizeof(line),fp)){
        util_trim(line);
        if(!line[0]||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){in_section=!strcmp(line,"[bluetooth]");continue;}
        if(!in_section)continue;
        char *eq=strchr(line,'=');if(!eq)continue;*eq++='\0';util_trim(line);util_trim(eq);
        if(!strcmp(line,"autoconnect"))bluetooth_autoconnect=atoi(eq)?1:0;
        else if(!strcmp(line,"device")){
            if(!eq[0]||bluetooth_mac_valid(eq))snprintf(bluetooth_device_mac,sizeof(bluetooth_device_mac),"%s",eq);
            else app_logf("Bluetooth: ungueltige MAC in config.ini ignoriert");
        }
    }
    fclose(fp);
}

int bluetooth_save_config(void)
{
    const char *path=get_storage_config_path();
    FILE *fp=fopen(path,"r");
    char **lines=NULL;size_t count=0,cap=0;char line[1200];
    if(fp){while(fgets(line,sizeof(line),fp)){if(count==cap){size_t nc=cap?cap*2:32;char **t=realloc(lines,nc*sizeof(*t));if(!t){fclose(fp);goto fail;}lines=t;cap=nc;}lines[count]=strdup(line);if(!lines[count]){fclose(fp);goto fail;}count++;}fclose(fp);}
    fp=fopen(path,"w");if(!fp)goto fail;
    int in=0,have=0,wrote_auto=0,wrote_dev=0;
    for(size_t i=0;i<count;i++){
        char check[1200];snprintf(check,sizeof(check),"%s",lines[i]);util_trim(check);
        if(check[0]=='['){
            if(in){if(!wrote_auto)fprintf(fp,"autoconnect=%d\n",bluetooth_autoconnect);if(!wrote_dev)fprintf(fp,"device=%s\n",bluetooth_device_mac);}
            in=!strcmp(check,"[bluetooth]");if(in)have=1;fputs(lines[i],fp);continue;
        }
        if(in&&!strncmp(check,"autoconnect=",12)){fprintf(fp,"autoconnect=%d\n",bluetooth_autoconnect);wrote_auto=1;continue;}
        if(in&&!strncmp(check,"device=",7)){fprintf(fp,"device=%s\n",bluetooth_device_mac);wrote_dev=1;continue;}
        fputs(lines[i],fp);
    }
    if(in){if(!wrote_auto)fprintf(fp,"autoconnect=%d\n",bluetooth_autoconnect);if(!wrote_dev)fprintf(fp,"device=%s\n",bluetooth_device_mac);}
    else if(!have){if(count&&lines[count-1][0]&&lines[count-1][strlen(lines[count-1])-1]!='\n')fputc('\n',fp);fprintf(fp,"\n[bluetooth]\nautoconnect=%d\ndevice=%s\n",bluetooth_autoconnect,bluetooth_device_mac);}
    if(fflush(fp)!=0||fsync(fileno(fp))!=0){fclose(fp);goto fail;}if(fclose(fp)!=0)goto fail;
    for(size_t i=0;i<count;i++)free(lines[i]);
    free(lines);
    return 0;
fail:
    for(size_t i=0;i<count;i++)free(lines[i]);
    free(lines);
    return -1;
}

static int device_info(const char *mac,char *name,size_t name_size,int *paired,int *trusted,int *connected)
{
    if(!bluetooth_mac_valid(mac))return -1;
    char cmd[256];snprintf(cmd,sizeof(cmd),"bluetoothctl info %s 2>/dev/null",mac);
    FILE *fp=popen(cmd,"r");if(!fp)return -1;
    char line[512];if(name&&name_size)name[0]='\0';if(paired)*paired=0;if(trusted)*trusted=0;if(connected)*connected=0;
    while(fgets(line,sizeof(line),fp)){
        char *p=line;while(*p&&isspace((unsigned char)*p))p++;
        if(!strncmp(p,"Alias:",6)&&name){p+=6;util_trim(p);snprintf(name,name_size,"%s",p);}
        else if(!strncmp(p,"Name:",5)&&name&&!name[0]){p+=5;util_trim(p);snprintf(name,name_size,"%s",p);}
        else if(!strncmp(p,"Paired:",7)&&paired)*paired=strstr(p,"yes")!=NULL;
        else if(!strncmp(p,"Trusted:",8)&&trusted)*trusted=strstr(p,"yes")!=NULL;
        else if(!strncmp(p,"Connected:",10)&&connected)*connected=strstr(p,"yes")!=NULL;
    }
    int rc=pclose(fp);return rc==0?0:-1;
}

int bluetooth_scan_paired_trusted(BluetoothDevice *devices,int max_devices)
{
    if(!bluetooth_adapter_present())return 0;
    if(!devices||max_devices<=0)return 0;
    FILE *fp=popen("bluetoothctl devices 2>/dev/null","r");if(!fp)return 0;
    char line[512];int count=0;
    while(count<max_devices&&fgets(line,sizeof(line),fp)){
        char mac[18]="",listed_name[128]="";
        if(sscanf(line,"Device %17s %127[^\n]",mac,listed_name)<1)continue;
        if(!bluetooth_mac_valid(mac))continue;
        int paired=0,trusted=0,connected=0;char name[128]="";
        if(device_info(mac,name,sizeof(name),&paired,&trusted,&connected)!=0)continue;
        if(!paired||!trusted)continue;
        snprintf(devices[count].mac,sizeof(devices[count].mac),"%s",mac);
        snprintf(devices[count].name,sizeof(devices[count].name),"%s",name[0]?name:listed_name);
        devices[count].connected=connected;count++;
    }
    pclose(fp);return count;
}

static void mac_to_bluez_path(const char *mac,char *path,size_t path_size)
{
    char id[18];snprintf(id,sizeof(id),"%s",mac);
    for(char *p=id;*p;p++)if(*p==':')*p='_';
    snprintf(path,path_size,"/org/bluez/hci0/dev_%s",id);
}

int bluetooth_connect_device(const char *mac)
{
    if(!bluetooth_adapter_powered()){app_logf("Bluetooth: Adapter ausgeschaltet oder nicht verfuegbar");return -1;}
    if(!bluetooth_mac_valid(mac))return -1;

    int paired=0,trusted=0,connected=0;
    if(device_info(mac,NULL,0,&paired,&trusted,&connected)!=0){
        app_logf("Bluetooth: Geraet %s nicht gefunden",mac);
        return -1;
    }
    if(connected){
        app_logf("Bluetooth: %s bereits verbunden",mac);
        return 0;
    }
    if(!paired||!trusted){
        app_logf("Bluetooth: %s nicht paired/trusted",mac);
        return -1;
    }

    char path[96];mac_to_bluez_path(mac,path,sizeof(path));
    app_logf("Bluetooth: Connect %s",mac);
    app_logf("Bluetooth: D-Bus %s",path);

    for(int attempt=1;attempt<=3;attempt++){
        sd_bus *bus=NULL;
        sd_bus_error error=SD_BUS_ERROR_NULL;
        sd_bus_message *reply=NULL;

        int r=sd_bus_open_system(&bus);
        if(r<0){
            app_logf("Bluetooth: System-D-Bus Fehler %d",r);
        }else{
            r=sd_bus_call_method(bus,
                                 "org.bluez",
                                 path,
                                 "org.bluez.Device1",
                                 "Connect",
                                 &error,
                                 &reply,
                                 "");
            if(r<0){
                app_logf("Bluetooth: Versuch %d Fehler",attempt);
                if(error.name&&error.name[0])app_logf("BT Fehler: %s",error.name);
                if(error.message&&error.message[0])app_logf("BT Text: %s",error.message);
            }else{
                app_logf("Bluetooth: Connect-Aufruf OK");
            }
        }

        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        sd_bus_unref(bus);

        for(int i=0;i<10;i++){
            usleep(200000);
            connected=0;
            device_info(mac,NULL,0,NULL,NULL,&connected);
            if(connected){
                app_logf("Bluetooth: %s verbunden",mac);
                return 0;
            }
        }

        if(attempt<3){
            app_logf("Bluetooth: Retry in 2 s");
            sleep(2);
        }
    }

    app_logf("Bluetooth: %s Verbindung fehlgeschlagen",mac);
    return -1;
}

void bluetooth_autoconnect_start(void)
{
    if(!bluetooth_adapter_powered())return;
    if(!bluetooth_autoconnect||!bluetooth_mac_valid(bluetooth_device_mac))return;
    pid_t pid=fork();
    if(pid<0){app_logf("Bluetooth Autoconnect: fork fehlgeschlagen");return;}
    if(pid==0){
        pid_t grandchild=fork();
        if(grandchild<0)_exit(1);
        if(grandchild>0)_exit(0);
        sleep(2);
        int rc=bluetooth_connect_device(bluetooth_device_mac);
        _exit(rc==0?0:1);
    }
    waitpid(pid,NULL,0);
    app_logf("Bluetooth Autoconnect: %s",bluetooth_device_mac);
}

void bluetooth_log_status(void)
{
    int present=bluetooth_adapter_present();
    int service=bluetooth_service_available();
    int powered=bluetooth_adapter_powered();

    app_logf("Bluetooth: adapter=%s bluez=%s powered=%s",
             present?"ja":"nein",
             service?"ja":"nein",
             powered?"ja":"nein");
}

void bluetooth_log_if_changed(void)
{
    static int initialized=0;
    static int last_present=-1;
    static int last_service=-1;
    static int last_powered=-1;

    int present=bluetooth_adapter_present();
    int service=bluetooth_service_available();
    int powered=bluetooth_adapter_powered();

    if(!initialized||
       present!=last_present||
       service!=last_service||
       powered!=last_powered){
        app_logf("=== Bluetooth ===");
        app_logf("Bluetooth: adapter=%s bluez=%s powered=%s",
                 present?"ja":"nein",
                 service?"ja":"nein",
                 powered?"ja":"nein");
        last_present=present;
        last_service=service;
        last_powered=powered;
        initialized=1;
    }
}
