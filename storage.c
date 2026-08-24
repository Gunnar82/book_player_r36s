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

static int copy_checked(char *dst,size_t dst_size,const char *src){
    if(!dst||dst_size==0)return -1;
    if(!src){dst[0]='\0';return 0;}
    size_t n=strlen(src);
    if(n>=dst_size){
        dst[0]='\0';
        return -1;
    }
    memcpy(dst,src,n+1);
    return 0;
}

static int join_path_checked(char *dst,size_t dst_size,const char *a,const char *b){
    if(!dst||dst_size==0||!a||!b)return -1;
    size_t al=strlen(a),bl=strlen(b);
    size_t sep=(al>0&&a[al-1]!='/')?1:0;
    if(al+sep+bl+1>dst_size){
        dst[0]='\0';
        return -1;
    }
    memcpy(dst,a,al);
    size_t pos=al;
    if(sep)dst[pos++]='/';
    memcpy(dst+pos,b,bl);
    dst[pos+bl]='\0';
    return 0;
}

static int directory_exists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}
static int has_book_content(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
        return 0;
    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
            continue;
        char child[1024];
        if(join_path_checked(child,sizeof(child),path,e->d_name)!=0)continue;
        struct stat st;
        if (stat(child, &st) == 0) {
            if (S_ISDIR(st.st_mode) || S_ISREG(st.st_mode)) {
                closedir(dir);
                return 1;
            }
        }
    }
    closedir(dir);
    return 0;
}
static void trim(char *s)
{
    if (!s) return;
    char *start = s;
    while (*start && isspace((unsigned char)*start))
        start++;
    if (start != s)
        memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
}
static void setup_config_path(void)
{
    if (config_path[0])
        return;
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
        char *slash = strrchr(exe_path, '/');
        if (slash) {
            *slash = '\0';
            if(join_path_checked(config_path,sizeof(config_path),exe_path,CONFIG_FILE)!=0)config_path[0]='\0';
            return;
        }
    }
    snprintf(config_path, sizeof(config_path), "./%s", CONFIG_FILE);
}
const char *get_storage_config_path(void)
{
    setup_config_path();
    return config_path;
}
static int write_default_config(void)
{
    setup_config_path();
    FILE *fp = fopen(config_path, "w");
    if (!fp)
        return -1;
    fprintf(fp,
        "# Hoerspiel Player\n"
        "# Beliebig viele path= Eintraege sind moeglich.\n"
        "# Nicht vorhandene Pfade werden beim Start einfach ignoriert.\n"
        "\n"
        "[storage]\n"
        "path=/roms/hoerspiele\n"
        "path=/roms2/hoerspiele\n"
        "path=/mnt/usbdrive/hoerspiele\n"
        "\n"
        "[hardware]\n"
        "# -1 = automatische Erkennung beim ersten Start\n"
        "led_gpio=-1\n"
        "led_gpio_mode=auto\n\n"
        "[playback]\n"
        "repeat_book=0\n\n"
        "[download]\n"
        "enabled=0\n"
        "base_url=\n"
        "target_path=/roms/hoerspiele\n"
        "verify_peer=1\n"
        "verify_host=1\n"
        "ca_cert=\n"
        "client_cert=\n"
        "client_key=\n"
        "client_key_password=\n"
        "\n"
        "[streams]\n"
        "xml_url=\n"
        "client_cert_mode=none\n"
        "ca_cert=\n"
        "client_cert=\n"
        "client_key=\n"
        "client_key_password=\n"
        "\n"
        "[input]\n"
        "# R36S-Standard. Fuer andere Controller profile=custom setzen.\n"
        "profile=r36s\n"
        "dpad_mode=buttons\n");
    fclose(fp);
    return 0;
}
static void make_label(const char *path, char *label, size_t size)
{
    if(copy_checked(label,size,path)!=0 && size>0)label[size-1]='\0';
}
int get_storage_paths(StoragePath paths[], int max_paths)
{
    if (!paths || max_paths <= 0)
        return 0;
    setup_config_path();
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        write_default_config();
        fp = fopen(config_path, "r");
    }
    if (!fp)
        return 0;
    int count = 0;
    int in_storage = 0;
    char line[1200];
    while (fgets(line, sizeof(line), fp) && count < max_paths) {
        trim(line);
        if (!line[0] || line[0] == '#' || line[0] == ';')
            continue;
        if (line[0] == '[') {
            in_storage = !strcmp(line, "[storage]");
            continue;
        }
        if (!in_storage)
            continue;
        if (!strncmp(line, "path=", 5)) {
            char *value = line + 5;
            trim(value);
            if (!value[0])
                continue;
            if(copy_checked(paths[count].path,sizeof(paths[count].path),value)!=0)continue;
            make_label(value, paths[count].label, sizeof(paths[count].label));
            paths[count].available = directory_exists(value) && has_book_content(value);
            count++;
        }
    }
    fclose(fp);
    return count;
}
const char *get_audio_directory(void)
{
    static char audio_dir[STORAGE_PATH_LEN];
    StoragePath paths[MAX_STORAGE_PATHS];
    int count = get_storage_paths(paths, MAX_STORAGE_PATHS);
    for (int i = 0; i < count; i++) {
        if (paths[i].available) {
            snprintf(audio_dir, sizeof(audio_dir), "%s", paths[i].path);
            return audio_dir;
        }
    }
    snprintf(audio_dir, sizeof(audio_dir), "%s", "/roms/hoerspiele");
    return audio_dir;
}
int get_led_gpio_config(int *gpio,int *is_manual){
    FILE *fp;
    char line[1200];
    int in_hardware=0,found=0,value=-1,manual=0;
    setup_config_path();
    fp=fopen(config_path,"r");
    if(!fp)return 0;
    while(fgets(line,sizeof(line),fp)){
        trim(line);
        if(!line[0]||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){in_hardware=!strcmp(line,"[hardware]");continue;}
        if(!in_hardware)continue;
        if(!strncmp(line,"led_gpio=",9)){
            char *v=line+9;trim(v);value=atoi(v);found=1;
        }else if(!strncmp(line,"led_gpio_mode=",14)){
            char *v=line+14;trim(v);manual=!strcasecmp(v,"manual");
        }
    }
    fclose(fp);
    if(gpio)*gpio=value;
    if(is_manual)*is_manual=manual;
    return found;
}
int set_led_gpio_config(int gpio,int is_manual){
    FILE *fp;
    char **lines=NULL;
    size_t count=0,cap=0;
    char line[1200];
    int in_hardware=0,have_section=0,wrote_gpio=0,wrote_mode=0;
    setup_config_path();
    fp=fopen(config_path,"r");
    if(fp){
        while(fgets(line,sizeof(line),fp)){
            if(count==cap){size_t ncap=cap?cap*2:32;char **tmp=realloc(lines,ncap*sizeof(*tmp));if(!tmp){fclose(fp);goto fail;}lines=tmp;cap=ncap;}
            lines[count]=strdup(line);if(!lines[count]){fclose(fp);goto fail;}count++;
        }
        fclose(fp);
    }
    fp=fopen(config_path,"w");
    if(!fp)goto fail;
    for(size_t i=0;i<count;i++){
        char check[1200];
        snprintf(check,sizeof(check),"%s",lines[i]);trim(check);
        if(check[0]=='['){
            if(in_hardware){
                if(!wrote_gpio)fprintf(fp,"led_gpio=%d\n",gpio);
                if(!wrote_mode)fprintf(fp,"led_gpio_mode=%s\n",is_manual?"manual":"auto");
            }
            in_hardware=!strcmp(check,"[hardware]");
            if(in_hardware)have_section=1;
            fputs(lines[i],fp);
            continue;
        }
        if(in_hardware&&!strncmp(check,"led_gpio=",9)){
            fprintf(fp,"led_gpio=%d\n",gpio);wrote_gpio=1;continue;
        }
        if(in_hardware&&!strncmp(check,"led_gpio_mode=",14)){
            fprintf(fp,"led_gpio_mode=%s\n",is_manual?"manual":"auto");wrote_mode=1;continue;
        }
        fputs(lines[i],fp);
    }
    if(in_hardware){
        if(!wrote_gpio)fprintf(fp,"led_gpio=%d\n",gpio);
        if(!wrote_mode)fprintf(fp,"led_gpio_mode=%s\n",is_manual?"manual":"auto");
    }else if(!have_section){
        if(count>0&&lines[count-1][0]&&lines[count-1][strlen(lines[count-1])-1]!='\n')fputc('\n',fp);
        fprintf(fp,"\n[hardware]\nled_gpio=%d\nled_gpio_mode=%s\n",gpio,is_manual?"manual":"auto");
    }
    fclose(fp);
    for(size_t i=0;i<count;i++) free(lines[i]);
    free(lines);
    return 0;
fail:
    for(size_t i=0;i<count;i++) free(lines[i]);
    free(lines);
    return -1;
}

