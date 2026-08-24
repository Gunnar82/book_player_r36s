#include "config_update.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int find_update(const ConfigUpdate *updates,size_t count,const char *key)
{
    for(size_t i=0;i<count;i++)
        if(updates[i].key&&strcmp(updates[i].key,key)==0)return (int)i;
    return -1;
}

int config_update_section(const char *path,
                          const char *section,
                          const ConfigUpdate *updates,
                          size_t update_count)
{
    if(!path||!section||(!updates&&update_count))return -1;

    FILE *in=fopen(path,"r");
    char **lines=NULL;
    size_t count=0,cap=0;
    char line[4096];

    if(in){
        while(fgets(line,sizeof(line),in)){
            if(count==cap){
                size_t next=cap?cap*2:32;
                char **grown=realloc(lines,next*sizeof(*grown));
                if(!grown){fclose(in);goto fail;}
                lines=grown;cap=next;
            }
            lines[count]=strdup(line);
            if(!lines[count]){fclose(in);goto fail;}
            count++;
        }
        fclose(in);
    }

    char tmp_path[1200];
    if(snprintf(tmp_path,sizeof(tmp_path),"%s.tmp",path)>=(int)sizeof(tmp_path))goto fail;
    FILE *out=fopen(tmp_path,"w");
    if(!out)goto fail;

    unsigned char *written=calloc(update_count?update_count:1,1);
    if(!written){fclose(out);unlink(tmp_path);goto fail;}

    int in_section=0,section_seen=0;
    char wanted[256];
    if(snprintf(wanted,sizeof(wanted),"[%s]",section)>=(int)sizeof(wanted)){
        free(written);fclose(out);unlink(tmp_path);goto fail;
    }

    for(size_t i=0;i<count;i++){
        char check[4096];
        if(util_copy_checked(check,sizeof(check),lines[i])!=0)check[0]='\0';
        util_trim(check);

        if(check[0]=='['){
            if(in_section){
                for(size_t u=0;u<update_count;u++)
                    if(!written[u])fprintf(out,"%s=%s\n",updates[u].key,updates[u].value?updates[u].value:"");
            }
            in_section=strcmp(check,wanted)==0;
            if(in_section)section_seen=1;
            fputs(lines[i],out);
            continue;
        }

        if(in_section){
            char *eq=strchr(check,'=');
            if(eq){
                *eq='\0';
                util_trim(check);
                int u=find_update(updates,update_count,check);
                if(u>=0){
                    fprintf(out,"%s=%s\n",updates[u].key,updates[u].value?updates[u].value:"");
                    written[u]=1;
                    continue;
                }
            }
        }
        fputs(lines[i],out);
    }

    if(in_section){
        for(size_t u=0;u<update_count;u++)
            if(!written[u])fprintf(out,"%s=%s\n",updates[u].key,updates[u].value?updates[u].value:"");
    }else if(!section_seen){
        if(count&&lines[count-1][0]&&lines[count-1][strlen(lines[count-1])-1]!='\n')fputc('\n',out);
        fprintf(out,"\n[%s]\n",section);
        for(size_t u=0;u<update_count;u++)fprintf(out,"%s=%s\n",updates[u].key,updates[u].value?updates[u].value:"");
    }

    free(written);
    if(fflush(out)!=0||fsync(fileno(out))!=0){fclose(out);unlink(tmp_path);goto fail;}
    if(fclose(out)!=0){unlink(tmp_path);goto fail;}
    if(rename(tmp_path,path)!=0){unlink(tmp_path);goto fail;}

    for(size_t i=0;i<count;i++)free(lines[i]);
    free(lines);
    return 0;

fail:
    for(size_t i=0;i<count;i++)free(lines[i]);
    free(lines);
    return -1;
}
