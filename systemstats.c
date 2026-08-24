#include "systemstats.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <stdlib.h>
#include "app_log.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "bluetooth.h"
static unsigned long long prev_total=0,prev_idle=0;static int cpu_initialized=0;
static int read_cpu_counters(unsigned long long *total,unsigned long long *idle){FILE *fp=fopen("/proc/stat","r");if(!fp)return 0;char line[256];unsigned long long user,nice,system,idle_v,iowait,irq,softirq,steal;if(!fgets(line,sizeof(line),fp)){fclose(fp);return 0;}fclose(fp);if(sscanf(line,"cpu %llu %llu %llu %llu %llu %llu %llu %llu",&user,&nice,&system,&idle_v,&iowait,&irq,&softirq,&steal)<4)return 0;*idle=idle_v+iowait;*total=user+nice+system+idle_v+iowait+irq+softirq+steal;return 1;}
double get_cpu_usage(void){unsigned long long total,idle;if(!read_cpu_counters(&total,&idle))return -1.0;if(!cpu_initialized){prev_total=total;prev_idle=idle;cpu_initialized=1;return 0.0;}unsigned long long td=total-prev_total,id=idle-prev_idle;prev_total=total;prev_idle=idle;if(td==0)return 0.0;if(id>td)id=td;return ((double)(td-id)*100.0)/(double)td;}
double get_ram_usage(void){FILE *fp=fopen("/proc/meminfo","r");if(!fp)return -1.0;unsigned long long total=0,available=0,value;char key[64],unit[16];while(fscanf(fp,"%63s %llu %15s",key,&value,unit)==3){if(!strcmp(key,"MemTotal:"))total=value;else if(!strcmp(key,"MemAvailable:"))available=value;}fclose(fp);if(total==0)return -1.0;if(available>total)available=total;return ((double)(total-available)*100.0)/(double)total;}
static double read_temperature_file(const char *path){FILE *fp=fopen(path,"r");if(!fp)return -1.0;long value;if(fscanf(fp,"%ld",&value)!=1){fclose(fp);return -1.0;}fclose(fp);if(value>1000||value<-1000)return value/1000.0;return(double)value;}
double get_cpu_temperature(void){double temp=read_temperature_file("/sys/class/thermal/thermal_zone0/temp");if(temp>=0.0)return temp;DIR *dir=opendir("/sys/class/thermal");if(!dir)return -1.0;struct dirent *entry;while((entry=readdir(dir))!=NULL){if(strncmp(entry->d_name,"thermal_zone",11)!=0)continue;char path[512];
int n=snprintf(path,sizeof(path),"/sys/class/thermal/%s/temp",entry->d_name);
if(n<0||(size_t)n>=sizeof(path))continue;
temp=read_temperature_file(path);if(temp>=0.0){closedir(dir);return temp;}}closedir(dir);return -1.0;}