int load_ui_config(int *out_volume,int *out_idle,int *out_display,int *out_font)
{
    setup_config_path();
    FILE *fp=fopen(config_path,"r");
    if(!fp)return 0;
    char line[1200];int in_ui=0,found=0;
    while(fgets(line,sizeof(line),fp)){
        trim(line);
        if(!line[0]||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){in_ui=!strcmp(line,"[ui]");continue;}
        if(!in_ui)continue;
        char *eq=strchr(line,'=');if(!eq)continue;*eq++='\0';trim(line);trim(eq);
        if(!strcmp(line,"volume")){if(out_volume)*out_volume=atoi(eq);found=1;}
        else if(!strcmp(line,"idle_timer_minutes")){if(out_idle)*out_idle=atoi(eq);found=1;}
        else if(!strcmp(line,"display_timeout_seconds")){if(out_display)*out_display=atoi(eq);found=1;}
        else if(!strcmp(line,"menu_font_size")){if(out_font)*out_font=atoi(eq);found=1;}
    }
    fclose(fp);return found;
}
int save_ui_config(int v,int idle,int display,int font)
{
    setup_config_path();
    FILE *fp=fopen(config_path,"r");char **lines=NULL;size_t count=0,cap=0;char line[1200];
    if(fp){while(fgets(line,sizeof(line),fp)){if(count==cap){size_t nc=cap?cap*2:32;char **tmp=realloc(lines,nc*sizeof(*tmp));if(!tmp){fclose(fp);goto fail;}lines=tmp;cap=nc;}lines[count]=strdup(line);if(!lines[count]){fclose(fp);goto fail;}count++;}fclose(fp);}
    fp=fopen(config_path,"w");if(!fp)goto fail;
    int in=0,have=0,wv=0,wi=0,wd=0,wf=0;
    for(size_t i=0;i<count;i++){
        char check[1200];snprintf(check,sizeof(check),"%s",lines[i]);trim(check);
        if(check[0]=='['){
            if(in){if(!wv)fprintf(fp,"volume=%d\n",v);if(!wi)fprintf(fp,"idle_timer_minutes=%d\n",idle);if(!wd)fprintf(fp,"display_timeout_seconds=%d\n",display);if(!wf)fprintf(fp,"menu_font_size=%d\n",font);}
            in=!strcmp(check,"[ui]");if(in)have=1;fputs(lines[i],fp);continue;
        }
        if(in&&!strncmp(check,"volume=",7)){fprintf(fp,"volume=%d\n",v);wv=1;continue;}
        if(in&&!strncmp(check,"idle_timer_minutes=",19)){fprintf(fp,"idle_timer_minutes=%d\n",idle);wi=1;continue;}
        if(in&&!strncmp(check,"display_timeout_seconds=",24)){fprintf(fp,"display_timeout_seconds=%d\n",display);wd=1;continue;}
        if(in&&!strncmp(check,"menu_font_size=",15)){fprintf(fp,"menu_font_size=%d\n",font);wf=1;continue;}
        fputs(lines[i],fp);
    }
    if(in){if(!wv)fprintf(fp,"volume=%d\n",v);if(!wi)fprintf(fp,"idle_timer_minutes=%d\n",idle);if(!wd)fprintf(fp,"display_timeout_seconds=%d\n",display);if(!wf)fprintf(fp,"menu_font_size=%d\n",font);}
    else if(!have){if(count&&lines[count-1][0]&&lines[count-1][strlen(lines[count-1])-1]!='\n')fputc('\n',fp);fprintf(fp,"\n[ui]\nvolume=%d\nidle_timer_minutes=%d\ndisplay_timeout_seconds=%d\nmenu_font_size=%d\n",v,idle,display,font);}
    if(fflush(fp)!=0||fsync(fileno(fp))!=0){fclose(fp);goto fail;}if(fclose(fp)!=0)goto fail;
    for(size_t i=0;i<count;i++)
        free(lines[i]);
    free(lines);
    return 0;
fail:
    for(size_t i=0;i<count;i++)
        free(lines[i]);
    free(lines);
    return -1;
}

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
        if (line[0] == '[') { in_playback = !strcmp(line, "[playback]"); continue; }
        if (in_playback && !strncmp(line, "repeat_book=", 12))
            repeat_book = atoi(line + 12) ? 1 : 0;
        else if (in_playback && !strncmp(line, "shutdown_after_tracks=", 22))
            shutdown_after_tracks = atoi(line + 22);
        else if (in_playback && !strncmp(line, "shutdown_at_book_end=", 21))
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
    int in_section = 0, have_section = 0, wrote_repeat = 0, wrote_tracks = 0, wrote_end = 0;
    setup_config_path();
    fp = fopen(config_path, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (count == cap) {
                size_t ncap = cap ? cap * 2 : 32;
                char **tmp = realloc(lines, ncap * sizeof(*tmp));
                if (!tmp) { fclose(fp); goto fail; }
                lines = tmp; cap = ncap;
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
            fprintf(fp, "shutdown_after_tracks=%d\n", shutdown_after_tracks); wrote_tracks = 1; continue;
        }
        if (in_section && !strncmp(check, "shutdown_at_book_end=", 21)) {
            fprintf(fp, "shutdown_at_book_end=%d\n", shutdown_at_book_end ? 1 : 0); wrote_end = 1; continue;
        }
        fputs(lines[i], fp);
    }
    if (in_section) {
        if (!wrote_repeat) fprintf(fp, "repeat_book=%d\n", repeat_book ? 1 : 0);
        if (!wrote_tracks) fprintf(fp, "shutdown_after_tracks=%d\n", shutdown_after_tracks);
        if (!wrote_end) fprintf(fp, "shutdown_at_book_end=%d\n", shutdown_at_book_end ? 1 : 0);
    } else if (!have_section) {
        if (count > 0 && lines[count-1][0] && lines[count-1][strlen(lines[count-1])-1] != '\n') fputc('\n', fp);
        fprintf(fp, "\n[playback]\nrepeat_book=%d\nshutdown_after_tracks=%d\nshutdown_at_book_end=%d\n", repeat_book ? 1 : 0, shutdown_after_tracks, shutdown_at_book_end ? 1 : 0);
    }
    if (fflush(fp) != 0 || fsync(fileno(fp)) != 0) { fclose(fp); goto fail; }
    if (fclose(fp) != 0) goto fail;
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return 0;
fail:
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return -1;
}

