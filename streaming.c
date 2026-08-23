#include "streaming.h"
#include "storage.h"
#include <strings.h>
#include <curl/curl.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
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
static pid_t stream_pid=-1;
static int stream_paused=0;
static int stream_backend=0; /* 0=none, 1=mpv, 2=ffmpeg */
static int stream_session=0;
static char current_name[STREAM_NAME_LEN]="";
static char current_url[STREAM_URL_LEN]="";

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

/* Radio-Favoriten werden als UUIDs in [streams] der config.ini gespeichert.
   Format: favorites=uuid1,uuid2,uuid3

   Die Liste wird im RAM gecacht, damit grosse Favoritenlisten nicht bei jedem
   Renderdurchlauf erneut die config.ini parsen. */
#define FAVORITES_KEY "favorites"

static char **favorite_cache=NULL;
static int favorite_cache_count=0;
static int favorite_cache_loaded=0;

static void favorites_free_items(char **items,int count){
    for(int i=0;i<count;i++)free(items[i]);
    free(items);
}

static void favorites_cache_clear(void){
    favorites_free_items(favorite_cache,favorite_cache_count);
    favorite_cache=NULL;
    favorite_cache_count=0;
    favorite_cache_loaded=0;
}

static int favorites_read_config(char ***items_out,int *count_out){
    char **items=NULL;
    int count=0;

    FILE *fp=fopen(get_storage_config_path(),"r");
    if(fp){
        char line[4096];
        int in_streams=0;

        while(fgets(line,sizeof(line),fp)){
            trim(line);

            if(line[0]=='['){
                in_streams=!strcmp(line,"[streams]");
                continue;
            }

            if(!in_streams)continue;

            char *eq=strchr(line,'=');
            if(!eq)continue;

            *eq='\0';
            char *key=line;
            char *value=eq+1;
            trim(key);
            trim(value);

            if(strcmp(key,FAVORITES_KEY))continue;

            char *save=NULL;
            for(char *tok=strtok_r(value,",",&save);tok;tok=strtok_r(NULL,",",&save)){
                trim(tok);
                if(!tok[0])continue;

                char **grown=realloc(items,(size_t)(count+1)*sizeof(*items));
                if(!grown)break;
                items=grown;

                items[count]=strdup(tok);
                if(items[count])count++;
            }
            break;
        }
        fclose(fp);
    }

    *items_out=items;
    *count_out=count;
    return 0;
}

static void favorites_cache_load(void){
    if(favorite_cache_loaded)return;

    favorite_cache_loaded=1;
    favorite_cache=NULL;
    favorite_cache_count=0;
    favorites_read_config(&favorite_cache,&favorite_cache_count);
}

static int favorites_cache_find(const char *uuid){
    if(!uuid||!uuid[0])return -1;

    favorites_cache_load();

    for(int i=0;i<favorite_cache_count;i++){
        if(!strcmp(favorite_cache[i],uuid))return i;
    }
    return -1;
}

static int favorites_write_cache(void){
    const char *path=get_storage_config_path();
    char tmp_path[1200];
    snprintf(tmp_path,sizeof(tmp_path),"%s.tmp",path);

    FILE *in=fopen(path,"r");
    FILE *out=fopen(tmp_path,"w");
    if(!out){
        if(in)fclose(in);
        return -1;
    }

    int in_streams=0,streams_seen=0,key_written=0;
    char line[4096];

    if(in){
        while(fgets(line,sizeof(line),in)){
            char check[4096];
            snprintf(check,sizeof(check),"%s",line);
            trim(check);

            if(check[0]=='['){
                if(in_streams && !key_written){
                    fprintf(out,"%s=",FAVORITES_KEY);
                    for(int i=0;i<favorite_cache_count;i++)
                        fprintf(out,"%s%s",i?",":"",favorite_cache[i]);
                    fprintf(out,"\n");
                    key_written=1;
                }

                in_streams=!strcmp(check,"[streams]");
                if(in_streams)streams_seen=1;
                fputs(line,out);
                continue;
            }

            if(in_streams){
                char *eq=strchr(check,'=');
                if(eq){
                    *eq='\0';
                    trim(check);

                    if(!strcmp(check,FAVORITES_KEY)){
                        if(!key_written){
                            fprintf(out,"%s=",FAVORITES_KEY);
                            for(int i=0;i<favorite_cache_count;i++)
                                fprintf(out,"%s%s",i?",":"",favorite_cache[i]);
                            fprintf(out,"\n");
                            key_written=1;
                        }
                        continue;
                    }
                }
            }

            fputs(line,out);
        }
        fclose(in);
    }

    if(in_streams && !key_written){
        fprintf(out,"%s=",FAVORITES_KEY);
        for(int i=0;i<favorite_cache_count;i++)
            fprintf(out,"%s%s",i?",":"",favorite_cache[i]);
        fprintf(out,"\n");
        key_written=1;
    }

    if(!streams_seen){
        fprintf(out,"\n[streams]\n%s=",FAVORITES_KEY);
        for(int i=0;i<favorite_cache_count;i++)
            fprintf(out,"%s%s",i?",":"",favorite_cache[i]);
        fprintf(out,"\n");
    }

    fclose(out);
    return rename(tmp_path,path);
}

