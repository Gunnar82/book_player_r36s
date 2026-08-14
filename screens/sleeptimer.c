#include "sleeptimer.h"
#include "../ui.h"
#include <stdlib.h>

void sleeptimer_handle_event(ScreenContext *c,const SDL_Event *e){
 if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;if(b==BUTTON_B){*c->screen=SCREEN_MENU;return;}if(b==BUTTON_A){if(*c->sleep_timer_minutes==0){*c->sleep_timer_active=0;*c->sleep_timer_end_ticks=0;}else{*c->sleep_timer_active=1;*c->sleep_timer_end_ticks=SDL_GetTicks()+(Uint32)(*c->sleep_timer_minutes*60000);}*c->screen=SCREEN_MENU;return;}if(b==BUTTON_DPAD_LEFT){*c->sleep_timer_minutes-=SLEEP_STEP_MINUTES;return;}if(b==BUTTON_DPAD_RIGHT){*c->sleep_timer_minutes+=SLEEP_STEP_MINUTES;return;}}
 if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_X){if(!*c->axis_x_lock&&e->jaxis.value<-AXIS_DEADZONE){*c->sleep_timer_minutes-=SLEEP_STEP_MINUTES;*c->axis_x_lock=1;}else if(!*c->axis_x_lock&&e->jaxis.value>AXIS_DEADZONE){*c->sleep_timer_minutes+=SLEEP_STEP_MINUTES;*c->axis_x_lock=1;}if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_x_lock=0;}
 if(*c->sleep_timer_minutes<SLEEP_MIN_MINUTES)*c->sleep_timer_minutes=SLEEP_MIN_MINUTES;if(*c->sleep_timer_minutes>SLEEP_MAX_MINUTES)*c->sleep_timer_minutes=SLEEP_MAX_MINUTES;
}
void sleeptimer_render(ScreenContext *c){
 draw_text(c->renderer,c->font,"Sleeptimer einstellen",20,20,c->selected);char s[32];snprintf(s,sizeof(s),"%d Minuten",*c->sleep_timer_minutes);draw_text(c->renderer,c->font,s,20,100,c->white);draw_text(c->renderer,c->font,"Links/Rechts: -/+2min   A: Starten   B: Zurueck",20,SCREEN_H-35,c->gray);
}
