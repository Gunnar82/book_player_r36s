#include "systemstats.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
static unsigned long long prev_total=0,prev_idle=0;static int cpu_initialized=0;
static int read_cpu_counters(unsigned long long *total,unsigned long long *idle){FILE *fp=fopen("/proc/stat","r");if(!fp)return 0;char line[256];unsigned long long user,nice,system,idle_v,iowait,irq,softirq,steal;if(!fgets(line,sizeof(line),fp)){fclose(fp);return 0;}fclose(fp);if(sscanf(line,"cpu %llu %llu %llu %llu %llu %llu %llu %llu",&user,&nice,&system,&idle_v,&iowait,&irq,&softirq,&steal)<4)return 0;*idle=idle_v+iowait;*total=user+nice+system+idle_v+iowait+irq+softirq+steal;return 1;}
double get_cpu_usage(void){unsigned long long total,idle;if(!read_cpu_counters(&total,&idle))return -1.0;if(!cpu_initialized){prev_total=total;prev_idle=idle;cpu_initialized=1;return 0.0;}unsigned long long td=total-prev_total,id=idle-prev_idle;prev_total=total;prev_idle=idle;if(td==0)return 0.0;if(id>td)id=td;return ((double)(td-id)*100.0)/(double)td;}
double get_ram_usage(void){FILE *fp=fopen("/proc/meminfo","r");if(!fp)return -1.0;unsigned long long total=0,available=0,value;char key[64],unit[16];while(fscanf(fp,"%63s %llu %15s",key,&value,unit)==3){if(!strcmp(key,"MemTotal:"))total=value;else if(!strcmp(key,"MemAvailable:"))available=value;}fclose(fp);if(total==0)return -1.0;if(available>total)available=total;return ((double)(total-available)*100.0)/(double)total;}
static double read_temperature_file(const char *path){FILE *fp=fopen(path,"r");if(!fp)return -1.0;long value;if(fscanf(fp,"%ld",&value)!=1){fclose(fp);return -1.0;}fclose(fp);if(value>1000||value<-1000)return value/1000.0;return(double)value;}
double get_cpu_temperature(void){double temp=read_temperature_file("/sys/class/thermal/thermal_zone0/temp");if(temp>=0.0)return temp;DIR *dir=opendir("/sys/class/thermal");if(!dir)return -1.0;struct dirent *entry;while((entry=readdir(dir))!=NULL){if(strncmp(entry->d_name,"thermal_zone",11)!=0)continue;char path[256];snprintf(path,sizeof(path),"/sys/class/thermal/%s/temp",entry->d_name);temp=read_temperature_file(path);if(temp>=0.0){closedir(dir);return temp;}}closedir(dir);return -1.0;}
