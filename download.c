#include "download.h"
#include "storage.h"

#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int ensure_curl_global(void)
{
    static int initialized=0;
    static int ok=0;
    if(!initialized){
        ok=(curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK);
        initialized=1;
    }
    return ok;
}

typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;

typedef struct {
    FILE *fp;
    const char *name;
    int file_index;
    int file_count;
    long long completed_before;
    long long total_size;
    DownloadProgressFn progress;
    void *userdata;
} FileProgress;

typedef struct {
    DownloadProgressFn progress;
    void *userdata;
    int total_files;
    int file_offset;
    long long total_size;
    long long byte_offset;
} SelectionProgress;

static size_t memory_write(void *ptr,size_t size,size_t nmemb,void *userdata)
{
    MemoryBuffer *m=(MemoryBuffer*)userdata;
    size_t n=size*nmemb;
    char *p=realloc(m->data,m->size+n+1);
    if(!p)return 0;
    m->data=p;
    memcpy(m->data+m->size,ptr,n);
    m->size+=n;
    m->data[m->size]='\0';
    return n;
}

static size_t file_write(void *ptr,size_t size,size_t nmemb,void *userdata)
{
    FileProgress *p=(FileProgress*)userdata;
    return fwrite(ptr,size,nmemb,p->fp);
}

static int xfer_info(void *userdata,curl_off_t dltotal,curl_off_t dlnow,curl_off_t ultotal,curl_off_t ulnow)
{
    (void)ultotal;
    (void)ulnow;
    FileProgress *p=(FileProgress*)userdata;
    if(!p->progress)return 0;
    return p->progress(p->name,p->file_index,p->file_count,
                       (long long)dlnow,(long long)dltotal,
                       p->completed_before+(long long)dlnow,
                       p->total_size,p->userdata)?1:0;
}

static int selection_progress(void *userdata_name);

static void set_tls_options(CURL *curl)
{
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,download_verify_peer?1L:0L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,download_verify_host?2L:0L);
    if(download_ca_cert[0])curl_easy_setopt(curl,CURLOPT_CAINFO,download_ca_cert);
    if(download_client_cert[0])curl_easy_setopt(curl,CURLOPT_SSLCERT,download_client_cert);
    if(download_client_key[0])curl_easy_setopt(curl,CURLOPT_SSLKEY,download_client_key);
    if(download_client_key_password[0])curl_easy_setopt(curl,CURLOPT_KEYPASSWD,download_client_key_password);
}

static int safe_component(const char *s)
{
    if(!s||!s[0]||!strcmp(s,".")||!strcmp(s,".."))return 0;
    return !strchr(s,'/')&&!strchr(s,'\\');
}

static void decode_xml_text(char *s)
{
    struct Pair{const char *from;const char *to;} pairs[]={
        {"&amp;","&"},{"&lt;","<"},{"&gt;",">"},{"&quot;","\""},{"&apos;","'"}
    };
    for(size_t p=0;p<sizeof(pairs)/sizeof(pairs[0]);p++){
        char *pos;
        while((pos=strstr(s,pairs[p].from))!=NULL){
            size_t fl=strlen(pairs[p].from),tl=strlen(pairs[p].to);
            memmove(pos+tl,pos+fl,strlen(pos+fl)+1);
            memcpy(pos,pairs[p].to,tl);
        }
    }
}

static void attr_value(const char *tag,const char *name,char *out,size_t out_size)
{
    out[0]='\0';
    char pat[64];
    snprintf(pat,sizeof(pat),"%s=\"",name);
    const char *p=strstr(tag,pat);
    if(!p)return;
    p+=strlen(pat);
    const char *e=strchr(p,'\"');
    if(!e)return;
    size_t n=(size_t)(e-p);
    if(n>=out_size)n=out_size-1;
    memcpy(out,p,n);
    out[n]='\0';
}

