#include "streaming.h"
#include "storage.h"
#include <strings.h>
#include <curl/curl.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

char stream_xml_url[STREAM_URL_LEN]="";
int stream_cert_mode=STREAM_CERT_NONE;
char stream_ca_cert[STREAM_CERT_PATH_LEN]="";
char stream_client_cert[STREAM_CERT_PATH_LEN]="";
char stream_client_key[STREAM_CERT_PATH_LEN]="";
char stream_client_key_password[256]="";
static pid_t ffmpeg_pid=-1;
static int ffmpeg_paused=0;
static char current_name[STREAM_NAME_LEN]="";

typedef struct { char *data; size_t size; } Buffer;

static size_t write_cb(void *ptr,size_t size,size_t nmemb,void *userdata){
    Buffer *b=(Buffer*)userdata; size_t n=size*nmemb;
    char *p=realloc(b->data,b->size+n+1); if(!p)return 0;
    b->data=p; memcpy(b->data+b->size,ptr,n); b->size+=n; b->data[b->size]='\0'; return n;
}
static void trim(char *s){
    char *p=s; while(*p&&isspace((unsigned char)*p))p++;
    if(p!=s)memmove(s,p,strlen(p)+1);
    size_t n=strlen(s); while(n&&isspace((unsigned char)s[n-1]))s[--n]='\0';
}
static int xml_entity_decode(const char *src,char *dst,size_t dst_size){
    size_t o=0;
    for(size_t i=0;src[i]&&o+1<dst_size;){
        if(src[i]=='&'){
            if(!strncmp(src+i,"&amp;",5)){dst[o++]='&';i+=5;continue;}
            if(!strncmp(src+i,"&quot;",6)){dst[o++]='"';i+=6;continue;}
            if(!strncmp(src+i,"&apos;",6)){dst[o++]='\'';i+=6;continue;}
            if(!strncmp(src+i,"&lt;",4)){dst[o++]='<';i+=4;continue;}
            if(!strncmp(src+i,"&gt;",4)){dst[o++]='>';i+=4;continue;}
        }
        dst[o++]=src[i++];
    }
    dst[o]='\0';
    return 1;
}

static int attr(const char *station,const char *name,char *out,size_t out_size){
    char needle[96];
    snprintf(needle,sizeof(needle),"%s=\"",name);
    const char *p=strstr(station,needle);
    if(!p)return 0;
    p+=strlen(needle);
    const char *e=strchr(p,'"');
    if(!e)return 0;
    size_t n=(size_t)(e-p);
    char *tmp=malloc(n+1);
    if(!tmp)return 0;
    memcpy(tmp,p,n);
    tmp[n]='\0';
    xml_entity_decode(tmp,out,out_size);
    free(tmp);
    trim(out);
    return 1;
}

static int starts_http(const char *s){
    return s && (!strncmp(s,"http://",7) || !strncmp(s,"https://",8));
}

static void curl_apply_stream_tls(CURL *c){
    const char *ca="",*cert="",*key="",*pass="";
    long verify_peer=1,verify_host=2;
    if(stream_cert_mode==STREAM_CERT_DOWNLOADS){
        ca=download_ca_cert; cert=download_client_cert; key=download_client_key; pass=download_client_key_password;
        verify_peer=download_verify_peer?1L:0L; verify_host=download_verify_host?2L:0L;
    }else if(stream_cert_mode==STREAM_CERT_SEPARATE){
        ca=stream_ca_cert; cert=stream_client_cert; key=stream_client_key; pass=stream_client_key_password;
    }
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,verify_peer);
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,verify_host);
    if(ca&&ca[0])curl_easy_setopt(c,CURLOPT_CAINFO,ca);
    if(cert&&cert[0])curl_easy_setopt(c,CURLOPT_SSLCERT,cert);
    if(key&&key[0])curl_easy_setopt(c,CURLOPT_SSLKEY,key);
    if(pass&&pass[0])curl_easy_setopt(c,CURLOPT_KEYPASSWD,pass);
}

