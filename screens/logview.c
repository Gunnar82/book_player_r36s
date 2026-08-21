#include "logview.h"
#include "../app_log.h"
#include "../ui.h"
#include <stdlib.h>

static int scroll_from_bottom=0;
#define LOG_VISIBLE_LINES 17

void logview_reset(void){scroll_from_bottom=0;}

static int max_scroll(void)
{
    int n=app_log_count();
    return n>LOG_VISIBLE_LINES?n-LOG_VISIBLE_LINES:0;
}

static void move_scroll(int delta)
{
    scroll_from_bottom+=delta;
    int m=max_scroll();
    if(scroll_from_bottom<0)scroll_from_bottom=0;
    if(scroll_from_bottom>m)scroll_from_bottom=m;
}

void logview_handle_event(ScreenContext *c,const SDL_Event *e)
{
    if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;
        if(b==BUTTON_B){*c->screen=SCREEN_SYSTEM_MENU;return;}
        if(b==BUTTON_DPAD_UP){move_scroll(1);return;}
        if(b==BUTTON_DPAD_DOWN){move_scroll(-1);return;}
        if(b==BUTTON_L1){move_scroll(8);return;}
        if(b==BUTTON_R1){move_scroll(-8);return;}
        if(b==BUTTON_L2){app_log_clear();scroll_from_bottom=0;return;}
    }
    if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_Y){
        if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){move_scroll(1);*c->axis_y_lock=1;}
        else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){move_scroll(-1);*c->axis_y_lock=1;}
        if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;
    }
}

static const char *screen_log_text(const char *line)
{
    if(!line)return "";
    /* Zeitstempel HH:MM:SS nur auf dem kleinen Display ausblenden.
       Im stderr-Log bleibt er erhalten. */
    if(strlen(line)>9 &&
       line[2]==':' && line[5]==':' && line[8]==' ')
        return line+9;
    return line;
}

static int wrapped_line_count(const char *s)
{
    if(!s||!*s)return 1;
    const int max_chars=54;
    int len=(int)strlen(s);
    return (len+max_chars-1)/max_chars;
}

static void draw_wrapped(ScreenContext *c,const char *s,int *y)
{
    const int max_chars=54;
    char part[64];
    if(!s||!*s){draw_text(c->renderer,c->font,"",10,*y,c->white);*y+=22;return;}
    while(*s&&*y<430){
        int remaining=(int)strlen(s);
        int take=remaining>max_chars?max_chars:remaining;
        if(remaining>max_chars){
            int split=take;
            while(split>28 && s[split]!=' ' && s[split]!='/' && s[split]!=':' && s[split]!='-')split--;
            if(split>28)take=split+1;
        }
        while(take>0&&s[take-1]==' ')take--;
        if(take<=0)take=remaining>max_chars?max_chars:remaining;
        memcpy(part,s,(size_t)take);part[take]='\0';
        draw_text(c->renderer,c->font,part,10,*y,c->white);
        *y+=22;
        s+=take;while(*s==' ')s++;
    }
}

void logview_render(ScreenContext *c)
{
    draw_text(c->renderer,c->font,"Programm-Log",20,15,c->selected);
    int n=app_log_count();
    int end=n-scroll_from_bottom;if(end<0)end=0;
    int first=end;
    int used=0;
    while(first>0){
        const char *s=screen_log_text(app_log_line(first-1));
        int need=wrapped_line_count(s);
        if(used+need>LOG_VISIBLE_LINES)break;
        used+=need;first--;
    }

    int y=48;
    for(int i=first;i<end&&y<430;i++)
        draw_wrapped(c,screen_log_text(app_log_line(i)),&y);

    if(n==0)draw_text(c->renderer,c->font,"Noch keine Log-Eintraege.",20,90,c->gray);
    draw_text(c->renderer,c->font,"Hoch/Runter: Scroll  L1/R1: Seite  L2: Leeren  B: Zurueck",10,450,c->gray);
}