int streaming_favorite_is_set(const char *uuid){
    return favorites_cache_find(uuid)>=0;
}

int streaming_favorite_toggle(const char *uuid){
    if(!uuid||!uuid[0])return -1;

    favorites_cache_load();
    int existing=favorites_cache_find(uuid);

    if(existing>=0){
        free(favorite_cache[existing]);

        for(int i=existing;i<favorite_cache_count-1;i++)
            favorite_cache[i]=favorite_cache[i+1];

        favorite_cache_count--;

        if(favorite_cache_count==0){
            free(favorite_cache);
            favorite_cache=NULL;
        }else{
            char **shrunk=realloc(favorite_cache,
                                  (size_t)favorite_cache_count*sizeof(*favorite_cache));
            if(shrunk)favorite_cache=shrunk;
        }

        if(favorites_write_cache()!=0)return -1;
        return 0;
    }

    char **grown=realloc(favorite_cache,
                         (size_t)(favorite_cache_count+1)*sizeof(*favorite_cache));
    if(!grown)return -1;

    favorite_cache=grown;
    favorite_cache[favorite_cache_count]=strdup(uuid);
    if(!favorite_cache[favorite_cache_count])return -1;

    favorite_cache_count++;

    if(favorites_write_cache()!=0)return -1;
    return 1;
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
        fprintf(fp,"favorites=\n");
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
    int have_xml=0,have_mode=0,have_ca=0,have_cert=0,have_key=0,have_pass=0,have_favorites=0;

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
            else if(!strcmp(check,FAVORITES_KEY))have_favorites=1;
        }
    }

    int complete=(section_start>=0&&have_xml&&have_mode&&have_ca&&have_cert&&have_key&&have_pass&&have_favorites);
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
        fprintf(out,"favorites=\n");
    }else{
        for(int i=0;i<line_count;i++){
            if(i==section_end){
                if(!have_xml)fprintf(out,"xml_url=\n");
                if(!have_mode)fprintf(out,"client_cert_mode=none\n");
                if(!have_ca)fprintf(out,"ca_cert=\n");
                if(!have_cert)fprintf(out,"client_cert=\n");
                if(!have_key)fprintf(out,"client_key=\n");
                if(!have_pass)fprintf(out,"client_key_password=\n");
                if(!have_favorites)fprintf(out,"favorites=\n");
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
            if(!have_favorites)fprintf(out,"favorites=\n");
        }
    }

    fclose(out);

    for(int i=0;i<line_count;i++)free(lines[i]);
    free(lines);

    rename(tmp_path,path);
}