static int load_local_file(const char *path,Buffer *b){
    FILE *fp=fopen(path,"rb"); if(!fp)return -1;
    if(fseek(fp,0,SEEK_END)!=0){fclose(fp);return -1;}
    long len=ftell(fp); if(len<0){fclose(fp);return -1;}
    rewind(fp);
    b->data=malloc((size_t)len+1); if(!b->data){fclose(fp);return -1;}
    size_t got=fread(b->data,1,(size_t)len,fp); fclose(fp);
    b->data[got]='\0'; b->size=got; return 0;
}

const char *streaming_cert_mode_name(void){
    if(stream_cert_mode==STREAM_CERT_DOWNLOADS)return "Downloads";
    if(stream_cert_mode==STREAM_CERT_SEPARATE)return "Separat";
    return "Keins";
}

int streaming_save_cert_mode(void){
    const char *config_path=get_storage_config_path();
    FILE *fp=fopen(config_path,"r");
    char *data=NULL; size_t size=0;
    if(fp){
        fseek(fp,0,SEEK_END); long n=ftell(fp); rewind(fp);
        if(n>=0){data=malloc((size_t)n+1); if(data){size=fread(data,1,(size_t)n,fp); data[size]='\0';}}
        fclose(fp);
    }
    char tmp_path[1200];
    snprintf(tmp_path,sizeof(tmp_path),"%s.tmp",config_path);
    FILE *out=fopen(tmp_path,"w"); if(!out){free(data);return -1;}
    int in_streams=0,wrote=0,have_streams=0;
    if(data){
        char *save=NULL;
        for(char *line=strtok_r(data,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){
            char check[512];snprintf(check,sizeof(check),"%s",line);trim(check);
            if(check[0]=='['){
                if(in_streams&&!wrote){fprintf(out,"client_cert_mode=%s\n",streaming_cert_mode_name());wrote=1;}
                in_streams=!strcmp(check,"[streams]"); if(in_streams)have_streams=1;
            }
            if(in_streams && !strncmp(check,"client_cert_mode=",17)){
                fprintf(out,"client_cert_mode=%s\n",streaming_cert_mode_name()); wrote=1; continue;
            }
            fprintf(out,"%s\n",line);
        }
    }
    if(in_streams&&!wrote)fprintf(out,"client_cert_mode=%s\n",streaming_cert_mode_name());
    if(!have_streams)fprintf(out,"\n[streams]\nxml_url=\nclient_cert_mode=%s\nca_cert=\nclient_cert=\nclient_key=\nclient_key_password=\n",streaming_cert_mode_name());
    fclose(out); free(data);
    return rename(tmp_path,config_path);
}

#define FAVORITES_FILE "stream_favorites.txt"
int streaming_favorite_is_set(const char *uuid){
    if(!uuid||!uuid[0])return 0;
    FILE *fp=fopen(FAVORITES_FILE,"r"); if(!fp)return 0;
    char line[128]; int found=0;
    while(fgets(line,sizeof(line),fp)){trim(line);if(!strcmp(line,uuid)){found=1;break;}}
    fclose(fp); return found;
}
int streaming_favorite_toggle(const char *uuid){
    if(!uuid||!uuid[0])return -1;
    FILE *fp=fopen(FAVORITES_FILE,"r");
    char **items=NULL; int count=0,exists=0; char line[128];
    if(fp){
        while(fgets(line,sizeof(line),fp)){
            trim(line); if(!line[0])continue;
            if(!strcmp(line,uuid)){exists=1;continue;}
            char **tmp=realloc(items,(size_t)(count+1)*sizeof(char*)); if(!tmp)break;
            items=tmp; items[count]=strdup(line); if(items[count])count++;
        }
        fclose(fp);
    }
    fp=fopen(FAVORITES_FILE,"w");
    if(!fp){for(int i=0;i<count;i++)free(items[i]);free(items);return -1;}
    for(int i=0;i<count;i++){fprintf(fp,"%s\n",items[i]);free(items[i]);}
    if(!exists)fprintf(fp,"%s\n",uuid);
    fclose(fp);free(items);return exists?0:1;
}

static void streaming_ensure_config_section(void){
    const char *path=get_storage_config_path();
    FILE *fp=fopen(path,"r");
    if(!fp){
        fp=fopen(path,"w");
        if(!fp)return;
        fprintf(fp,"[streams]\n");
        fprintf(fp,"xml_url=\n");
        fprintf(fp,"client_cert_mode=none\n");
        fprintf(fp,"ca_cert=\n");
        fprintf(fp,"client_cert=\n");
        fprintf(fp,"client_key=\n");
        fprintf(fp,"client_key_password=\n");
        fclose(fp);
        return;
    }

    char **lines=NULL;
    int line_count=0;
    char line[4096];

    while(fgets(line,sizeof(line),fp)){
        char **tmp=realloc(lines,(size_t)(line_count+1)*sizeof(char*));
        if(!tmp)break;
        lines=tmp;
        lines[line_count]=strdup(line);
        if(!lines[line_count])break;
        line_count++;
    }
    fclose(fp);

    int section_start=-1;
    int section_end=line_count;
    int have_xml=0,have_mode=0,have_ca=0,have_cert=0,have_key=0,have_pass=0;

    for(int i=0;i<line_count;i++){
        char check[4096];
        snprintf(check,sizeof(check),"%s",lines[i]);
        trim(check);

        if(check[0]=='['){
            if(section_start>=0){
                section_end=i;
                break;
            }
            if(!strcmp(check,"[streams]"))
                section_start=i;
            continue;
        }

        if(section_start>=0){
            char *eq=strchr(check,'=');
            if(!eq)continue;
            *eq='\0';
            trim(check);

            if(!strcmp(check,"xml_url"))have_xml=1;
            else if(!strcmp(check,"client_cert_mode"))have_mode=1;
            else if(!strcmp(check,"ca_cert"))have_ca=1;
            else if(!strcmp(check,"client_cert"))have_cert=1;
            else if(!strcmp(check,"client_key"))have_key=1;
            else if(!strcmp(check,"client_key_password"))have_pass=1;
        }
    }

    int complete=(section_start>=0&&have_xml&&have_mode&&have_ca&&have_cert&&have_key&&have_pass);
    if(complete){
        for(int i=0;i<line_count;i++)free(lines[i]);
        free(lines);
        return;
    }

    char tmp_path[1200];
    snprintf(tmp_path,sizeof(tmp_path),"%s.tmp",path);
    FILE *out=fopen(tmp_path,"w");
    if(!out){
        for(int i=0;i<line_count;i++)free(lines[i]);
        free(lines);
        return;
    }

    if(section_start<0){
        for(int i=0;i<line_count;i++)fputs(lines[i],out);
        if(line_count>0 && lines[line_count-1][strlen(lines[line_count-1])-1]!='\n')
            fputc('\n',out);
        fprintf(out,"\n[streams]\n");
        fprintf(out,"xml_url=\n");
        fprintf(out,"client_cert_mode=none\n");
        fprintf(out,"ca_cert=\n");
        fprintf(out,"client_cert=\n");
        fprintf(out,"client_key=\n");
        fprintf(out,"client_key_password=\n");
    }else{
        for(int i=0;i<line_count;i++){
            if(i==section_end){
                if(!have_xml)fprintf(out,"xml_url=\n");
                if(!have_mode)fprintf(out,"client_cert_mode=none\n");
                if(!have_ca)fprintf(out,"ca_cert=\n");
                if(!have_cert)fprintf(out,"client_cert=\n");
                if(!have_key)fprintf(out,"client_key=\n");
                if(!have_pass)fprintf(out,"client_key_password=\n");
            }
            fputs(lines[i],out);
        }

        if(section_end==line_count){
            if(!have_xml)fprintf(out,"xml_url=\n");
            if(!have_mode)fprintf(out,"client_cert_mode=none\n");
            if(!have_ca)fprintf(out,"ca_cert=\n");
            if(!have_cert)fprintf(out,"client_cert=\n");
            if(!have_key)fprintf(out,"client_key=\n");
            if(!have_pass)fprintf(out,"client_key_password=\n");
        }
    }

    fclose(out);

    for(int i=0;i<line_count;i++)free(lines[i]);
    free(lines);

    rename(tmp_path,path);
}

void streaming_load_config(void){
    streaming_ensure_config_section();
    stream_xml_url[0]='\0'; stream_cert_mode=STREAM_CERT_NONE;
    stream_ca_cert[0]='\0'; stream_client_cert[0]='\0'; stream_client_key[0]='\0'; stream_client_key_password[0]='\0';
    const char *config_path=get_storage_config_path();
    FILE *fp=fopen(config_path,"r"); if(!fp)return;
    char line[4096],section[64]="";
    while(fgets(line,sizeof(line),fp)){
        trim(line); if(!line[0]||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){char *e=strchr(line,']');if(e){*e='\0';snprintf(section,sizeof(section),"%s",line+1);}continue;}
        if(strcmp(section,"streams"))continue;
        char *eq=strchr(line,'='); if(!eq)continue; *eq++='\0'; trim(line);trim(eq);
        if(!strcmp(line,"xml_url"))snprintf(stream_xml_url,sizeof(stream_xml_url),"%s",eq);
        else if(!strcmp(line,"client_cert_mode")){
            if(!strcasecmp(eq,"downloads"))stream_cert_mode=STREAM_CERT_DOWNLOADS;
            else if(!strcasecmp(eq,"separate")||!strcasecmp(eq,"separat"))stream_cert_mode=STREAM_CERT_SEPARATE;
            else stream_cert_mode=STREAM_CERT_NONE;
        }else if(!strcmp(line,"ca_cert"))snprintf(stream_ca_cert,sizeof(stream_ca_cert),"%s",eq);
        else if(!strcmp(line,"client_cert"))snprintf(stream_client_cert,sizeof(stream_client_cert),"%s",eq);
        else if(!strcmp(line,"client_key"))snprintf(stream_client_key,sizeof(stream_client_key),"%s",eq);
        else if(!strcmp(line,"client_key_password"))snprintf(stream_client_key_password,sizeof(stream_client_key_password),"%s",eq);
    }
    fclose(fp);
}
int streaming_fetch_xml(StreamEntry **entries,int *count_out,char *err,size_t err_size){
    if(err&&err_size)err[0]='\0';
    if(!stream_xml_url[0]){if(err&&err_size)snprintf(err,err_size,"streams.xml_url fehlt");return -1;}
    Buffer b={0};
    if(starts_http(stream_xml_url)){
        CURL *c=curl_easy_init(); if(!c)return -1;
        curl_easy_setopt(c,CURLOPT_URL,stream_xml_url);
        curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);
        curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,write_cb);
        curl_easy_setopt(c,CURLOPT_WRITEDATA,&b);
        curl_easy_setopt(c,CURLOPT_TIMEOUT,20L);
        if(!strncmp(stream_xml_url,"https://",8))curl_apply_stream_tls(c);
        CURLcode rc=curl_easy_perform(c); long code=0;
        curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&code); curl_easy_cleanup(c);
        if(rc!=CURLE_OK||code>=400){
            if(err&&err_size)snprintf(err,err_size,"XML Abruf fehlgeschlagen (%ld): %s",code,curl_easy_strerror(rc));
            free(b.data);return -1;
        }
    }else{
        if(load_local_file(stream_xml_url,&b)!=0){
            if(err&&err_size)snprintf(err,err_size,"Lokale XML nicht lesbar: %s",stream_xml_url);
            return -1;
        }
    }
    int count=0;
    int capacity=256;
    StreamEntry *list=calloc((size_t)capacity,sizeof(StreamEntry));
    if(!list){
        free(b.data);
        if(err&&err_size)snprintf(err,err_size,"Nicht genug Speicher fuer Streams");
        return -1;
    }

    const char *p=b.data?b.data:"";
    while(1){
        const char *a=strstr(p,"<station ");
        if(!a)break;
        const char *e=strstr(a,"></station>");
        if(!e){
            e=strstr(a,"/>");
            if(!e)break;
        }

        size_t len=(size_t)(e-a)+2;
        char *blk=malloc(len+1);
        if(!blk)break;
        memcpy(blk,a,len);
        blk[len]='\0';

        if(count>=capacity){
            int new_capacity=capacity*2;
            StreamEntry *grown=realloc(list,(size_t)new_capacity*sizeof(StreamEntry));
            if(!grown){
                free(blk);
                free(list);
                free(b.data);
                if(err&&err_size)snprintf(err,err_size,"Nicht genug Speicher fuer weitere Streams");
                return -1;
            }
            memset(grown+capacity,0,(size_t)(new_capacity-capacity)*sizeof(StreamEntry));
            list=grown;
            capacity=new_capacity;
        }

        StreamEntry *se=&list[count];
        memset(se,0,sizeof(*se));

        attr(blk,"stationuuid",se->uuid,sizeof(se->uuid));
        attr(blk,"name",se->name,sizeof(se->name));

        char resolved[STREAM_URL_LEN]="";
        char direct[STREAM_URL_LEN]="";
        attr(blk,"url_resolved",resolved,sizeof(resolved));
        attr(blk,"url",direct,sizeof(direct));

        if(resolved[0])
            snprintf(se->url,sizeof(se->url),"%s",resolved);
        else if(direct[0])
            snprintf(se->url,sizeof(se->url),"%s",direct);

        attr(blk,"codec",se->type,sizeof(se->type));
        attr(blk,"tags",se->group,sizeof(se->group));
        attr(blk,"favicon",se->logo,sizeof(se->logo));

        free(blk);

        if(se->url[0]){
            if(!se->name[0])
                snprintf(se->name,sizeof(se->name),"Stream %d",count+1);
            count++;
        }

        p=e+2;
    }
    free(b.data);
    if(!count){
        free(list);
        if(entries)*entries=NULL;
        if(count_out)*count_out=0;
        if(err&&err_size)snprintf(err,err_size,"Keine Streams in XML");
        return -1;
    }

    StreamEntry *shrunk=realloc(list,(size_t)count*sizeof(StreamEntry));
    if(shrunk)list=shrunk;

    if(entries)*entries=list;
    else free(list);
    if(count_out)*count_out=count;
    return 0;
}

