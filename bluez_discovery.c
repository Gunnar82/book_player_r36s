#include "bluez_discovery.h"
#include "app_log.h"

#include <systemd/sd-bus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static int active;
#ifdef BUILD_R36S
static pid_t scan_pid=-1;
static int scan_stdin=-1;
#endif

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

#ifdef BUILD_R36S
static int start_bluetoothctl_scan(void)
{
    if(scan_pid>0&&scan_stdin>=0)return 0;

    int pipefd[2];
    if(pipe(pipefd)!=0)return -1;

    pid_t pid=fork();
    if(pid<0){
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if(pid==0){
        close(pipefd[1]);
        if(dup2(pipefd[0],STDIN_FILENO)<0)_exit(126);
        if(pipefd[0]!=STDIN_FILENO)close(pipefd[0]);

        int logfd=open("/tmp/hoerspiel_bt_scan.log",O_WRONLY|O_CREAT|O_TRUNC,0644);
        if(logfd>=0){
            dup2(logfd,STDOUT_FILENO);
            dup2(logfd,STDERR_FILENO);
            if(logfd>STDERR_FILENO)close(logfd);
        }

        execlp("bluetoothctl","bluetoothctl",(char*)NULL);
        _exit(127);
    }

    close(pipefd[0]);
    scan_pid=pid;
    scan_stdin=pipefd[1];

    const char command[]="scan on\n";
    ssize_t written=write(scan_stdin,command,sizeof(command)-1);
    if(written!=(ssize_t)(sizeof(command)-1)){
        app_logf("Bluetooth R36S: scan-on Kommando konnte nicht gesendet werden");
        close(scan_stdin);
        scan_stdin=-1;
        kill(scan_pid,SIGTERM);
        waitpid(scan_pid,NULL,0);
        scan_pid=-1;
        return -1;
    }

    app_logf("Bluetooth R36S: bluetoothctl Scan-Prozess gestartet (pid=%ld)",(long)scan_pid);
    return 0;
}

static void stop_bluetoothctl_scan(void)
{
    if(scan_stdin>=0){
        const char commands[]="scan off\nquit\n";
        write(scan_stdin,commands,sizeof(commands)-1);
        close(scan_stdin);
        scan_stdin=-1;
    }

    if(scan_pid>0){
        for(int i=0;i<20;i++){
            int status=0;
            pid_t r=waitpid(scan_pid,&status,WNOHANG);
            if(r==scan_pid||r<0){scan_pid=-1;break;}
            usleep(50000);
        }
        if(scan_pid>0){
            app_logf("Bluetooth R36S: bluetoothctl beendet sich nicht, SIGTERM");
            kill(scan_pid,SIGTERM);
            waitpid(scan_pid,NULL,0);
            scan_pid=-1;
        }
    }
}
#endif

int bluez_discovery_start(void)
{
#ifdef BUILD_R36S
    if(start_bluetoothctl_scan()!=0){
        app_logf("Bluetooth R36S: bluetoothctl Suche konnte nicht gestartet werden");
        return -1;
    }
    active=1;
    app_logf("Bluetooth R36S: Suche via persistentem bluetoothctl gestartet");
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
    stop_bluetoothctl_scan();
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
