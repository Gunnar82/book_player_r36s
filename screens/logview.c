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

void logview_render(ScreenContext *c)
{
    draw_text(c->renderer,c->font,"Programm-Log",20,15,c->selected);
    int n=app_log_count();
    int first=n-LOG_VISIBLE_LINES-scroll_from_bottom;if(first<0)first=0;
    int y=48;
    for(int i=first;i<n-scroll_from_bottom&&y<430;i++){
        draw_text(c->renderer,c->font,app_log_line(i),10,y,c->white);
        y+=22;
    }
    if(n==0)draw_text(c->renderer,c->font,"Noch keine Log-Eintraege.",20,90,c->gray);
    draw_text(c->renderer,c->font,"Hoch/Runter: Scroll  L1/R1: Seite  L2: Leeren  B: Zurueck",10,450,c->gray);
}