static int metadata_parse_line(const char *line,char *station,size_t station_size,char *title,size_t title_size){
    if(!line)return 0;
    const char *p=NULL;

    p=strstr(line,"icy-name:");
    if(p && station && station_size){
        p+=9;
        while(*p==' '||*p=='\t')p++;
        snprintf(station,station_size,"%s",p);
        trim(station);
    }

    p=strstr(line,"icy-title:");
    if(p && title && title_size){
        p+=10;
        while(*p==' '||*p=='\t')p++;
        snprintf(title,title_size,"%s",p);
        trim(title);
    }

    p=strstr(line,"StreamTitle=");
    if(p && title && title_size){
        p+=12;
        if(*p=='\'')p++;
        const char *e=strchr(p,'\'');
        size_t n=e?(size_t)(e-p):strlen(p);
        if(n>=title_size)n=title_size-1;
        memcpy(title,p,n);
        title[n]='\0';
        trim(title);
    }

    return 1;
}

static int read_ffmpeg_metadata(char *station,size_t station_size,char *title,size_t title_size){
    FILE *fp=fopen("/tmp/hoerspiel-ffmpeg.log","r");
    if(!fp)return 0;
    char line[4096];
    while(fgets(line,sizeof(line),fp)){
        trim(line);
        metadata_parse_line(line,station,station_size,title,title_size);
    }
    fclose(fp);
    return 1;
}