int downloads_enabled = 0;
int download_verify_peer = 1;
int download_verify_host = 1;
char download_base_url[DOWNLOAD_URL_LEN] = "";
char download_target_path[STORAGE_PATH_LEN] = "/roms/hoerspiele";
char download_ca_cert[STORAGE_PATH_LEN] = "";
char download_client_cert[STORAGE_PATH_LEN] = "";
char download_client_key[STORAGE_PATH_LEN] = "";
char download_client_key_password[256] = "";

void load_download_config(void)
{
    FILE *fp;
    char line[1400];
    int in_download = 0;

    downloads_enabled = 0;
    download_verify_peer = 1;
    download_verify_host = 1;
    download_base_url[0] = '\0';
    snprintf(download_target_path, sizeof(download_target_path), "%s", "/roms/hoerspiele");
    download_ca_cert[0] = '\0';
    download_client_cert[0] = '\0';
    download_client_key[0] = '\0';
    download_client_key_password[0] = '\0';

    setup_config_path();
    fp = fopen(config_path, "r");
    if (!fp) return;

    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (!line[0] || line[0] == '#' || line[0] == ';') continue;
        if (line[0] == '[') {
            in_download = !strcmp(line, "[download]");
            continue;
        }
        if (!in_download) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq++ = '\0';
        trim(line);
        trim(eq);

        if (!strcmp(line, "enabled")) downloads_enabled = atoi(eq) ? 1 : 0;
        else if (!strcmp(line, "base_url")) snprintf(download_base_url, sizeof(download_base_url), "%s", eq);
        else if (!strcmp(line, "target_path")) snprintf(download_target_path, sizeof(download_target_path), "%s", eq);
        else if (!strcmp(line, "verify_peer")) download_verify_peer = atoi(eq) ? 1 : 0;
        else if (!strcmp(line, "verify_host")) download_verify_host = atoi(eq) ? 1 : 0;
        else if (!strcmp(line, "ca_cert")) snprintf(download_ca_cert, sizeof(download_ca_cert), "%s", eq);
        else if (!strcmp(line, "client_cert")) snprintf(download_client_cert, sizeof(download_client_cert), "%s", eq);
        else if (!strcmp(line, "client_key")) snprintf(download_client_key, sizeof(download_client_key), "%s", eq);
        else if (!strcmp(line, "client_key_password")) snprintf(download_client_key_password, sizeof(download_client_key_password), "%s", eq);
    }
    fclose(fp);
}