static int parse_listing(const char *xml,RemoteEntry *entries,int max_entries)
{
    int n=0;
    const char *p=xml;
    while(*p&&n<max_entries){
        const char *d=strstr(p,"<directory");
        const char *f=strstr(p,"<file");
        const char *start=NULL,*close=NULL;
        RemoteEntryType type;
        if(d&&(!f||d<f)){start=d;close=strstr(d,">");type=REMOTE_DIRECTORY;}
        else if(f){start=f;close=strstr(f,">");type=REMOTE_FILE;}
        else break;
        if(!close)break;
        const char *endtag=type==REMOTE_DIRECTORY?strstr(close,"</directory>"):strstr(close,"</file>");
        if(!endtag)break;

        char tag[512];
        size_t taglen=(size_t)(close-start+1);
        if(taglen>=sizeof(tag))taglen=sizeof(tag)-1;
        memcpy(tag,start,taglen);
        tag[taglen]='\0';

        size_t namelen=(size_t)(endtag-(close+1));
        if(namelen>=sizeof(entries[n].name))namelen=sizeof(entries[n].name)-1;
        memcpy(entries[n].name,close+1,namelen);
        entries[n].name[namelen]='\0';
        decode_xml_text(entries[n].name);

        if(safe_component(entries[n].name)){
            char tmp[64];
            entries[n].type=type;
            entries[n].size=0;
            entries[n].mtime[0]='\0';
            attr_value(tag,"mtime",entries[n].mtime,sizeof(entries[n].mtime));
            if(type==REMOTE_FILE){
                attr_value(tag,"size",tmp,sizeof(tmp));
                if(tmp[0])entries[n].size=strtoll(tmp,NULL,10);
            }
            n++;
        }
        p=endtag+(type==REMOTE_DIRECTORY?12:7);
    }
    return n;
}

static int append_encoded_path(CURL *curl,const char *relative,char *url,size_t url_size)
{
    snprintf(url,url_size,"%s",download_base_url);
    size_t len=strlen(url);
    if(len&&url[len-1]!='/')strncat(url,"/",url_size-strlen(url)-1);
    if(!relative||!relative[0])return 0;

    char copy[REMOTE_PATH_LEN];
    snprintf(copy,sizeof(copy),"%s",relative);
    char *save=NULL;
    char *part=strtok_r(copy,"/",&save);
    while(part){
        if(!safe_component(part))return -1;
        char *esc=curl_easy_escape(curl,part,0);
        if(!esc)return -1;
        if(strlen(url)+strlen(esc)+2>=url_size){curl_free(esc);return -1;}
        strcat(url,esc);
        curl_free(esc);
        part=strtok_r(NULL,"/",&save);
        if(part)strcat(url,"/");
    }
    if(url[strlen(url)-1]!='/')strcat(url,"/");
    return 0;
}

static int make_dirs(const char *path)
{
    char tmp[REMOTE_PATH_LEN+1024];
    snprintf(tmp,sizeof(tmp),"%s",path);
    for(char *p=tmp+1;*p;p++){
        if(*p=='/'){
            *p='\0';
            if(mkdir(tmp,0775)!=0&&errno!=EEXIST)return -1;
            *p='/';
        }
    }
    if(mkdir(tmp,0775)!=0&&errno!=EEXIST)return -1;
    return 0;
}

static int local_file_matches_remote_size(const char *path,long long remote_size)
{
    if(!path||remote_size<=0)return 0;
    struct stat st;
    if(stat(path,&st)!=0||!S_ISREG(st.st_mode))return 0;
    return (long long)st.st_size==remote_size;
}