static const char *find_ffmpeg_binary(void){
    if(access("/usr/bin/ffmpeg",X_OK)==0)return "/usr/bin/ffmpeg";
    if(access("/bin/ffmpeg",X_OK)==0)return "/bin/ffmpeg";
    return "ffmpeg";
}

void streaming_stop(void){
    if(ffmpeg_pid>0){
        kill(ffmpeg_pid,SIGTERM);
        usleep(150000);
        waitpid(ffmpeg_pid,NULL,WNOHANG);
    }
    ffmpeg_pid=-1;
    ffmpeg_paused=0;
    current_name[0]='\0';
}

int streaming_start(const StreamEntry *entry,char *err,size_t err_size){
    streaming_stop();

    pid_t pid=fork();
    if(pid<0){
        if(err&&err_size)snprintf(err,err_size,"fork fehlgeschlagen");
        return -1;
    }

    if(pid==0){
        int log_fd=open("/tmp/hoerspiel-ffmpeg.log",O_WRONLY|O_CREAT|O_TRUNC,0644);
        if(log_fd>=0){
            dup2(log_fd,STDERR_FILENO);
            close(log_fd);
        }

        const char *ffmpeg=find_ffmpeg_binary();
        execlp(ffmpeg,ffmpeg,
               "-hide_banner",
               "-loglevel","info",
               "-reconnect","1",
               "-reconnect_streamed","1",
               "-reconnect_delay_max","5",
               "-i",entry->url,
               "-vn",
               "-f","alsa",
               "default",
               (char*)NULL);
        _exit(127);
    }

    ffmpeg_pid=pid;
    snprintf(current_name,sizeof(current_name),"%s",entry->name);

    /* Kurz pruefen, ob FFmpeg sofort wegen fehlendem Binary/Audio-Device beendet. */
    for(int i=0;i<10;i++){
        usleep(100000);
        int status=0;
        pid_t r=waitpid(ffmpeg_pid,&status,WNOHANG);
        if(r==ffmpeg_pid){
            ffmpeg_pid=-1;
            if(err&&err_size)snprintf(err,err_size,"ffmpeg konnte Stream nicht starten");
            return -1;
        }
    }
    return 0;
}