int save_download_enabled(void)
{
    FILE *fp;
    char **lines = NULL;
    size_t count = 0, cap = 0;
    char line[1400];
    int in_section = 0, have_section = 0, wrote_enabled = 0;

    setup_config_path();
    fp = fopen(config_path, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (count == cap) {
                size_t ncap = cap ? cap * 2 : 32;
                char **tmp = realloc(lines, ncap * sizeof(*tmp));
                if (!tmp) { fclose(fp); goto fail; }
                lines = tmp; cap = ncap;
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
        char check[1400];
        snprintf(check, sizeof(check), "%s", lines[i]);
        trim(check);

        if (check[0] == '[') {
            if (in_section && !wrote_enabled)
                fprintf(fp, "enabled=%d\n", downloads_enabled ? 1 : 0);
            in_section = !strcmp(check, "[download]");
            if (in_section) have_section = 1;
            fputs(lines[i], fp);
            continue;
        }

        if (in_section && !strncmp(check, "enabled=", 8)) {
            fprintf(fp, "enabled=%d\n", downloads_enabled ? 1 : 0);
            wrote_enabled = 1;
            continue;
        }
        fputs(lines[i], fp);
    }

    if (in_section) {
        if (!wrote_enabled) fprintf(fp, "enabled=%d\n", downloads_enabled ? 1 : 0);
    } else if (!have_section) {
        if (count > 0 && lines[count-1][0] && lines[count-1][strlen(lines[count-1])-1] != '\n') fputc('\n', fp);
        fprintf(fp,
                "\n[download]\nenabled=%d\nbase_url=\ntarget_path=/roms/hoerspiele\n"
                "verify_peer=1\nverify_host=1\nca_cert=\nclient_cert=\nclient_key=\nclient_key_password=\n",
                downloads_enabled ? 1 : 0);
    }

    if (fflush(fp) != 0 || fsync(fileno(fp)) != 0) { fclose(fp); goto fail; }
    if (fclose(fp) != 0) goto fail;
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return 0;

fail:
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return -1;
}