int remote_fetch_listing(const char *relative_path,RemoteEntry *entries,int max_entries,char *error,size_t error_size)
{
    if(error&&error_size)error[0]='\0';
    if(!download_base_url[0]){snprintf(error,error_size,"base_url fehlt in config.ini");return -1;}
    if(!ensure_curl_global()){snprintf(error,error_size,"libcurl konnte nicht initialisiert werden");return -1;}

    CURL *curl=curl_easy_init();
    if(!curl){snprintf(error,error_size,"libcurl konnte nicht initialisiert werden");return -1;}
    char url[4096];
    if(append_encoded_path(curl,relative_path,url,sizeof(url))!=0){
        snprintf(error,error_size,"Ungueltiger oder zu langer Pfad");
        curl_easy_cleanup(curl);
        return -1;
    }

    MemoryBuffer mem={0};
    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1L);
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,10L);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,30L);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,memory_write);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&mem);
    curl_easy_setopt(curl,CURLOPT_USERAGENT,"book_player_r36s/0.2");
    set_tls_options(curl);

    CURLcode rc=curl_easy_perform(curl);
    long code=0;
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);
    curl_easy_cleanup(curl);
    if(rc!=CURLE_OK){snprintf(error,error_size,"Netzwerk: %s",curl_easy_strerror(rc));free(mem.data);return -1;}
    if(code<200||code>=300){snprintf(error,error_size,"HTTP %ld",code);free(mem.data);return -1;}

    int n=parse_listing(mem.data?mem.data:"",entries,max_entries);
    free(mem.data);
    if(n<=0){snprintf(error,error_size,"Listing leer oder unbekanntes XML-Format");return -1;}
    return n;
}

int remote_download_files(const char *relative_path,const RemoteEntry *entries,int entry_count,
                          DownloadProgressFn progress,void *userdata,char *error,size_t error_size)
{
    if(error&&error_size)error[0]='\0';
    int file_count=0;
    long long total_size=0;
    for(int i=0;i<entry_count;i++)if(entries[i].type==REMOTE_FILE){
        file_count++;
        if(entries[i].size>0)total_size+=entries[i].size;
    }
    if(file_count==0){snprintf(error,error_size,"Keine Dateien in diesem Verzeichnis");return -1;}
    if(!ensure_curl_global()){snprintf(error,error_size,"libcurl konnte nicht initialisiert werden");return -1;}

    char local_dir[REMOTE_PATH_LEN+1024];
    if(relative_path&&relative_path[0])snprintf(local_dir,sizeof(local_dir),"%s/%s",download_target_path,relative_path);
    else snprintf(local_dir,sizeof(local_dir),"%s",download_target_path);
    if(make_dirs(local_dir)!=0){snprintf(error,error_size,"Zielverzeichnis kann nicht angelegt werden: %s",local_dir);return -1;}

    CURL *curl=curl_easy_init();
    if(!curl){snprintf(error,error_size,"libcurl konnte nicht initialisiert werden");return -1;}
    long long completed=0;
    int file_index=0;

    for(int i=0;i<entry_count;i++){
        if(entries[i].type!=REMOTE_FILE)continue;
        file_index++;
        char dir_url[4096],url[4608],local[REMOTE_PATH_LEN+1536],part[REMOTE_PATH_LEN+1556];
        snprintf(local,sizeof(local),"%s/%s",local_dir,entries[i].name);
        snprintf(part,sizeof(part),"%s.part",local);

        if(local_file_matches_remote_size(local,entries[i].size)){
            unlink(part);
            if(progress&&progress(entries[i].name,file_index,file_count,
                                  entries[i].size,entries[i].size,
                                  completed+entries[i].size,total_size,userdata)){
                snprintf(error,error_size,"Download abgebrochen");
                curl_easy_cleanup(curl);
                return -1;
            }
            completed+=entries[i].size;
            continue;
        }

        if(append_encoded_path(curl,relative_path,dir_url,sizeof(dir_url))!=0){snprintf(error,error_size,"Ungueltiger Remote-Pfad");curl_easy_cleanup(curl);return -1;}
        char *esc=curl_easy_escape(curl,entries[i].name,0);
        if(!esc){snprintf(error,error_size,"Dateiname kann nicht kodiert werden");curl_easy_cleanup(curl);return -1;}
        snprintf(url,sizeof(url),"%s%s",dir_url,esc);
        curl_free(esc);

        FILE *fp=fopen(part,"wb");
        if(!fp){snprintf(error,error_size,"Kann %s nicht schreiben",part);curl_easy_cleanup(curl);return -1;}
        FileProgress pc={fp,entries[i].name,file_index,file_count,completed,total_size,progress,userdata};
        curl_easy_reset(curl);
        curl_easy_setopt(curl,CURLOPT_URL,url);
        curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1L);
        curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,10L);
        curl_easy_setopt(curl,CURLOPT_TIMEOUT,0L);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,file_write);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA,&pc);
        curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);
        curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,xfer_info);
        curl_easy_setopt(curl,CURLOPT_XFERINFODATA,&pc);
        curl_easy_setopt(curl,CURLOPT_USERAGENT,"book_player_r36s/0.2");
        set_tls_options(curl);

        CURLcode rc=curl_easy_perform(curl);
        fclose(fp);
        if(rc!=CURLE_OK){
            unlink(part);
            snprintf(error,error_size,rc==CURLE_ABORTED_BY_CALLBACK?"Download abgebrochen":"Download %s: %s",entries[i].name,curl_easy_strerror(rc));
            curl_easy_cleanup(curl);
            return -1;
        }
        if(rename(part,local)!=0){unlink(part);snprintf(error,error_size,"Kann fertige Datei nicht umbenennen");curl_easy_cleanup(curl);return -1;}
        completed+=entries[i].size>0?entries[i].size:pc.completed_before==completed?0:0;
    }
    curl_easy_cleanup(curl);
    sync();
    return file_count;
}

