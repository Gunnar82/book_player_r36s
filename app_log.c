#include "app_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static char lines[APP_LOG_MAX_LINES][APP_LOG_LINE_LEN];
static int start_index=0;
static int line_count=0;

void app_log_clear(void)
{
    start_index=0;
    line_count=0;
    memset(lines,0,sizeof(lines));
}

void app_logf(const char *fmt,...)
{
    if(!fmt)return;
    char msg[APP_LOG_LINE_LEN-16];
    va_list ap;va_start(ap,fmt);vsnprintf(msg,sizeof(msg),fmt,ap);va_end(ap);
    time_t t=time(NULL);struct tm tmv;
    localtime_r(&t,&tmv);
    int idx;
    if(line_count<APP_LOG_MAX_LINES){idx=(start_index+line_count)%APP_LOG_MAX_LINES;line_count++;}
    else{idx=start_index;start_index=(start_index+1)%APP_LOG_MAX_LINES;}
    snprintf(lines[idx],APP_LOG_LINE_LEN,"%02d:%02d:%02d %s",tmv.tm_hour,tmv.tm_min,tmv.tm_sec,msg);
    fprintf(stderr,"%s\n",lines[idx]);
}

int app_log_count(void){return line_count;}

const char *app_log_line(int index)
{
    if(index<0||index>=line_count)return "";
    return lines[(start_index+index)%APP_LOG_MAX_LINES];
}
