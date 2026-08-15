#include "tracks.h"
#include "../scanner.h"
#include "../state.h"
#include "../audio.h"
#include "../ui.h"
#include <stdlib.h>

void tracks_handle_event(ScreenContext *c, const SDL_Event *e)
{
    if (*c->track_count > 0) { if (*c->track_index < 0) *c->track_index=*c->track_count-1; if (*c->track_index>=*c->track_count) *c->track_index=0; }
    if (e->type == SDL_JOYBUTTONDOWN) {
        int b=e->jbutton.button;
        if(b==BUTTON_B){*c->screen=SCREEN_MENU;return;}
        if(b==BUTTON_DPAD_UP){(*c->track_index)--;return;}
        if(b==BUTTON_DPAD_DOWN){(*c->track_index)++;return;}
        if(b==BUTTON_L1){*c->track_index-=LIST_PAGE_SIZE;if(*c->track_index<0)*c->track_index=0;return;}
        if(b==BUTTON_L2){*c->track_index=0;return;}
        if(b==BUTTON_R1){*c->track_index+=LIST_PAGE_SIZE;if(*c->track_index>=*c->track_count)*c->track_index=*c->track_count-1;return;}
        if(b==BUTTON_R2){*c->track_index=*c->track_count-1;return;}
        if(b==BUTTON_A && *c->track_count>0){
            int pi=ensure_book_progress(c->book_paths[*c->book_index]); double resume=0;
            if(pi>=0 && progress[pi].track==*c->track_index) resume=progress[pi].position;
            if(*c->music){Mix_HaltMusic();Mix_FreeMusic(*c->music);*c->music=NULL;}
            *c->music=play_track(c->tracks,*c->track_index,resume,c->base_position,c->started_ticks,c->paused);
            if(*c->music){touch_book_progress(pi);save_state();*c->duration=get_duration(*c->music);*c->last_save=SDL_GetTicks();*c->screen=SCREEN_PLAYER;}
            return;
        }
    }
    if(e->type==SDL_JOYAXISMOTION && e->jaxis.axis==AXIS_Y){
        if(!*c->axis_y_lock && e->jaxis.value<-AXIS_DEADZONE){(*c->track_index)--;*c->axis_y_lock=1;}
        else if(!*c->axis_y_lock && e->jaxis.value>AXIS_DEADZONE){(*c->track_index)++;*c->axis_y_lock=1;}
        if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;
    }
}

void tracks_render(ScreenContext *c)
{
    draw_text(c->renderer,c->font,c->book_names[*c->book_index],20,20,c->selected);
    const int top=60,bottom=SCREEN_H-60,row=28,visible=(bottom-top)/row;
    int start=0;if(*c->track_count>visible&&*c->track_index>=visible)start=*c->track_index-visible+1;
    int n=*c->track_count-start;if(n>visible)n=visible;int y=top;
    for(int i=0;i<n;i++){int t=start+i;draw_text(c->renderer,c->font,c->tracks[t].name,40,y,t==*c->track_index?c->selected:c->white);y+=row;}
    if(*c->track_count>visible){int h=bottom-top,thumb=(h*visible)/(*c->track_count);if(thumb<12)thumb=12;int range=*c->track_count-visible,travel=h-thumb,ty=top;if(range>0)ty+=(travel*start)/range;SDL_Rect r={SCREEN_W-8,ty,2,thumb};SDL_SetRenderDrawColor(c->renderer,230,210,70,255);SDL_RenderFillRect(c->renderer,&r);}
    draw_text(c->renderer,c->font,"A: Start   B: Zurueck   X: System   Y: Hoerspiele",20,SCREEN_H-55,c->gray);
    draw_text(c->renderer,c->font,"L1: Seite hoch  L2: Anfang  R1: Seite runter  R2: Ende",20,SCREEN_H-35,c->gray);
}