static int split_remote_file_path(const char *path,char *parent,size_t parent_size,char *name,size_t name_size)
{
    snprintf(parent,parent_size,"%s",path?path:"");
    char *slash=strrchr(parent,'/');
    if(slash){snprintf(name,name_size,"%s",slash+1);*slash='\0';}
    else{snprintf(name,name_size,"%s",parent);parent[0]='\0';}
    return safe_component(name)?0:-1;
}

static int find_remote_file_entry(const char *path,RemoteEntry *out,char *error,size_t error_size)
{
    char parent[REMOTE_PATH_LEN],name[REMOTE_NAME_LEN];
    if(split_remote_file_path(path,parent,sizeof(parent),name,sizeof(name))!=0){snprintf(error,error_size,"Ungueltiger Dateipfad");return -1;}
    RemoteEntry entries[REMOTE_MAX_ENTRIES];
    char listing_error[256]="";
    int n=remote_fetch_listing(parent,entries,REMOTE_MAX_ENTRIES,listing_error,sizeof(listing_error));
    if(n<0){snprintf(error,error_size,"%s: %s",parent[0]?parent:"/",listing_error);return -1;}
    for(int i=0;i<n;i++)if(entries[i].type==REMOTE_FILE&&!strcmp(entries[i].name,name)){
        *out=entries[i];
        return 0;
    }
    snprintf(error,error_size,"Datei nicht im Listing gefunden: %s",name);
    return -1;
}

static int scan_directory_stats(const char *path,int *files,long long *bytes,char *error,size_t error_size)
{
    RemoteEntry entries[REMOTE_MAX_ENTRIES];
    char listing_error[256]="";
    int n=remote_fetch_listing(path,entries,REMOTE_MAX_ENTRIES,listing_error,sizeof(listing_error));
    if(n<0){snprintf(error,error_size,"%s: %s",path&&path[0]?path:"/",listing_error);return -1;}
    for(int i=0;i<n;i++){
        if(entries[i].type==REMOTE_FILE){(*files)++;if(entries[i].size>0)*bytes+=entries[i].size;continue;}
        char child[REMOTE_PATH_LEN];
        if(path&&path[0])snprintf(child,sizeof(child),"%s/%s",path,entries[i].name);
        else snprintf(child,sizeof(child),"%s",entries[i].name);
        if(strlen(child)>=sizeof(child)-1){snprintf(error,error_size,"Remote-Pfad zu lang");return -1;}
        if(scan_directory_stats(child,files,bytes,error,error_size)<0)return -1;
    }
    return 0;
}

static int global_progress_cb(const char *name,int file_index,int file_count,
                              long long file_now,long long file_total,
                              long long total_now,long long total_size,void *userdata)
{
    (void)file_count;
    (void)total_size;
    SelectionProgress *p=(SelectionProgress*)userdata;
    if(!p->progress)return 0;
    return p->progress(name,p->file_offset+file_index,p->total_files,
                       file_now,file_total,p->byte_offset+total_now,p->total_size,p->userdata);
}

