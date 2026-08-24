#include "bluetooth_discovery.h"
#include "app_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>

static int discovery_active=0;
static int last_logged_count=-1;

static int mac_valid(const char *mac)
{
    if(!mac||strlen(mac)!=17)return 0;
    for(int i=0;i<17;i++){
        if(i%3==2){if(mac[i]!=':')return 0;}
        else if(!isxdigit((unsigned char)mac[i]))return 0;
    }
    return 1;
}

static int adapter_set_discovery_filter(sd_bus *bus,const char *path)
{
    sd_bus_message *m=NULL;
    sd_bus_message *reply=NULL;
    sd_bus_error error=SD_BUS_ERROR_NULL;
    int r=sd_bus_message_new_method_call(bus,&m,"org.bluez",path,"org.bluez.Adapter1","SetDiscoveryFilter");
    if(r<0)goto out;
    r=sd_bus_message_open_container(m,'a',"{sv}");
    if(r<0)goto out;
    r=sd_bus_message_open_container(m,'e',"sv");
    if(r<0)goto out;
    r=sd_bus_message_append_basic(m,'s',"Transport");
    if(r<0)goto out;
    r=sd_bus_message_open_container(m,'v',"s");
    if(r<0)goto out;
    r=sd_bus_message_append_basic(m,'s',"auto");
    if(r<0)goto out;
    r=sd_bus_message_close_container(m);
    if(r<0)goto out;
    r=sd_bus_message_close_container(m);
    if(r<0)goto out;
    r=sd_bus_message_close_container(m);
    if(r<0)goto out;
    r=sd_bus_call(bus,m,0,&error,&reply);
out:
    if(r<0&&error.name&&error.name[0])app_logf("Bluetooth Discovery: Filter %s",error.name);
    sd_bus_message_unref(reply);
    sd_bus_message_unref(m);
    sd_bus_error_free(&error);
    return r;
}

static int adapter_start(void)
{
    const char *paths[]={"/org/bluez/hci0","/org/bluez/hci1"};
    sd_bus *bus=NULL;
    int r=sd_bus_open_system(&bus);
    if(r<0)return r;

    int last=-1;
    for(size_t i=0;i<sizeof(paths)/sizeof(paths[0]);i++){
        sd_bus_error error=SD_BUS_ERROR_NULL;
        sd_bus_message *reply=NULL;
        r=adapter_set_discovery_filter(bus,paths[i]);
        if(r<0){last=r;continue;}
        r=sd_bus_call_method(bus,"org.bluez",paths[i],"org.bluez.Adapter1","StartDiscovery",&error,&reply,"");
        if(r>=0){
            app_logf("Bluetooth Discovery: Adapter %s",paths[i]);
            last=0;
        }else{
            if(error.name&&error.name[0])app_logf("Bluetooth Discovery: Start %s",error.name);
            last=r;
        }
        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        if(last==0)break;
    }
    sd_bus_unref(bus);
    return last;
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
    int r=adapter_start();
    if(r<0){app_logf("Bluetooth Discovery: Start fehlgeschlagen (%d)",r);return -1;}
    discovery_active=1;
    last_logged_count=-1;
    app_logf("Bluetooth Discovery: gestartet");
    return 0;
}

int bluetooth_discovery_stop(void)
{
    if(!discovery_active)return 0;
    int r=adapter_method("StopDiscovery");
    discovery_active=0;
    last_logged_count=-1;
    if(r<0){app_logf("Bluetooth Discovery: Stop fehlgeschlagen (%d)",r);return -1;}
    app_logf("Bluetooth Discovery: gestoppt");
    return 0;
}

int bluetooth_discovery_active(void)
{
    return discovery_active;
}

