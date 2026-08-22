#include "input_config.h"
#include "config.h"
#include "storage.h"
#include "app_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct {
    int custom;
    int axis_mode;
    int axis_x;
    int axis_y;
    int deadzone;
    int a,b,x,y,l1,r1,l2,r2,start,select;
    int dpad_up,dpad_down,dpad_left,dpad_right;
    int axis_x_active;
    int axis_y_active;
} InputConfig;

static InputConfig cfg;
#define REPEAT_EVENT_WHICH ((SDL_JoystickID)-4242)
static int repeat_button=-1;
static Uint32 repeat_started=0;
static Uint32 repeat_next=0;

static void repeat_press(int button,Uint32 now){
    if(button!=BUTTON_DPAD_UP&&button!=BUTTON_DPAD_DOWN)return;
    if(repeat_button!=button){
        repeat_button=button;
        repeat_started=now;
        repeat_next=now+400U;
    }
}
static void repeat_release(int button){
    if(button==repeat_button){
        repeat_button=-1;
        repeat_started=0;
        repeat_next=0;
    }
}


static void trim(char *s)
{
    char *p=s;
    while(*p && isspace((unsigned char)*p))p++;
    if(p!=s)memmove(s,p,strlen(p)+1);
    size_t n=strlen(s);
    while(n && isspace((unsigned char)s[n-1]))s[--n]='\0';
}

static void defaults(void)
{
    memset(&cfg,0,sizeof(cfg));
    cfg.axis_x=0;
    cfg.axis_y=1;
    cfg.deadzone=16000;
    cfg.a=BUTTON_A; cfg.b=BUTTON_B; cfg.x=BUTTON_X; cfg.y=BUTTON_Y;
    cfg.l1=BUTTON_L1; cfg.r1=BUTTON_R1; cfg.l2=BUTTON_L2; cfg.r2=BUTTON_R2;
    cfg.start=BUTTON_START; cfg.select=BUTTON_SELECT;
    cfg.dpad_up=BUTTON_DPAD_UP; cfg.dpad_down=BUTTON_DPAD_DOWN; cfg.dpad_left=BUTTON_DPAD_LEFT; cfg.dpad_right=BUTTON_DPAD_RIGHT;
}

static void set_int(const char *key,const char *value)
{
    int v=atoi(value);
    if(!strcmp(key,"dpad_x_axis"))cfg.axis_x=v;
    else if(!strcmp(key,"dpad_y_axis"))cfg.axis_y=v;
    else if(!strcmp(key,"dpad_deadzone"))cfg.deadzone=v;
    else if(!strcmp(key,"dpad_up"))cfg.dpad_up=v;
    else if(!strcmp(key,"dpad_down"))cfg.dpad_down=v;
    else if(!strcmp(key,"dpad_left"))cfg.dpad_left=v;
    else if(!strcmp(key,"dpad_right"))cfg.dpad_right=v;
    else if(!strcmp(key,"a"))cfg.a=v;
    else if(!strcmp(key,"b"))cfg.b=v;
    else if(!strcmp(key,"x"))cfg.x=v;
    else if(!strcmp(key,"y"))cfg.y=v;
    else if(!strcmp(key,"l1"))cfg.l1=v;
    else if(!strcmp(key,"r1"))cfg.r1=v;
    else if(!strcmp(key,"l2"))cfg.l2=v;
    else if(!strcmp(key,"r2"))cfg.r2=v;
    else if(!strcmp(key,"start"))cfg.start=v;
    else if(!strcmp(key,"select"))cfg.select=v;
}

void input_config_load(void)
{
    defaults();
    FILE *fp=fopen(get_storage_config_path(),"r");
    if(!fp){app_logf("Input: config.ini nicht lesbar, R36S-Standard");return;}
    int in_input=0;
    char line[512];
    while(fgets(line,sizeof(line),fp)){
        trim(line);
        if(!line[0]||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){in_input=!strcmp(line,"[input]");continue;}
        if(!in_input)continue;
        char *eq=strchr(line,'=');if(!eq)continue;*eq++='\0';trim(line);trim(eq);
        if(!strcmp(line,"profile")){cfg.custom=!strcasecmp(eq,"custom");continue;}
        if(!strcmp(line,"dpad_mode")){cfg.axis_mode=!strcasecmp(eq,"axis");continue;}
        set_int(line,eq);
    }
    fclose(fp);
    if(cfg.deadzone<1000)cfg.deadzone=1000;
    if(cfg.deadzone>32000)cfg.deadzone=32000;
    if(!cfg.custom)cfg.axis_mode=0;
    app_logf("Input: Profil %s, D-Pad %s",cfg.custom?"custom":"r36s",cfg.axis_mode?"axis":"buttons");
}

