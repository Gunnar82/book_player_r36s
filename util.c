#include "util.h"

#include <ctype.h>
#include <string.h>

void util_trim(char *s)
{
    if(!s)return;
    char *start=s;
    while(*start&&isspace((unsigned char)*start))start++;
    if(start!=s)memmove(s,start,strlen(start)+1);
    size_t len=strlen(s);
    while(len>0&&isspace((unsigned char)s[len-1]))s[--len]='\0';
}

int util_copy_checked(char *dst,size_t dst_size,const char *src)
{
    if(!dst||dst_size==0)return -1;
    if(!src){dst[0]='\0';return 0;}
    size_t n=strlen(src);
    if(n>=dst_size){dst[0]='\0';return -1;}
    memcpy(dst,src,n+1);
    return 0;
}

int util_join_path_checked(char *dst,size_t dst_size,const char *a,const char *b)
{
    if(!dst||dst_size==0||!a||!b)return -1;
    size_t al=strlen(a),bl=strlen(b);
    size_t sep=(al>0&&a[al-1]!='/')?1:0;
    if(al+sep+bl+1>dst_size){dst[0]='\0';return -1;}
    memcpy(dst,a,al);
    size_t pos=al;
    if(sep)dst[pos++]='/';
    memcpy(dst+pos,b,bl);
    dst[pos+bl]='\0';
    return 0;
}