static int read_device(sd_bus *bus,const char *path,BluetoothDevice *device)
{
    sd_bus_error error=SD_BUS_ERROR_NULL;
    char *address=NULL;
    char *alias=NULL;
    char *name=NULL;
    int paired=0;
    int connected=0;

    int r=sd_bus_get_property_string(bus,"org.bluez",path,"org.bluez.Device1","Address",&error,&address);
    if(r<0)goto out;
    if(!mac_valid(address)){r=0;goto out;}

    sd_bus_error_free(&error);
    r=sd_bus_get_property_string(bus,"org.bluez",path,"org.bluez.Device1","Alias",&error,&alias);
    if(r<0){
        sd_bus_error_free(&error);
        (void)sd_bus_get_property_string(bus,"org.bluez",path,"org.bluez.Device1","Name",&error,&name);
    }

    sd_bus_error_free(&error);
    (void)sd_bus_get_property_trivial(bus,"org.bluez",path,"org.bluez.Device1","Paired",&error,'b',&paired);
    sd_bus_error_free(&error);
    (void)sd_bus_get_property_trivial(bus,"org.bluez",path,"org.bluez.Device1","Connected",&error,'b',&connected);

    snprintf(device->mac,sizeof(device->mac),"%s",address);
    snprintf(device->name,sizeof(device->name),"%s",(alias&&alias[0])?alias:((name&&name[0])?name:address));
    device->paired=paired?1:0;
    device->connected=connected?1:0;
    r=1;

out:
    if(r<0&&error.name&&error.name[0])
        app_logf("Bluetooth Discovery: Property %s %s",path,error.name);
    free(address);
    free(alias);
    free(name);
    sd_bus_error_free(&error);
    return r;
}

int bluetooth_scan_all(BluetoothDevice *devices,int max_devices)
{
    if(!devices||max_devices<=0)return 0;

    sd_bus *bus=NULL;
    sd_bus_error error=SD_BUS_ERROR_NULL;
    sd_bus_message *reply=NULL;
    int r=sd_bus_open_system(&bus);
    if(r<0){app_logf("Bluetooth Discovery: System-D-Bus %d",r);return 0;}

    r=sd_bus_call_method(bus,
                         "org.bluez",
                         "/",
                         "org.freedesktop.DBus.ObjectManager",
                         "GetManagedObjects",
                         &error,
                         &reply,
                         "");
    if(r<0){
        if(error.name&&error.name[0])app_logf("Bluetooth Discovery: ObjectManager %s",error.name);
        goto out;
    }

    int count=0;
    r=sd_bus_message_enter_container(reply,'a',"{oa{sa{sv}}}");
    if(r<0)goto out;

    while((r=sd_bus_message_enter_container(reply,'e',"oa{sa{sv}}"))>0){
        const char *object_path=NULL;
        r=sd_bus_message_read_basic(reply,'o',&object_path);
        if(r<0)break;

        int is_device=0;
        r=sd_bus_message_enter_container(reply,'a',"{sa{sv}}");
        if(r<0)break;

        while((r=sd_bus_message_enter_container(reply,'e',"sa{sv}"))>0){
            const char *interface_name=NULL;
            r=sd_bus_message_read_basic(reply,'s',&interface_name);
            if(r<0)break;
            if(interface_name&&!strcmp(interface_name,"org.bluez.Device1"))is_device=1;
            r=sd_bus_message_skip(reply,"a{sv}");
            if(r<0)break;
            r=sd_bus_message_exit_container(reply);
            if(r<0)break;
        }
        if(r<0)break;

        r=sd_bus_message_exit_container(reply);
        if(r<0)break;
        r=sd_bus_message_exit_container(reply);
        if(r<0)break;

        if(is_device&&object_path&&count<max_devices){
            BluetoothDevice candidate;
            memset(&candidate,0,sizeof(candidate));
            int rr=read_device(bus,object_path,&candidate);
            if(rr>0){
                devices[count++]=candidate;
                if(discovery_active)app_logf("Bluetooth Discovery: Device %s %s paired=%d connected=%d",
                                             candidate.mac,candidate.name,candidate.paired,candidate.connected);
            }
        }
    }

    if(r>=0)sd_bus_message_exit_container(reply);
    if(r<0)app_logf("Bluetooth Discovery: Parse-Fehler %d",r);
    if(discovery_active&&count!=last_logged_count){
        app_logf("Bluetooth Discovery: %d Geraete sichtbar",count);
        last_logged_count=count;
    }

out:
    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return r<0?0:count;
}
