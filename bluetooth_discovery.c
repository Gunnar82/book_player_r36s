#include "bluetooth_discovery.h"
#include "app_log.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>

static int discovery_active=0;

static int mac_valid(const char *mac)
{
    if(!mac||strlen(mac)!=17)return 0;
    for(int i=0;i<17;i++){
        if(i%3==2){if(mac[i]!=':')return 0;}
        else if(!isxdigit((unsigned char)mac[i]))return 0;
    }
    return 1;
}

static int adapter_method(const char *method)
{
    const char *paths[]={"/org/bluez/hci0","/org/bluez/hci1"};
    sd_bus *bus=NULL;
    int r=sd_bus_open_system(&bus);
    if(r<0)return r;

    int last=-1;
    for(size_t i=0;i<sizeof(paths)/sizeof(paths[0]);i++){
        sd_bus_error error=SD_BUS_ERROR_NULL;
        sd_bus_message *reply=NULL;
        r=sd_bus_call_method(bus,"org.bluez",paths[i],"org.bluez.Adapter1",method,&error,&reply,"");
        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        if(r>=0){last=0;break;}
        last=r;
    }
    sd_bus_unref(bus);
    return last;
}

int bluetooth_discovery_start(void)
{
    int r=adapter_method("StartDiscovery");
    if(r<0){app_logf("Bluetooth Discovery: Start fehlgeschlagen (%d)",r);return -1;}
    discovery_active=1;
    app_logf("Bluetooth Discovery: gestartet");
    return 0;
}

int bluetooth_discovery_stop(void)
{
    if(!discovery_active)return 0;
    int r=adapter_method("StopDiscovery");
    discovery_active=0;
    if(r<0){app_logf("Bluetooth Discovery: Stop fehlgeschlagen (%d)",r);return -1;}
    app_logf("Bluetooth Discovery: gestoppt");
    return 0;
}

int bluetooth_discovery_active(void)
{
    return discovery_active;
}

static int query_info(const char *mac,char *name,size_t name_size,int *paired,int *connected)
{
    if(!mac_valid(mac))return -1;
    char cmd[256];
    snprintf(cmd,sizeof(cmd),"bluetoothctl info %s 2>/dev/null",mac);
    FILE *fp=popen(cmd,"r");
    if(!fp)return -1;
    if(name&&name_size)name[0]='\0';
    if(paired)*paired=0;
    if(connected)*connected=0;
    char line[512];
    while(fgets(line,sizeof(line),fp)){
        char *p=line;
        while(*p&&isspace((unsigned char)*p))p++;
        if(!strncmp(p,"Alias:",6)&&name){p+=6;util_trim(p);snprintf(name,name_size,"%s",p);}
        else if(!strncmp(p,"Name:",5)&&name&&!name[0]){p+=5;util_trim(p);snprintf(name,name_size,"%s",p);}
        else if(!strncmp(p,"Paired:",7)&&paired)*paired=strstr(p,"yes")!=NULL;
        else if(!strncmp(p,"Connected:",10)&&connected)*connected=strstr(p,"yes")!=NULL;
    }
    return pclose(fp)==0?0:-1;
}

int bluetooth_scan_all(BluetoothDevice *devices,int max_devices)
{
    if(!devices||max_devices<=0)return 0;
    FILE *fp=popen("bluetoothctl devices 2>/dev/null","r");
    if(!fp)return 0;
    char line[512];
    int count=0;
    while(count<max_devices&&fgets(line,sizeof(line),fp)){
        char mac[18]="",listed_name[128]="";
        if(sscanf(line,"Device %17s %127[^\n]",mac,listed_name)<1)continue;
        if(!mac_valid(mac))continue;
        char name[128]="";
        int paired=0,connected=0;
        if(query_info(mac,name,sizeof(name),&paired,&connected)!=0)continue;
        snprintf(devices[count].mac,sizeof(devices[count].mac),"%s",mac);
        snprintf(devices[count].name,sizeof(devices[count].name),"%s",name[0]?name:listed_name);
        devices[count].connected=connected;
        devices[count].paired=paired;
        count++;
    }
    pclose(fp);
    return count;
}
