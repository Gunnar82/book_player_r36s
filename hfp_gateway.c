#define _GNU_SOURCE
#include "hfp_gateway.h"
#include "app_log.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#define HFP_MSG_MAX 256

struct HfpGateway {
    int fd;
    char socket_path[sizeof(((struct sockaddr_un *)0)->sun_path)];
    char pending_dial[64];
    int dial_pending;
};

static int extract_number(const char *msg,char *out,size_t out_size)
{
    if(!msg||!out||out_size<2)return 0;
    const char *p=msg;
    while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;
    if(!strncmp(p,"DIAL ",5))p+=5;
    else if(!strncmp(p,"ATD",3))p+=3;
    else return 0;
    size_t n=0;
    while(*p&&*p!=';'&&*p!='\r'&&*p!='\n'&&n+1<out_size){
        if((*p>='0'&&*p<='9')||*p=='+'||*p=='#'||*p=='*')out[n++]=*p;
        p++;
    }
    out[n]='\0';
    return n>0;
}

int hfp_gateway_init(HfpGateway **out_gateway)
{
    if(!out_gateway)return -EINVAL;
    *out_gateway=NULL;
    HfpGateway *g=calloc(1,sizeof(*g));
    if(!g)return -ENOMEM;
    g->fd=-1;

    const char *runtime=getenv("XDG_RUNTIME_DIR");
    char fallback[64];
    if(!runtime||!runtime[0]){snprintf(fallback,sizeof(fallback),"/run/user/%ld",(long)getuid());runtime=fallback;}
    snprintf(g->socket_path,sizeof(g->socket_path),"%s/hoerspiel-player-hfp.sock",runtime);

    g->fd=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0);
    if(g->fd<0){int e=-errno;free(g);return e;}

    struct sockaddr_un sa;
    memset(&sa,0,sizeof(sa));sa.sun_family=AF_UNIX;
    snprintf(sa.sun_path,sizeof(sa.sun_path),"%s",g->socket_path);
    unlink(g->socket_path);
    if(bind(g->fd,(struct sockaddr*)&sa,sizeof(sa))<0){int e=-errno;close(g->fd);free(g);return e;}
    chmod(g->socket_path,0600);
    app_logf("HFP IPC bereit: %s",g->socket_path);
    *out_gateway=g;
    return 0;
}

void hfp_gateway_process(HfpGateway *g)
{
    if(!g||g->fd<0)return;
    char msg[HFP_MSG_MAX];
    for(;;){
        ssize_t n=recv(g->fd,msg,sizeof(msg)-1,0);
        if(n<0){if(errno==EINTR)continue;if(errno==EAGAIN||errno==EWOULDBLOCK)break;app_logf("HFP IPC Lesefehler: %d",errno);break;}
        msg[n]='\0';
        char number[64];
        if(extract_number(msg,number,sizeof(number))){
            snprintf(g->pending_dial,sizeof(g->pending_dial),"%s",number);
            g->dial_pending=1;
            app_logf("HFP Dial: %s",number);
        }else app_logf("HFP IPC unbekannt: %s",msg);
    }
}

int hfp_gateway_poll_dial(HfpGateway *g,char *number,size_t number_size)
{
    if(!g||!g->dial_pending)return 0;
    if(number&&number_size)snprintf(number,number_size,"%s",g->pending_dial);
    g->dial_pending=0;
    return 1;
}

const char *hfp_gateway_socket_path(const HfpGateway *g){return g?g->socket_path:"";}

void hfp_gateway_close(HfpGateway *g)
{
    if(!g)return;
    if(g->fd>=0)close(g->fd);
    if(g->socket_path[0])unlink(g->socket_path);
    free(g);
}
