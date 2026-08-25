#include "bluez_discovery.h"
#include "app_log.h"

#include <systemd/sd-bus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static int active;

static void mac_to_path_id(const char *path,char *mac,size_t n)
{
    const char *p=strstr(path,"/dev_");
    if(!p){mac[0]='\0';return;}
    p+=5;
    size_t i=0;
    while(*p&&i+1<n){
        mac[i++]=(*p=='_')?':':*p++;
    }
    mac[i]='\0';
}

int bluez_discovery_start(void)
{
#ifdef BUILD_R36S
    /* On the R36S BlueZ stack, a direct Adapter1.StartDiscovery call does not
       reliably populate new Device1 objects. bluetoothctl scan on does the
       required setup first and has been verified on the device. Run it in the
       background so the UI remains responsive. */
    int rc=system("bluetoothctl scan on >/tmp/hoerspiel_bt_scan.log 2>&1 &");
    if(rc!=0){
        app_logf("Bluetooth R36S: bluetoothctl scan on konnte nicht gestartet werden (%d)",rc);
        return -1;
    }
    active=1;
    app_logf("Bluetooth R36S: Suche via bluetoothctl gestartet");
    return 0;
#else
    sd_bus *bus=NULL;
    int r=sd_bus_open_system(&bus);
    if(r<0)return -1;
    r=sd_bus_call_method(bus,"org.bluez","/org/bluez/hci0","org.bluez.Adapter1","StartDiscovery",NULL,NULL,"");
    sd_bus_unref(bus);
    if(r<0)return -1;
    active=1;
    return 0;
#endif
}

void bluez_discovery_stop(void)
{
    if(!active)return;
#ifdef BUILD_R36S
    /* scan off is intentionally a separate short-lived bluetoothctl process;
       it stops the discovery started by the background scan-on client. */
    int rc=system("bluetoothctl scan off >/dev/null 2>&1");
    if(rc!=0)app_logf("Bluetooth R36S: bluetoothctl scan off fehlgeschlagen (%d)",rc);
    system("pkill -f 'bluetoothctl scan on' >/dev/null 2>&1 || true");
#else
    sd_bus *bus=NULL;
    if(sd_bus_open_system(&bus)>=0){
        sd_bus_call_method(bus,"org.bluez","/org/bluez/hci0","org.bluez.Adapter1","StopDiscovery",NULL,NULL,"");
        sd_bus_unref(bus);
    }
#endif
    active=0;
    app_logf("Bluetooth R36S: Suche gestoppt");
}

int bluez_discovery_devices(BluetoothDevice *devices,int max_devices)
{
    if(!devices||max_devices<=0)return 0;
    sd_bus *bus=NULL;
    sd_bus_message *reply=NULL;
    sd_bus_error error=SD_BUS_ERROR_NULL;
    if(sd_bus_open_system(&bus)<0)return 0;

    int r=sd_bus_call_method(bus,
                             "org.bluez",
                             "/",
                             "org.freedesktop.DBus.ObjectManager",
                             "GetManagedObjects",
                             &error,&reply,"");
    if(r<0){
        sd_bus_error_free(&error);
        sd_bus_unref(bus);
        return 0;
    }

    int count=0;
    r=sd_bus_message_enter_container(reply,'a',"{oa{sa{sv}}}");
    while(r>0&&count<max_devices){
        r=sd_bus_message_enter_container(reply,'e',"oa{sa{sv}}");
        if(r<=0)break;

        const char *path=NULL;
        sd_bus_message_read_basic(reply,'o',&path);
        char mac[18]="",name[128]="",icon[64]="";
        int paired=0,trusted=0,connected=0,have_device=0;

        int ir=sd_bus_message_enter_container(reply,'a',"{sa{sv}}");
        while(ir>0){
            ir=sd_bus_message_enter_container(reply,'e',"sa{sv}");
            if(ir<=0)break;

            const char *iface=NULL;
            sd_bus_message_read_basic(reply,'s',&iface);
            int pr=sd_bus_message_enter_container(reply,'a',"{sv}");
            if(iface&&!strcmp(iface,"org.bluez.Device1"))have_device=1;

            while(pr>0){
                pr=sd_bus_message_enter_container(reply,'e',"sv");
                if(pr<=0)break;

                const char *key=NULL;
                sd_bus_message_read_basic(reply,'s',&key);
                sd_bus_message_enter_container(reply,'v',NULL);

                if(have_device&&key){
                    if(!strcmp(key,"Address")){
                        const char *v=NULL;
                        sd_bus_message_read_basic(reply,'s',&v);
                        if(v)snprintf(mac,sizeof(mac),"%s",v);
                    }else if(!strcmp(key,"Alias")||(!name[0]&&!strcmp(key,"Name"))){
                        const char *v=NULL;
                        sd_bus_message_read_basic(reply,'s',&v);
                        if(v)snprintf(name,sizeof(name),"%s",v);
                    }else if(!strcmp(key,"Icon")){
                        const char *v=NULL;
                        sd_bus_message_read_basic(reply,'s',&v);
                        if(v)snprintf(icon,sizeof(icon),"%s",v);
                    }else if(!strcmp(key,"Paired")){
                        sd_bus_message_read_basic(reply,'b',&paired);
                    }else if(!strcmp(key,"Trusted")){
                        sd_bus_message_read_basic(reply,'b',&trusted);
                    }else if(!strcmp(key,"Connected")){
                        sd_bus_message_read_basic(reply,'b',&connected);
                    }else{
                        sd_bus_message_skip(reply,NULL);
                    }
                }else{
                    sd_bus_message_skip(reply,NULL);
                }

                sd_bus_message_exit_container(reply);
                sd_bus_message_exit_container(reply);
            }
            sd_bus_message_exit_container(reply);
            sd_bus_message_exit_container(reply);
        }
        sd_bus_message_exit_container(reply);
        sd_bus_message_exit_container(reply);

        if(have_device&&!paired&&!trusted){
            if(!mac[0])mac_to_path_id(path,mac,sizeof(mac));
            if(mac[0]){
                snprintf(devices[count].mac,sizeof(devices[count].mac),"%s",mac);
                snprintf(devices[count].name,sizeof(devices[count].name),"%s",name[0]?name:mac);
                devices[count].connected=connected;
                snprintf(devices[count].type,sizeof(devices[count].type),"%s",
                         strstr(icon,"audio")?"audio":
                         strstr(icon,"phone")?"phone":"unknown");
                count++;
            }
        }
    }

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return count;
}