int input_config_is_custom(void){return cfg.custom;}

static int remap_button(int physical)
{
    if(!cfg.custom)return physical;
    if(!cfg.axis_mode){
        if(physical==cfg.dpad_up)return BUTTON_DPAD_UP;
        if(physical==cfg.dpad_down)return BUTTON_DPAD_DOWN;
        if(physical==cfg.dpad_left)return BUTTON_DPAD_LEFT;
        if(physical==cfg.dpad_right)return BUTTON_DPAD_RIGHT;
    }
    if(physical==cfg.a)return BUTTON_A;
    if(physical==cfg.b)return BUTTON_B;
    if(physical==cfg.x)return BUTTON_X;
    if(physical==cfg.y)return BUTTON_Y;
    if(cfg.l1>=0&&physical==cfg.l1)return BUTTON_L1;
    if(cfg.r1>=0&&physical==cfg.r1)return BUTTON_R1;
    if(cfg.l2>=0&&physical==cfg.l2)return BUTTON_L2;
    if(cfg.r2>=0&&physical==cfg.r2)return BUTTON_R2;
    if(cfg.start>=0&&physical==cfg.start)return BUTTON_START;
    if(cfg.select>=0&&physical==cfg.select)return BUTTON_SELECT;
    return -1;
}

static void make_button(SDL_Event *e,Uint8 button)
{
    Uint32 timestamp=e->jaxis.timestamp;
    SDL_JoystickID which=e->jaxis.which;
    memset(e,0,sizeof(*e));
    e->type=SDL_JOYBUTTONDOWN;
    e->jbutton.type=SDL_JOYBUTTONDOWN;
    e->jbutton.timestamp=timestamp;
    e->jbutton.which=which;
    e->jbutton.button=button;
    e->jbutton.state=SDL_PRESSED;
}


int input_repeat_event(SDL_Event *e,Uint32 now,int enabled)
{
    if(!enabled||repeat_button<0)return 0;
    if((Sint32)(now-repeat_next)<0)return 0;

    Uint32 held=now-repeat_started;
    Uint32 interval=(held>=5000U)?70U:160U;
    repeat_next=now+interval;

    memset(e,0,sizeof(*e));
    e->type=SDL_JOYBUTTONDOWN;
    e->jbutton.type=SDL_JOYBUTTONDOWN;
    e->jbutton.which=REPEAT_EVENT_WHICH;
    e->jbutton.button=(Uint8)repeat_button;
    e->jbutton.state=SDL_PRESSED;
    return 1;
}

int input_normalize_event(SDL_Event *e)
{
    if(!e)return 0;
    if(e->type==SDL_JOYBUTTONDOWN||e->type==SDL_JOYBUTTONUP){
        if(e->jbutton.which==REPEAT_EVENT_WHICH)return 1;
        int b=remap_button(e->jbutton.button);
        if(b<0)return cfg.custom?0:1;
        e->jbutton.button=(Uint8)b;
        if(e->type==SDL_JOYBUTTONDOWN)repeat_press(b,SDL_GetTicks());
        else repeat_release(b);
        return 1;
    }
    if(e->type!=SDL_JOYAXISMOTION)return 1;
    if(!cfg.custom||!cfg.axis_mode)return 0; /* R36S: Achsen weiterhin komplett ignorieren */

    int axis=e->jaxis.axis;
    int value=e->jaxis.value;
    if(axis==cfg.axis_x){
        if(abs(value)<cfg.deadzone/2){cfg.axis_x_active=0;return 0;}
        if(cfg.axis_x_active)return 0;
        if(value<=-cfg.deadzone){cfg.axis_x_active=1;make_button(e,BUTTON_DPAD_LEFT);return 1;}
        if(value>= cfg.deadzone){cfg.axis_x_active=1;make_button(e,BUTTON_DPAD_RIGHT);return 1;}
        return 0;
    }
    if(axis==cfg.axis_y){
        if(abs(value)<cfg.deadzone/2){repeat_release(BUTTON_DPAD_UP);repeat_release(BUTTON_DPAD_DOWN);cfg.axis_y_active=0;return 0;}
        if(cfg.axis_y_active)return 0;
        if(value<=-cfg.deadzone){cfg.axis_y_active=1;make_button(e,BUTTON_DPAD_UP);repeat_press(BUTTON_DPAD_UP,SDL_GetTicks());return 1;}
        if(value>= cfg.deadzone){cfg.axis_y_active=1;make_button(e,BUTTON_DPAD_DOWN);repeat_press(BUTTON_DPAD_DOWN,SDL_GetTicks());return 1;}
        return 0;
    }
    return 0;
}