static int interface_has_ipv4(const char *iface)
{
    if(!iface||!iface[0])return 0;

    struct ifaddrs *ifaddr=NULL;
    if(getifaddrs(&ifaddr)!=0)return 0;

    int found=0;
    for(struct ifaddrs *ifa=ifaddr;ifa;ifa=ifa->ifa_next){
        if(!ifa->ifa_name||!ifa->ifa_addr)continue;
        if(strcmp(ifa->ifa_name,iface))continue;
        if(ifa->ifa_addr->sa_family!=AF_INET)continue;

        struct sockaddr_in *sin=(struct sockaddr_in *)ifa->ifa_addr;
        if(sin->sin_addr.s_addr!=htonl(INADDR_LOOPBACK)){
            found=1;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return found;
}

static int interface_is_usable(const char *iface)
{
    if(!iface||!iface[0])return 0;

    char path[256];
    char state[64]={0};

    int n=snprintf(path,sizeof(path),"/sys/class/net/%s/operstate",iface);
    if(n<0||(size_t)n>=sizeof(path))return 0;

    FILE *fp=fopen(path,"r");
    if(fp){
        if(fgets(state,sizeof(state),fp))
            state[strcspn(state,"\r\n")]='\0';
        fclose(fp);

        if(!strcmp(state,"down")||
           !strcmp(state,"dormant")||
           !strcmp(state,"notpresent")||
           !strcmp(state,"lowerlayerdown"))
            return 0;
    }

    n=snprintf(path,sizeof(path),"/sys/class/net/%s/flags",iface);
    if(n<0||(size_t)n>=sizeof(path))return 0;

    fp=fopen(path,"r");
    if(fp){
        unsigned long flags=0;
        if(fscanf(fp,"%lx",&flags)!=1){
            fclose(fp);
            return 0;
        }
        fclose(fp);

        /* IFF_UP */
        if((flags & 0x1)==0)return 0;
    }

    n=snprintf(path,sizeof(path),"/sys/class/net/%s/carrier",iface);
    if(n<0||(size_t)n>=sizeof(path))return 0;

    fp=fopen(path,"r");
    if(fp){
        int carrier=0;
        if(fscanf(fp,"%d",&carrier)!=1){
            fclose(fp);
            return 0;
        }
        fclose(fp);
        if(carrier!=1)return 0;
    }else if(state[0]&&strcmp(state,"up")&&strcmp(state,"unknown")){
        return 0;
    }

    /*
     * Batocera: ein verbundenes wlan0 hat eine echte IPv4-Adresse.
     * Das schützt außerdem gegen übrig gebliebene Default-Routen.
     */
    if(!interface_has_ipv4(iface))
        return 0;

    return 1;
}

int network_connection_active(void)
{
    FILE *fp=fopen("/proc/net/route","r");
    if(!fp)return 0;

    char line[512];

    if(!fgets(line,sizeof(line),fp)){
        fclose(fp);
        return 0;
    }

    while(fgets(line,sizeof(line),fp)){
        char iface[64]={0};
        unsigned long destination=0,gateway=0,flags=0;

        if(sscanf(line,"%63s %lx %lx %lx",
                  iface,&destination,&gateway,&flags)==4){
            /* Default-Route + Route aktiv + Interface wirklich nutzbar. */
            if(destination==0 &&
               (flags & 0x1) &&
               interface_is_usable(iface)){
                fclose(fp);
                return 1;
            }
        }
    }

    fclose(fp);
    return 0;
}


int network_get_active_ipv4(char *out,size_t out_size)
{
    if(!out||out_size==0)return 0;
    out[0]='\0';

    FILE *fp=fopen("/proc/net/route","r");
    if(!fp)return 0;

    char line[512];
    if(!fgets(line,sizeof(line),fp)){fclose(fp);return 0;}

    char active_iface[64]={0};
    while(fgets(line,sizeof(line),fp)){
        char iface[64]={0};
        unsigned long destination=0,gateway=0,flags=0;
        if(sscanf(line,"%63s %lx %lx %lx",iface,&destination,&gateway,&flags)==4 &&
           destination==0 && (flags&0x1) && interface_is_usable(iface)){
            snprintf(active_iface,sizeof(active_iface),"%s",iface);
            break;
        }
    }
    fclose(fp);
    if(!active_iface[0])return 0;

    struct ifaddrs *ifaddr=NULL;
    if(getifaddrs(&ifaddr)!=0)return 0;
    int found=0;
    for(struct ifaddrs *ifa=ifaddr;ifa;ifa=ifa->ifa_next){
        if(!ifa->ifa_name||!ifa->ifa_addr)continue;
        if(strcmp(ifa->ifa_name,active_iface))continue;
        if(ifa->ifa_addr->sa_family!=AF_INET)continue;
        struct sockaddr_in *sin=(struct sockaddr_in *)ifa->ifa_addr;
        if(inet_ntop(AF_INET,&sin->sin_addr,out,out_size)){found=1;break;}
    }
    freeifaddrs(ifaddr);
    if(!found)out[0]='\0';
    return found;
}

void network_log_status(void)
{
    FILE *fp=fopen("/proc/net/route","r");
    if(!fp){
        app_logf("Netzwerk: /proc/net/route nicht lesbar");
        return;
    }

    char line[512];
    if(!fgets(line,sizeof(line),fp)){
        app_logf("Netzwerk: Routingtabelle leer");
        fclose(fp);
        return;
    }

    int found_default=0;

    while(fgets(line,sizeof(line),fp)){
        char iface[64]={0};
        unsigned long destination=0,gateway=0,flags=0;

        if(sscanf(line,"%63s %lx %lx %lx",
                  iface,&destination,&gateway,&flags)!=4)
            continue;

        if(destination!=0)
            continue;

        found_default=1;

        char state_path[256];
        char carrier_path[256];
        char flags_path[256];
        char state[64]="?";
        int carrier=-1;
        unsigned long iflags=0;

        snprintf(state_path,sizeof(state_path),
                 "/sys/class/net/%s/operstate",iface);
        FILE *sf=fopen(state_path,"r");
        if(sf){
            if(fgets(state,sizeof(state),sf))
                state[strcspn(state,"\r\n")]='\0';
            fclose(sf);
        }

        snprintf(carrier_path,sizeof(carrier_path),
                 "/sys/class/net/%s/carrier",iface);
        FILE *cf=fopen(carrier_path,"r");
        if(cf){
            if(fscanf(cf,"%d",&carrier)!=1)carrier=-1;
            fclose(cf);
        }

        snprintf(flags_path,sizeof(flags_path),
                 "/sys/class/net/%s/flags",iface);
        FILE *ff=fopen(flags_path,"r");
        if(ff){
            if(fscanf(ff,"%lx",&iflags)!=1)iflags=0;
            fclose(ff);
        }

        app_logf(
            "Netzwerk: iface=%s route_flags=0x%lx operstate=%s carrier=%d iflags=0x%lx usable=%d",
            iface,flags,state,carrier,iflags,interface_is_usable(iface)
        );
    }

    fclose(fp);

    if(!found_default)
        app_logf("Netzwerk: keine Default-Route");

    app_logf("Netzwerk: aktiv=%s",
             network_connection_active()?"ja":"nein");
}


void network_log_if_changed(void)
{
    static int initialized=0;
    static int last_active=-1;

    int active=network_connection_active();

    if(!initialized||active!=last_active){
        app_logf("=== Netzwerk ===");
        network_log_status();
        last_active=active;
        initialized=1;
    }
}

#ifdef BUILD_BATOCERA
static int batocera_conf_bool(const char *key,int fallback)
{
    FILE *fp=fopen("/userdata/system/batocera.conf","r");
    if(!fp)return fallback;
    char line[512];
    int result=fallback;
    size_t keylen=strlen(key);
    while(fgets(line,sizeof(line),fp)){
        char *p=line;
        while(*p==' '||*p=='\t')p++;
        if(*p=='#')p++;
        if(strncmp(p,key,keylen)||p[keylen]!='=')continue;
        p+=keylen+1;
        p[strcspn(p,"\r\n")]='\0';
        result=(!strcmp(p,"1")||!strcasecmp(p,"true")||!strcasecmp(p,"enabled"))?1:0;
    }
    fclose(fp);
    return result;
}
static int batocera_set_conf_bool(const char *key,int enabled)
{
    char cmd[512];
    int n=snprintf(cmd,sizeof(cmd),
        "batocera-settings-set %s %d >/dev/null 2>&1",
        key,enabled?1:0);
    if(n<0||(size_t)n>=sizeof(cmd))return -1;
    int rc=system(cmd);
    return rc==0?0:-1;
}
int batocera_wifi_enabled(void){return batocera_conf_bool("wifi.enabled",network_connection_active());}
int batocera_set_wifi_enabled(int enabled)
{
    int rc=batocera_set_conf_bool("wifi.enabled",enabled);
    if(rc==0){
        system(enabled?
          "batocera-wifi enable >/dev/null 2>&1":
          "batocera-wifi disable >/dev/null 2>&1");
        app_logf("WLAN: %s",enabled?"eingeschaltet":"ausgeschaltet");
    }else app_logf("WLAN: Umschalten fehlgeschlagen");
    return rc;
}
int batocera_set_bluetooth_enabled(int enabled)
{
    int rc=batocera_set_conf_bool("controllers.bluetooth.enabled",enabled);
    if(rc!=0){
        app_logf("Bluetooth: Batocera-Konfiguration konnte nicht gesetzt werden");
        return rc;
    }

    int cmd_rc=system(enabled?
        "/usr/bin/batocera-bluetooth enable >/dev/null 2>&1":
        "/usr/bin/batocera-bluetooth disable >/dev/null 2>&1");

    if(cmd_rc!=0){
        app_logf("Bluetooth: batocera-bluetooth %s fehlgeschlagen (%d)",
                 enabled?"enable":"disable",cmd_rc);
        return -1;
    }

    app_logf("Bluetooth: Batocera Sollzustand=%s, Livezustand=%s",
             enabled?"An":"Aus",
             bluetooth_adapter_powered()?"An":"Aus");
    return 0;
}
#endif
