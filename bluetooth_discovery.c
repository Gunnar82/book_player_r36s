#include "bluetooth_discovery.h"
#include "app_log.h"

#include <ctype.h>
#include <stdio.h>
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

static int parse_device_properties(sd_bus_message *m,BluetoothDevice *device)
{
    int r;
    char address[18]="";
    char alias[128]="";
    char name[128]="";
    int paired=0;
    int connected=0;

    r=sd_bus_message_enter_container(m,'a',"{sv}");
    if(r<0)return r;

    while((r=sd_bus_message_enter_container(m,'e',"sv"))>0){
        const char *key=NULL;
        const char *sig=NULL;
        r=sd_bus_message_read_basic(m,'s',&key);
        if(r<0)return r;
        r=sd_bus_message_peek_type(m,NULL,&sig);
        if(r<0)return r;
        r=sd_bus_message_enter_container(m,'v',sig);
        if(r<0)return r;

        if(key&&!strcmp(key,"Address")&&sig&&sig[0]=='s'){
            const char *v=NULL;
            r=sd_bus_message_read_basic(m,'s',&v);
            if(r<0)return r;
            if(v)snprintf(address,sizeof(address),"%s",v);
        }else if(key&&!strcmp(key,"Alias")&&sig&&sig[0]=='s'){
            const char *v=NULL;
            r=sd_bus_message_read_basic(m,'s',&v);
            if(r<0)return r;
            if(v)snprintf(alias,sizeof(alias),"%s",v);
        }else if(key&&!strcmp(key,"Name")&&sig&&sig[0]=='s'){
            const char *v=NULL;
            r=sd_bus_message_read_basic(m,'s',&v);
            if(r<0)return r;
            if(v)snprintf(name,sizeof(name),"%s",v);
        }else if(key&&!strcmp(key,"Paired")&&sig&&sig[0]=='b'){
            r=sd_bus_message_read_basic(m,'b',&paired);
            if(r<0)return r;
        }else if(key&&!strcmp(key,"Connected")&&sig&&sig[0]=='b'){
            r=sd_bus_message_read_basic(m,'b',&connected);
            if(r<0)return r;
        }else{
            r=sd_bus_message_skip(m,sig);
            if(r<0)return r;
        }

        r=sd_bus_message_exit_container(m);
        if(r<0)return r;
        r=sd_bus_message_exit_container(m);
        if(r<0)return r;
    }
    if(r<0)return r;
    r=sd_bus_message_exit_container(m);
    if(r<0)return r;

    if(!mac_valid(address))return 0;
    snprintf(device->mac,sizeof(device->mac),"%s",address);
    snprintf(device->name,sizeof(device->name),"%s",alias[0]?alias:(name[0]?name:address));
    device->paired=paired?1:0;
    device->connected=connected?1:0;
    return 1;
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
        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        sd_bus_unref(bus);
        return 0;
    }

    int count=0;
    r=sd_bus_message_enter_container(reply,'a',"{oa{sa{sv}}}");
    if(r<0)goto out;

    while((r=sd_bus_message_enter_container(reply,'e',"oa{sa{sv}}"))>0){
        const char *object_path=NULL;
        r=sd_bus_message_read_basic(reply,'o',&object_path);
        if(r<0)break;

        r=sd_bus_message_enter_container(reply,'a',"{sa{sv}}");
        if(r<0)break;

        int have_device=0;
        BluetoothDevice candidate;
        memset(&candidate,0,sizeof(candidate));

        while((r=sd_bus_message_enter_container(reply,'e',"sa{sv}"))>0){
            const char *interface_name=NULL;
            r=sd_bus_message_read_basic(reply,'s',&interface_name);
            if(r<0)break;

            if(interface_name&&!strcmp(interface_name,"org.bluez.Device1")){
                int parsed=parse_device_properties(reply,&candidate);
                if(parsed<0){r=parsed;break;}
                if(parsed>0)have_device=1;
            }else{
                r=sd_bus_message_skip(reply,"a{sv}");
                if(r<0)break;
            }

            r=sd_bus_message_exit_container(reply);
            if(r<0)break;
        }
        if(r<0)break;

        r=sd_bus_message_exit_container(reply);
        if(r<0)break;
        r=sd_bus_message_exit_container(reply);
        if(r<0)break;

        if(have_device){
            if(count<max_devices)devices[count++]=candidate;
            if(discovery_active)app_logf("Bluetooth Discovery: Device %s %s paired=%d connected=%d",
                                         candidate.mac,candidate.name,candidate.paired,candidate.connected);
        }
        (void)object_path;
    }

    if(r>=0)sd_bus_message_exit_container(reply);

out:
    if(r<0)app_logf("Bluetooth Discovery: Parse-Fehler %d",r);
    if(discovery_active&&count!=last_logged_count){
        app_logf("Bluetooth Discovery: %d Geraete sichtbar",count);
        last_logged_count=count;
    }
    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return count;
}