int streaming_is_active(void){
    if(ffmpeg_pid<=0)return 0;
    int status=0;
    pid_t r=waitpid(ffmpeg_pid,&status,WNOHANG);
    if(r==ffmpeg_pid){
        ffmpeg_pid=-1;
        return 0;
    }
    return 1;
}

int streaming_toggle_pause(void){
    if(ffmpeg_pid<=0)return -1;
    if(ffmpeg_paused){
        if(kill(ffmpeg_pid,SIGCONT)!=0)return -1;
        ffmpeg_paused=0;
    }else{
        if(kill(ffmpeg_pid,SIGSTOP)!=0)return -1;
        ffmpeg_paused=1;
    }
    return 0;
}

int streaming_set_volume(int percent){
    (void)percent;
    return 0;
}

int streaming_get_metadata(char *station,size_t ss,char *title,size_t ts,char *extra,size_t es){
    if(station&&ss)station[0]='\0';
    if(title&&ts)title[0]='\0';
    if(extra&&es)snprintf(extra,es,"LIVE");
    if(!streaming_is_active())return 0;

    if(station&&ss)snprintf(station,ss,"%s",current_name);
    read_ffmpeg_metadata(station,ss,title,ts);
    return 1;
}

const char *streaming_current_name(void){
    return current_name;
}