void streaming_load_config(void){
    streaming_ensure_config_section();
    favorites_cache_clear();
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

#define STREAM_BACKEND_NONE 0
#define STREAM_BACKEND_MPV 1
#define STREAM_BACKEND_FFMPEG 2
#define MPV_SOCKET "/tmp/hoerspiel-mpv.sock"

static int command_exists(const char *name){
    if(!name||!name[0])return 0;
    if(strchr(name,'/'))return access(name,X_OK)==0;
    const char *path=getenv("PATH");
    if(!path)path="/usr/local/bin:/usr/bin:/bin";
    char tmp[4096];
    snprintf(tmp,sizeof(tmp),"%s",path);
    char *save=NULL;
    for(char *dir=strtok_r(tmp,":",&save);dir;dir=strtok_r(NULL,":",&save)){
        char full[PATH_MAX];
        snprintf(full,sizeof(full),"%s/%s",dir,name);
        if(access(full,X_OK)==0)return 1;
    }
    return 0;
}

static const char *find_mpv_binary(void){
    if(access("/usr/bin/mpv",X_OK)==0)return "/usr/bin/mpv";
    if(access("/bin/mpv",X_OK)==0)return "/bin/mpv";
    if(command_exists("mpv"))return "mpv";
    return NULL;
}

static const char *find_ffmpeg_binary(void){
    if(access("/usr/bin/ffmpeg",X_OK)==0)return "/usr/bin/ffmpeg";
    if(access("/bin/ffmpeg",X_OK)==0)return "/bin/ffmpeg";
    if(command_exists("ffmpeg"))return "ffmpeg";
    return NULL;
}

static int mpv_ipc(const char *json,char *response,size_t response_size){
    int fd=socket(AF_UNIX,SOCK_STREAM,0);
    if(fd<0)return -1;
    struct sockaddr_un a;
    memset(&a,0,sizeof(a));
    a.sun_family=AF_UNIX;
    snprintf(a.sun_path,sizeof(a.sun_path),"%s",MPV_SOCKET);
    if(connect(fd,(struct sockaddr*)&a,sizeof(a))<0){close(fd);return -1;}
    write(fd,json,strlen(json));
    write(fd,"\n",1);
    if(response&&response_size){
        ssize_t n=read(fd,response,response_size-1);
        if(n<0)n=0;
        response[n]='\0';
    }
    close(fd);
    return 0;
}

static int json_data_string(const char *json,char *out,size_t out_size){
    const char *p=strstr(json,"\"data\":");
    if(!p)return 0;
    p+=7;
    while(*p&&isspace((unsigned char)*p))p++;
    if(*p!='"')return 0;
    p++;
    size_t o=0;
    while(*p&&*p!='"'&&o+1<out_size){
        if(*p=='\\'&&p[1])p++;
        out[o++]=*p++;
    }
    out[o]='\0';
    return 1;
}

static int mpv_prop(const char *name,char *out,size_t out_size){
    char cmd[256],resp[4096];
    snprintf(cmd,sizeof(cmd),"{\"command\":[\"get_property\",\"%s\"]}",name);
    if(mpv_ipc(cmd,resp,sizeof(resp))<0)return 0;
    return json_data_string(resp,out,out_size);
}

static int metadata_parse_line(const char *line,char *station,size_t station_size,char *title,size_t title_size){
    if(!line)return 0;

    char buf[4096];
    snprintf(buf,sizeof(buf),"%s",line);
    trim(buf);
    if(!buf[0])return 0;

    /* FFmpeg schreibt ICY-Metadaten typischerweise als:
       icy-name        : Sender
       StreamTitle     : Interpret - Titel
       Manche Builds/Quellen verwenden alternativ StreamTitle=... */
    char *sep=strchr(buf,':');
    char *eq=strchr(buf,'=');

    if(eq && (!sep || eq<sep))sep=eq;
    if(!sep)return 0;

    *sep='\0';
    char *key=buf;
    char *value=sep+1;
    trim(key);
    trim(value);

    size_t vl=strlen(value);
    if(vl>=2 && ((value[0]=='\'' && value[vl-1]=='\'') ||
                 (value[0]=='"'  && value[vl-1]=='"'))){
        value[vl-1]='\0';
        value++;
        trim(value);
    }

    if(!strcasecmp(key,"icy-name")){
        if(station&&station_size)snprintf(station,station_size,"%s",value);
        return 1;
    }

    if(!strcasecmp(key,"icy-title") || !strcasecmp(key,"StreamTitle")){
        if(title&&title_size)snprintf(title,title_size,"%s",value);
        return 1;
    }

    return 0;
}

static int read_ffmpeg_metadata(char *station,size_t station_size,
                                char *title,size_t title_size,
                                char *br,size_t br_size,
                                char *samplerate,size_t samplerate_size,
                                char *channels,size_t channels_size,
                                char *description,size_t description_size){
    FILE *fp=fopen("/tmp/hoerspiel-ffmpeg.log","r");
    if(!fp)return 0;

    char line[4096];
    while(fgets(line,sizeof(line),fp)){
        char raw[4096];
        snprintf(raw,sizeof(raw),"%s",line);
        trim(raw);

        metadata_parse_line(raw,station,station_size,title,title_size);

        char *sep=strchr(raw,':');
        char *eq=strchr(raw,'=');
        if(eq && (!sep || eq<sep))sep=eq;
        if(!sep)continue;

        *sep='\0';
        char *key=raw;
        char *value=sep+1;
        trim(key); trim(value);

        if(!strcasecmp(key,"icy-br") && br && br_size)
            snprintf(br,br_size,"%s",value);
        else if(!strcasecmp(key,"icy-samplerate") && samplerate && samplerate_size)
            snprintf(samplerate,samplerate_size,"%s",value);
        else if(!strcasecmp(key,"icy-channels") && channels && channels_size)
            snprintf(channels,channels_size,"%s",value);
        else if(!strcasecmp(key,"icy-description") && description && description_size)
            snprintf(description,description_size,"%s",value);
    }
    fclose(fp);
    return 1;
}

static void stop_child(void){
    if(stream_pid>0){
        if(stream_backend==STREAM_BACKEND_MPV){
            char r[64];
            mpv_ipc("{\"command\":[\"quit\"]}",r,sizeof(r));
            usleep(100000);
        }
        kill(stream_pid,SIGTERM);
        usleep(100000);
        waitpid(stream_pid,NULL,WNOHANG);
    }
    stream_pid=-1;
}

void streaming_stop(void){
    stop_child();
    stream_paused=0;
    stream_backend=STREAM_BACKEND_NONE;
    stream_session=0;
    current_name[0]='\0';
    current_url[0]='\0';
    unlink(MPV_SOCKET);
}

static int start_mpv(const StreamEntry *entry){
    const char *mpv=find_mpv_binary();
    if(!mpv)return -1;
    unlink(MPV_SOCKET);
    pid_t pid=fork();
    if(pid<0)return -1;
    if(pid==0){
        execlp(mpv,mpv,
               "--no-video","--really-quiet",
               "--input-ipc-server=" MPV_SOCKET,
               "--cache=yes","--cache-secs=8",
               entry->url,(char*)NULL);
        _exit(127);
    }
    stream_pid=pid;
    for(int i=0;i<40;i++){
        struct stat st;
        if(stat(MPV_SOCKET,&st)==0){
            stream_backend=STREAM_BACKEND_MPV;
            return 0;
        }
        usleep(100000);
        int status=0;
        if(waitpid(stream_pid,&status,WNOHANG)==stream_pid){
            stream_pid=-1;
            break;
        }
    }
    stop_child();
    unlink(MPV_SOCKET);
    return -1;
}

static int start_ffmpeg(const StreamEntry *entry){
    const char *ffmpeg=find_ffmpeg_binary();
    if(!ffmpeg)return -1;
    pid_t pid=fork();
    if(pid<0)return -1;
    if(pid==0){
        int log_fd=open("/tmp/hoerspiel-ffmpeg.log",O_WRONLY|O_CREAT|O_TRUNC,0644);
        if(log_fd>=0){
            dup2(log_fd,STDERR_FILENO);
            close(log_fd);
        }
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
    stream_pid=pid;
    for(int i=0;i<10;i++){
        usleep(100000);
        int status=0;
        if(waitpid(stream_pid,&status,WNOHANG)==stream_pid){
            stream_pid=-1;
            return -1;
        }
    }
    stream_backend=STREAM_BACKEND_FFMPEG;
    return 0;
}

int streaming_start(const StreamEntry *entry,char *err,size_t err_size){
    stop_child();
    stream_paused=0;
    stream_backend=STREAM_BACKEND_NONE;
    stream_session=0;
    unlink(MPV_SOCKET);
    snprintf(current_name,sizeof(current_name),"%s",entry->name);
    snprintf(current_url,sizeof(current_url),"%s",entry->url);

    if(start_mpv(entry)==0){
        stream_session=1;
        return 0;
    }
    if(start_ffmpeg(entry)==0){
        stream_session=1;
        return 0;
    }

    current_name[0]='\0';
    current_url[0]='\0';
    if(err&&err_size)snprintf(err,err_size,"Weder mpv noch ffmpeg verfuegbar/startbar");
    return -1;
}

int streaming_is_active(void){
    if(stream_pid<=0)return 0;
    int status=0;
    if(waitpid(stream_pid,&status,WNOHANG)==stream_pid){
        stream_pid=-1;
        stream_backend=STREAM_BACKEND_NONE;
        unlink(MPV_SOCKET);
        return 0;
    }
    return 1;
}

int streaming_is_paused(void){
    return stream_paused;
}

int streaming_toggle_pause(void){
    if(stream_pid<=0)return -1;
    if(stream_backend==STREAM_BACKEND_MPV){
        char cmd[128],r[256];
        stream_paused=!stream_paused;
        snprintf(cmd,sizeof(cmd),"{\"command\":[\"set_property\",\"pause\",%s]}",
                 stream_paused?"true":"false");
        return mpv_ipc(cmd,r,sizeof(r));
    }

    if(stream_backend==STREAM_BACKEND_FFMPEG){
        if(stream_paused){
            if(kill(stream_pid,SIGCONT)!=0)return -1;
            stream_paused=0;
        }else{
            if(kill(stream_pid,SIGSTOP)!=0)return -1;
            stream_paused=1;
        }
        return 0;
    }
    return -1;
}

int streaming_set_volume(int percent){
    if(stream_backend==STREAM_BACKEND_MPV){
        if(percent<0)percent=0;
        if(percent>100)percent=100;
        char cmd[128],r[256];
        snprintf(cmd,sizeof(cmd),"{\"command\":[\"set_property\",\"volume\",%d]}",percent);
        return mpv_ipc(cmd,r,sizeof(r));
    }
    (void)percent;
    return 0;
}

int streaming_get_metadata(char *station,size_t ss,char *title,size_t ts,char *extra,size_t es){
    if(station&&ss)station[0]='\0';
    if(title&&ts)title[0]='\0';
    if(extra&&es)extra[0]='\0';
    if(!streaming_is_active())return 0;

    if(station&&ss)snprintf(station,ss,"%s",current_name);

    if(stream_backend==STREAM_BACKEND_MPV){
        char br[32]="",sr[32]="",ch[32]="";
        mpv_prop("metadata/by-key/icy-name",station,ss);
        mpv_prop("metadata/by-key/icy-title",title,ts);
        mpv_prop("metadata/by-key/icy-br",br,sizeof(br));
        mpv_prop("metadata/by-key/icy-samplerate",sr,sizeof(sr));
        mpv_prop("metadata/by-key/icy-channels",ch,sizeof(ch));

        if(extra&&es){
            if(br[0]&&sr[0]&&ch[0])
                snprintf(extra,es,"mpv | %s kbps | %s Hz | %s ch",br,sr,ch);
            else if(br[0]&&sr[0])
                snprintf(extra,es,"mpv | %s kbps | %s Hz",br,sr);
            else if(br[0])
                snprintf(extra,es,"mpv | %s kbps",br);
            else snprintf(extra,es,"mpv");
        }
    }else if(stream_backend==STREAM_BACKEND_FFMPEG){
        char br[32]="",sr[32]="",ch[32]="",desc[512]="";
        read_ffmpeg_metadata(station,ss,title,ts,
                             br,sizeof(br),sr,sizeof(sr),ch,sizeof(ch),
                             desc,sizeof(desc));
        if(extra&&es){
            if(br[0]&&sr[0]&&ch[0])
                snprintf(extra,es,"ffmpeg | %s kbps | %s Hz | %s ch",br,sr,ch);
            else if(br[0]&&sr[0])
                snprintf(extra,es,"ffmpeg | %s kbps | %s Hz",br,sr);
            else if(br[0])
                snprintf(extra,es,"ffmpeg | %s kbps",br);
            else snprintf(extra,es,"ffmpeg");
        }
    }
    return 1;
}

int streaming_get_description(char *description,size_t description_size){
    if(!description||description_size==0)return 0;
    description[0]='\0';
    if(!streaming_is_active())return 0;

    if(stream_backend==STREAM_BACKEND_MPV){
        return mpv_prop("metadata/by-key/icy-description",description,description_size);
    }

    if(stream_backend==STREAM_BACKEND_FFMPEG){
        char station[8]="",title[8]="",br[8]="",sr[8]="",ch[8]="";
        read_ffmpeg_metadata(station,sizeof(station),title,sizeof(title),
                             br,sizeof(br),sr,sizeof(sr),ch,sizeof(ch),
                             description,description_size);
        return description[0]!=0;
    }
    return 0;
}

const char *streaming_current_url(void){
    return current_url;
}

const char *streaming_current_name(void){
    return current_name;
}

const char *streaming_backend_name(void){
    if(stream_backend==STREAM_BACKEND_MPV)return "mpv";
    if(stream_backend==STREAM_BACKEND_FFMPEG)return "ffmpeg";
    if(find_mpv_binary())return "mpv";
    if(find_ffmpeg_binary())return "ffmpeg";
    return "nicht verfuegbar";
}

int streaming_backend_available(void){
    return find_mpv_binary()!=NULL || find_ffmpeg_binary()!=NULL;
}

int streaming_session_active(void){
    return stream_session;
}

void streaming_session_clear(void){
    stream_session=0;
    current_name[0]='\0';
    stream_backend=STREAM_BACKEND_NONE;
    stream_paused=0;
    unlink(MPV_SOCKET);
}
