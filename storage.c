#include "storage.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CONFIG_FILE "config.ini"
static char config_path[1024];

static int directory_exists(const char *path){struct stat st;return path && stat(path,&st)==0 && S_ISDIR(st.st_mode);}
static int has_book_content(const char *path){DIR *dir=opendir(path);if(!dir)return 0;struct dirent *e;while((e=readdir(dir))!=NULL){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;char child[1024];snprintf(child,sizeof(child),"%s/%s",path,e->d_name);struct stat st;if(stat(child,&st)==0&&(S_ISDIR(st.st_mode)||S_ISREG(st.st_mode))){closedir(dir);return 1;}}closedir(dir);return 0;}
static void trim(char *s){if(!s)return;char *start=s;while(*start&&isspace((unsigned char)*start))start++;if(start!=s)memmove(s,start,strlen(start)+1);size_t len=strlen(s);while(len>0&&isspace((unsigned char)s[len-1]))s[--len]='\0';}
static void setup_config_path(void){if(config_path[0])return;char exe_path[1024];ssize_t len=readlink("/proc/self/exe",exe_path,sizeof(exe_path)-1);if(len>0){exe_path[len]='\0';char *slash=strrchr(exe_path,'/');if(slash){*slash='\0';snprintf(config_path,sizeof(config_path),"%s/%s",exe_path,CONFIG_FILE);return;}}snprintf(config_path,sizeof(config_path),"./%s",CONFIG_FILE);}
const char *get_storage_config_path(void){setup_config_path();return config_path;}
static int write_default_config(void){setup_config_path();FILE *fp=fopen(config_path,"w");if(!fp)return -1;fprintf(fp,"# Hoerspiel Player\n# Beliebig viele path= Eintraege sind moeglich.\n# Nicht vorhandene Pfade werden beim Start einfach ignoriert.\n\n[storage]\npath=/roms/hoerspiele\npath=/roms2/hoerspiele\npath=/mnt/usbdrive/hoerspiele\n");fclose(fp);return 0;}
static void make_label(const char *path,char *label,size_t size){snprintf(label,size,"%s",path);}
int get_storage_paths(StoragePath paths[],int max_paths){if(!paths||max_paths<=0)return 0;setup_config_path();FILE *fp=fopen(config_path,"r");if(!fp){write_default_config();fp=fopen(config_path,"r");}if(!fp)return 0;int count=0,in_storage=0;char line[1200];while(fgets(line,sizeof(line),fp)&&count<max_paths){trim(line);if(!line[0]||line[0]=='#'||line[0]==';')continue;if(line[0]=='['){in_storage=!strcmp(line,"[storage]");continue;}if(!in_storage)continue;if(!strncmp(line,"path=",5)){char *value=line+5;trim(value);if(!value[0])continue;snprintf(paths[count].path,sizeof(paths[count].path),"%s",value);make_label(value,paths[count].label,sizeof(paths[count].label));paths[count].available=directory_exists(value)&&has_book_content(value);count++;}}fclose(fp);return count;}
const char *get_audio_directory(void){static char audio_dir[STORAGE_PATH_LEN];StoragePath paths[MAX_STORAGE_PATHS];int count=get_storage_paths(paths,MAX_STORAGE_PATHS);for(int i=0;i<count;i++){if(paths[i].available){snprintf(audio_dir,sizeof(audio_dir),"%s",paths[i].path);return audio_dir;}}snprintf(audio_dir,sizeof(audio_dir),"%s","/roms/hoerspiele");return audio_dir;}
