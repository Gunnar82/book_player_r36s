#include "../util.h"
#include "../config_update.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures=0;

static void check(int ok,const char *name)
{
    if(ok)printf("OK   %s\n",name);
    else{printf("FAIL %s\n",name);failures++;}
}

static char *read_all(const char *path)
{
    FILE *fp=fopen(path,"rb");
    if(!fp)return NULL;
    if(fseek(fp,0,SEEK_END)!=0){fclose(fp);return NULL;}
    long len=ftell(fp);
    if(len<0){fclose(fp);return NULL;}
    rewind(fp);
    char *data=malloc((size_t)len+1);
    if(!data){fclose(fp);return NULL;}
    size_t got=fread(data,1,(size_t)len,fp);
    fclose(fp);
    data[got]='\0';
    return data;
}

int main(void)
{
    char s[64]="  hallo welt \t\n";
    util_trim(s);
    check(!strcmp(s,"hallo welt"),"util_trim");

    char dst[8];
    check(util_copy_checked(dst,sizeof(dst),"abc")==0&&!strcmp(dst,"abc"),"util_copy_checked ok");
    check(util_copy_checked(dst,sizeof(dst),"12345678")!=0&&dst[0]=='\0',"util_copy_checked overflow");

    char path[32];
    check(util_join_path_checked(path,sizeof(path),"/tmp","file")==0&&!strcmp(path,"/tmp/file"),"util_join_path_checked separator");
    check(util_join_path_checked(path,sizeof(path),"/tmp/","file")==0&&!strcmp(path,"/tmp/file"),"util_join_path_checked existing separator");

    char tmp[]="/tmp/hoerspiel-config-test-XXXXXX";
    int fd=mkstemp(tmp);
    check(fd>=0,"temp config create");
    if(fd>=0){
        const char *initial="[ui]\nvolume=50\n\n[bluetooth]\nautoconnect=0\ndevice=\n";
        check(write(fd,initial,strlen(initial))==(ssize_t)strlen(initial),"temp config seed");
        close(fd);

        const ConfigUpdate bt[]={
            {"autoconnect","1"},
            {"device","AA:BB:CC:DD:EE:FF"}
        };
        check(config_update_section(tmp,"bluetooth",bt,2)==0,"config update existing section");

        const ConfigUpdate streams[]={
            {"client_cert_mode","Downloads"},
            {"favorites","uuid-1,uuid-2"}
        };
        check(config_update_section(tmp,"streams",streams,2)==0,"config add section");

        char *data=read_all(tmp);
        check(data&&strstr(data,"[ui]\nvolume=50\n")!=NULL,"config preserves unrelated section");
        check(data&&strstr(data,"[bluetooth]")&&strstr(data,"autoconnect=1")&&strstr(data,"device=AA:BB:CC:DD:EE:FF"),"config replaces keys");
        check(data&&strstr(data,"[streams]")&&strstr(data,"client_cert_mode=Downloads")&&strstr(data,"favorites=uuid-1,uuid-2"),"config writes new section");
        free(data);
        unlink(tmp);
    }

    if(failures){
        fprintf(stderr,"%d Test(s) fehlgeschlagen\n",failures);
        return 1;
    }
    puts("Alle Tests erfolgreich.");
    return 0;
}
