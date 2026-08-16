#include "storage.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
static int write_default_config(void){setup_config_path();FILE *fp=fopen(config_path,"w");if(!fp)return -1;fprintf(fp,"# Hoerspiel Player\n# Beliebig viele path= Eintraege sind moeglich.\n# Nicht vorhandene Pfade werden beim Start einfach ignoriert.\n\n[storage]\npath=/roms/hoerspiele\npath=/roms2/hoerspiele\npath=/mnt/usbdrive/hoerspiele\n\n[hardware]\n# -1 = automatische Erkennung beim ersten Start\nled_gpio=-1\nled_gpio_mode=auto\n\n[playback]\nrepeat_book=0\nshutdown_after_tracks=0\nshutdown_at_book_end=0\n");fclose(fp);return 0;}
static void make_label(const char *path,char *label,size_t size){snprintf(label,size,"%s",path);}
int get_storage_paths(StoragePath paths[],int max_paths){if(!paths||max_paths<=0)return 0;setup_config_path();FILE *fp=fopen(config_path,"r");if(!fp){write_default_config();fp=fopen(config_path,"r");}if(!fp)return 0;int count=0,in_storage=0;char line[1200];while(fgets(line,sizeof(line),fp)&&count<max_paths){trim(line);if(!line[0]||line[0]=='#'||line[0]==';')continue;if(line[0]=='['){in_storage=!strcmp(line,"[storage]");continue;}if(!in_storage)continue;if(!strncmp(line,"path=",5)){char *value=line+5;trim(value);if(!value[0])continue;snprintf(paths[count].path,sizeof(paths[count].path),"%s",value);make_label(value,paths[count].label,sizeof(paths[count].label));paths[count].available=directory_exists(value)&&has_book_content(value);count++;}}fclose(fp);return count;}
const char *get_audio_directory(void){static char audio_dir[STORAGE_PATH_LEN];StoragePath paths[MAX_STORAGE_PATHS];int count=get_storage_paths(paths,MAX_STORAGE_PATHS);for(int i=0;i<count;i++){if(paths[i].available){snprintf(audio_dir,sizeof(audio_dir),"%s",paths[i].path);return audio_dir;}}snprintf(audio_dir,sizeof(audio_dir),"%s","/roms/hoerspiele");return audio_dir;}

int get_led_gpio_config(int *gpio,int *is_manual){FILE *fp;char line[1200];int in_hardware=0,found=0,value=-1,manual=0;setup_config_path();fp=fopen(config_path,"r");if(!fp)return 0;while(fgets(line,sizeof(line),fp)){trim(line);if(!line[0]||line[0]=='#'||line[0]==';')continue;if(line[0]=='['){in_hardware=!strcmp(line,"[hardware]");continue;}if(!in_hardware)continue;if(!strncmp(line,"led_gpio=",9)){char *v=line+9;trim(v);value=atoi(v);found=1;}else if(!strncmp(line,"led_gpio_mode=",14)){char *v=line+14;trim(v);manual=!strcasecmp(v,"manual");}}fclose(fp);if(gpio)*gpio=value;if(is_manual)*is_manual=manual;return found;}

int set_led_gpio_config(int gpio,int is_manual){FILE *fp;char **lines=NULL;size_t count=0,cap=0;char line[1200];int in_hardware=0,have_section=0,wrote_gpio=0,wrote_mode=0;setup_config_path();fp=fopen(config_path,"r");if(fp){while(fgets(line,sizeof(line),fp)){if(count==cap){size_t ncap=cap?cap*2:32;char **tmp=realloc(lines,ncap*sizeof(*tmp));if(!tmp){fclose(fp);goto fail;}lines=tmp;cap=ncap;}lines[count]=strdup(line);if(!lines[count]){fclose(fp);goto fail;}count++;}fclose(fp);}fp=fopen(config_path,"w");if(!fp)goto fail;for(size_t i=0;i<count;i++){char check[1200];snprintf(check,sizeof(check),"%s",lines[i]);trim(check);if(check[0]=='['){if(in_hardware){if(!wrote_gpio)fprintf(fp,"led_gpio=%d\n",gpio);if(!wrote_mode)fprintf(fp,"led_gpio_mode=%s\n",is_manual?"manual":"auto");}in_hardware=!strcmp(check,"[hardware]");if(in_hardware)have_section=1;fputs(lines[i],fp);continue;}if(in_hardware&&!strncmp(check,"led_gpio=",9)){fprintf(fp,"led_gpio=%d\n",gpio);wrote_gpio=1;continue;}if(in_hardware&&!strncmp(check,"led_gpio_mode=",14)){fprintf(fp,"led_gpio_mode=%s\n",is_manual?"manual":"auto");wrote_mode=1;continue;}fputs(lines[i],fp);}if(in_hardware){if(!wrote_gpio)fprintf(fp,"led_gpio=%d\n",gpio);if(!wrote_mode)fprintf(fp,"led_gpio_mode=%s\n",is_manual?"manual":"auto");}else if(!have_section){if(count>0&&lines[count-1][0]&&lines[count-1][strlen(lines[count-1])-1]!='\n')fputc('\n',fp);fprintf(fp,"\n[hardware]\nled_gpio=%d\nled_gpio_mode=%s\n",gpio,is_manual?"manual":"auto");}fclose(fp);for(size_t i=0;i<count;i++)free(lines[i]);free(lines);return 0;fail:for(size_t i=0;i<count;i++)free(lines[i]);free(lines);return -1;}