static int download_one_selected_file_global(const char *path,SelectionProgress *gp,char *error,size_t error_size)
{
    char parent[REMOTE_PATH_LEN],name[REMOTE_NAME_LEN];
    if(split_remote_file_path(path,parent,sizeof(parent),name,sizeof(name))!=0){snprintf(error,error_size,"Ungueltiger Dateipfad");return -1;}
    RemoteEntry e={0};
    if(find_remote_file_entry(path,&e,error,error_size)<0)return -1;
    int rc=remote_download_files(parent,&e,1,global_progress_cb,gp,error,error_size);
    if(rc<0)return -1;
    gp->file_offset++;
    if(e.size>0)gp->byte_offset+=e.size;
    return rc;
}

static int download_directory_recursive_global(const char *path,SelectionProgress *gp,char *error,size_t error_size)
{
    RemoteEntry entries[REMOTE_MAX_ENTRIES];
    char listing_error[256]="";
    int n=remote_fetch_listing(path,entries,REMOTE_MAX_ENTRIES,listing_error,sizeof(listing_error));
    if(n<0){snprintf(error,error_size,"%s: %s",path&&path[0]?path:"/",listing_error);return -1;}

    int total=0,files=0;
    long long bytes=0;
    for(int i=0;i<n;i++)if(entries[i].type==REMOTE_FILE){files++;if(entries[i].size>0)bytes+=entries[i].size;}
    if(files>0){
        int rc=remote_download_files(path,entries,n,global_progress_cb,gp,error,error_size);
        if(rc<0)return -1;
        total+=rc;
        gp->file_offset+=files;
        gp->byte_offset+=bytes;
    }else{
        char local_dir[REMOTE_PATH_LEN+1024];
        if(path&&path[0])snprintf(local_dir,sizeof(local_dir),"%s/%s",download_target_path,path);
        else snprintf(local_dir,sizeof(local_dir),"%s",download_target_path);
        if(make_dirs(local_dir)!=0){snprintf(error,error_size,"Zielverzeichnis kann nicht angelegt werden: %s",local_dir);return -1;}
    }

    for(int i=0;i<n;i++){
        if(entries[i].type!=REMOTE_DIRECTORY)continue;
        char child[REMOTE_PATH_LEN];
        if(path&&path[0])snprintf(child,sizeof(child),"%s/%s",path,entries[i].name);
        else snprintf(child,sizeof(child),"%s",entries[i].name);
        if(strlen(child)>=sizeof(child)-1){snprintf(error,error_size,"Remote-Pfad zu lang");return -1;}
        int rc=download_directory_recursive_global(child,gp,error,error_size);
        if(rc<0)return -1;
        total+=rc;
    }
    return total;
}

int remote_download_selection(const RemoteSelection *selection,int selection_count,
                              DownloadProgressFn progress,void *userdata,char *error,size_t error_size)
{
    if(error&&error_size)error[0]='\0';
    if(!selection||selection_count<=0){snprintf(error,error_size,"Keine Auswahl");return -1;}

    int total_files=0;
    long long total_size=0;
    for(int i=0;i<selection_count;i++){
        if(selection[i].type==REMOTE_DIRECTORY){
            if(scan_directory_stats(selection[i].relative_path,&total_files,&total_size,error,error_size)<0)return -1;
        }else{
            RemoteEntry e={0};
            if(find_remote_file_entry(selection[i].relative_path,&e,error,error_size)<0)return -1;
            total_files++;
            if(e.size>0)total_size+=e.size;
        }
    }
    if(total_files<=0){snprintf(error,error_size,"Auswahl enthaelt keine Dateien");return -1;}

    SelectionProgress gp={progress,userdata,total_files,0,total_size,0};
    int total=0;
    for(int i=0;i<selection_count;i++){
        int rc=selection[i].type==REMOTE_DIRECTORY
            ?download_directory_recursive_global(selection[i].relative_path,&gp,error,error_size)
            :download_one_selected_file_global(selection[i].relative_path,&gp,error,error_size);
        if(rc<0)return -1;
        total+=rc;
    }
    sync();
    return total;
}
