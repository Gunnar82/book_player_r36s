#include "stream_favorites.h"
#include "config_update.h"
#include "storage.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAVORITES_KEY "favorites"

static char **favorite_cache=NULL;
static int favorite_cache_count=0;
static int favorite_cache_loaded=0;

static void free_items(char **items,int count){
    for(int i=0;i<count;i++)free(items[i]);
    free(items);
}

void stream_favorites_reload(void){
    free_items(favorite_cache,favorite_cache_count);
    favorite_cache=NULL;
    favorite_cache_count=0;
    favorite_cache_loaded=0;
}

static void load_cache(void){
    if(favorite_cache_loaded)return;
    favorite_cache_loaded=1;

    FILE *fp=fopen(get_storage_config_path(),"r");
    if(!fp)return;

    char line[4096];
    int in_streams=0;
    while(fgets(line,sizeof(line),fp)){
        util_trim(line);
        if(!line[0]||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){
            in_streams=!strcmp(line,"[streams]");
            continue;
        }
        if(!in_streams)continue;

        char *eq=strchr(line,'=');
        if(!eq)continue;
        *eq++='\0';
        util_trim(line);
        util_trim(eq);
        if(strcmp(line,FAVORITES_KEY))continue;

        char *save=NULL;
        for(char *tok=strtok_r(eq,",",&save);tok;tok=strtok_r(NULL,",",&save)){
            util_trim(tok);
            if(!tok[0])continue;
            char **grown=realloc(favorite_cache,(size_t)(favorite_cache_count+1)*sizeof(*favorite_cache));
            if(!grown)break;
            favorite_cache=grown;
            favorite_cache[favorite_cache_count]=strdup(tok);
            if(favorite_cache[favorite_cache_count])favorite_cache_count++;
        }
        break;
    }
    fclose(fp);
}

static int find_uuid(const char *uuid){
    if(!uuid||!uuid[0])return -1;
    load_cache();
    for(int i=0;i<favorite_cache_count;i++)
        if(!strcmp(favorite_cache[i],uuid))return i;
    return -1;
}

static int write_cache(void){
    size_t total=1;
    for(int i=0;i<favorite_cache_count;i++)total+=strlen(favorite_cache[i])+1;

    char *value=malloc(total);
    if(!value)return -1;
    value[0]='\0';
    for(int i=0;i<favorite_cache_count;i++){
        if(i)strcat(value,",");
        strcat(value,favorite_cache[i]);
    }

    ConfigUpdate update={FAVORITES_KEY,value};
    int rc=config_update_section(get_storage_config_path(),"streams",&update,1);
    free(value);
    return rc;
}

int stream_favorite_is_set(const char *uuid){
    return find_uuid(uuid)>=0;
}

int stream_favorite_toggle(const char *uuid){
    if(!uuid||!uuid[0])return -1;
    load_cache();

    int existing=find_uuid(uuid);
    if(existing>=0){
        free(favorite_cache[existing]);
        for(int i=existing;i<favorite_cache_count-1;i++)favorite_cache[i]=favorite_cache[i+1];
        favorite_cache_count--;
        if(favorite_cache_count==0){
            free(favorite_cache);
            favorite_cache=NULL;
        }
        if(write_cache()!=0)return -1;
        return 0;
    }

    char **grown=realloc(favorite_cache,(size_t)(favorite_cache_count+1)*sizeof(*favorite_cache));
    if(!grown)return -1;
    favorite_cache=grown;
    favorite_cache[favorite_cache_count]=strdup(uuid);
    if(!favorite_cache[favorite_cache_count])return -1;
    favorite_cache_count++;
    if(write_cache()!=0)return -1;
    return 1;
}