int repeat_book = 0;
int shutdown_after_tracks = 0;
int shutdown_at_book_end = 0;

void load_playback_config(void)
{
    FILE *fp;
    char line[1200];
    int in_playback = 0;

    repeat_book = 0;
    shutdown_after_tracks = 0;
    shutdown_at_book_end = 0;

    setup_config_path();
    fp = fopen(config_path, "r");
    if (!fp) return;

    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (!line[0] || line[0] == '#' || line[0] == ';') continue;
        if (line[0] == '[') {
            in_playback = !strcmp(line, "[playback]");
            continue;
        }
        if (!in_playback) continue;

        if (!strncmp(line, "repeat_book=", 12))
            repeat_book = atoi(line + 12) ? 1 : 0;
        else if (!strncmp(line, "shutdown_after_tracks=", 22)) {
            shutdown_after_tracks = atoi(line + 22);
            if (shutdown_after_tracks < 0) shutdown_after_tracks = 0;
            if (shutdown_after_tracks > 999) shutdown_after_tracks = 999;
        } else if (!strncmp(line, "shutdown_at_book_end=", 21))
            shutdown_at_book_end = atoi(line + 21) ? 1 : 0;
    }
    fclose(fp);
}

int save_playback_config(void)
{
    FILE *fp;
    char **lines = NULL;
    size_t count = 0, cap = 0;
    char line[1200];
    int in_section = 0, have_section = 0;
    int wrote_repeat = 0, wrote_tracks = 0, wrote_end = 0;

    setup_config_path();
    fp = fopen(config_path, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (count == cap) {
                size_t ncap = cap ? cap * 2 : 32;
                char **tmp = realloc(lines, ncap * sizeof(*tmp));
                if (!tmp) { fclose(fp); goto fail; }
                lines = tmp;
                cap = ncap;
            }
            lines[count] = strdup(line);
            if (!lines[count]) { fclose(fp); goto fail; }
            count++;
        }
        fclose(fp);
    }

    fp = fopen(config_path, "w");
    if (!fp) goto fail;

    for (size_t i = 0; i < count; i++) {
        char check[1200];
        snprintf(check, sizeof(check), "%s", lines[i]);
        trim(check);

        if (check[0] == '[') {
            if (in_section) {
                if (!wrote_repeat) fprintf(fp, "repeat_book=%d\n", repeat_book ? 1 : 0);
                if (!wrote_tracks) fprintf(fp, "shutdown_after_tracks=%d\n", shutdown_after_tracks);
                if (!wrote_end) fprintf(fp, "shutdown_at_book_end=%d\n", shutdown_at_book_end ? 1 : 0);
            }
            in_section = !strcmp(check, "[playback]");
            if (in_section) have_section = 1;
            fputs(lines[i], fp);
            continue;
        }

        if (in_section && !strncmp(check, "repeat_book=", 12)) {
            fprintf(fp, "repeat_book=%d\n", repeat_book ? 1 : 0);
            wrote_repeat = 1;
            continue;
        }
        if (in_section && !strncmp(check, "shutdown_after_tracks=", 22)) {
            fprintf(fp, "shutdown_after_tracks=%d\n", shutdown_after_tracks);
            wrote_tracks = 1;
            continue;
        }
        if (in_section && !strncmp(check, "shutdown_at_book_end=", 21)) {
            fprintf(fp, "shutdown_at_book_end=%d\n", shutdown_at_book_end ? 1 : 0);
            wrote_end = 1;
            continue;
        }
        fputs(lines[i], fp);
    }

    if (in_section) {
        if (!wrote_repeat) fprintf(fp, "repeat_book=%d\n", repeat_book ? 1 : 0);
        if (!wrote_tracks) fprintf(fp, "shutdown_after_tracks=%d\n", shutdown_after_tracks);
        if (!wrote_end) fprintf(fp, "shutdown_at_book_end=%d\n", shutdown_at_book_end ? 1 : 0);
    } else if (!have_section) {
        if (count > 0 && lines[count-1][0] &&
            lines[count-1][strlen(lines[count-1])-1] != '\n')
            fputc('\n', fp);
        fprintf(fp,
                "\n[playback]\nrepeat_book=%d\nshutdown_after_tracks=%d\nshutdown_at_book_end=%d\n",
                repeat_book ? 1 : 0, shutdown_after_tracks,
                shutdown_at_book_end ? 1 : 0);
    }

    if (fflush(fp) != 0 || fsync(fileno(fp)) != 0) {
        fclose(fp);
        goto fail;
    }
    if (fclose(fp) != 0) goto fail;
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return 0;

fail:
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return -1;
}
