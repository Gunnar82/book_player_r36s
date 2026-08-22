#include "streams.h"
#include "../streaming.h"
#include "../ui.h"
#include "../state.h"
#include <stdio.h>
#include <stdlib.h>

static StreamEntry *entries=NULL;
static int count=0,selection=0,loaded=0,loading=0,favorites_only=0;
static char status[256]="";

static int visible_count(void){
    if(!favorites_only)return count;
    int n=0;for(int i=0;i<count;i++)if(streaming_favorite_is_set(entries[i].uuid))n++;
    return n;
}
static int actual_index(int vi){
    if(!favorites_only)return vi;
    int n=0;for(int i=0;i<count;i++)if(streaming_favorite_is_set(entries[i].uuid)){if(n==vi)return i;n++;}
    return -1;
}
static void clamp_selection(void){
    int n=visible_count(); if(n<=0)selection=0; else if(selection>=n)selection=n-1; if(selection<0)selection=0;
}

void streams_reset(void){free(entries);entries=NULL;count=0;selection=0;loaded=0;loading=0;favorites_only=0;status[0]='\0';}
static void load_streams(void){
    loading=1;
    free(entries);
    entries=NULL;
    count=0;
    if(streaming_fetch_xml(&entries,&count,status,sizeof(status))!=0){
        free(entries);
        entries=NULL;
        count=0;
    }
    loaded=1;
    loading=0;
    clamp_selection();
}
void streams_handle_event(ScreenContext *c,const SDL_Event *e){
    if(!loaded&&!loading)load_streams();
    if(e->type!=SDL_JOYBUTTONDOWN)return;
    int b=e->jbutton.button,n=visible_count();
    if(b==BUTTON_B||b==BUTTON_DPAD_LEFT){*c->screen=SCREEN_SYSTEM_MENU;return;}
    if(b==BUTTON_X){favorites_only=!favorites_only;selection=0;status[0]='\0';return;}
    if(b==BUTTON_Y&&n>0){
        int ai=actual_index(selection);
        if(ai>=0&&entries[ai].uuid[0]){
            int r=streaming_favorite_toggle(entries[ai].uuid);
            snprintf(status,sizeof(status),r>0?"Favorit gesetzt":"Favorit entfernt");
            clamp_selection();
        }else snprintf(status,sizeof(status),"Keine stationuuid vorhanden");
        return;
    }
    if(b==BUTTON_DPAD_UP&&n){if(--selection<0)selection=n-1;return;}
    if(b==BUTTON_DPAD_DOWN&&n){if(++selection>=n)selection=0;return;}

    /* Wie im Downloads-Menue:
       L1/R1 = eine Seite zurueck/vor
       L2/R2 = Anfang/Ende der aktuellen Liste */
    if(b==BUTTON_L1&&n){
        int page=(SCREEN_H-145)/menu_line_height();
        if(page<1)page=1;
        selection-=page;
        if(selection<0)selection=0;
        return;
    }
    if(b==BUTTON_R1&&n){
        int page=(SCREEN_H-145)/menu_line_height();
        if(page<1)page=1;
        selection+=page;
        if(selection>=n)selection=n-1;
        return;
    }
    if(b==BUTTON_L2&&n){selection=0;return;}
    if(b==BUTTON_R2&&n){selection=n-1;return;}

    if((b==BUTTON_A||b==BUTTON_DPAD_RIGHT)&&n){
        int ai=actual_index(selection);if(ai<0)return;
        char err[256]="";
        if(streaming_start(&entries[ai],err,sizeof(err))==0){streaming_set_volume((volume*100)/128);*c->screen=SCREEN_PLAYER;}
        else snprintf(status,sizeof(status),"%s",err);
    }
}
void streams_render(ScreenContext *c){
    if(!loaded&&!loading)load_streams();
    menu_font_apply(c->font);
    char heading[128];snprintf(heading,sizeof(heading),"Streams - %s",favorites_only?"Favoriten":"Alle");
    draw_text(c->renderer,c->font,heading,20,20,c->selected);

    int n=visible_count();
    if(!n){
        draw_text(c->renderer,c->font,favorites_only?"Keine Favoriten":"Keine Streams",20,80,c->gray);
        if(status[0])draw_text(c->renderer,c->font,status,20,115,c->gray);
        draw_text(c->renderer,c->font,"X: Alle/Favoriten   Y: Favorit",20,SCREEN_H-55,c->gray);
        draw_text(c->renderer,c->font,"B/Links: Zurueck",20,SCREEN_H-30,c->gray);
        menu_font_restore(c->font);return;
    }

    int row=menu_line_height(),top=65,visible=(SCREEN_H-145)/row;if(visible<1)visible=1;
    int start=selection>=visible?selection-visible+1:0,y=top;
    for(int vi=start;vi<n&&vi<start+visible;vi++,y+=row){
        int ai=actual_index(vi);if(ai<0)continue;
        char label[STREAM_NAME_LEN+8];
        snprintf(label,sizeof(label),"%s%s",streaming_favorite_is_set(entries[ai].uuid)?"* ":"",entries[ai].name);
        draw_text(c->renderer,c->font,label,35,y,vi==selection?c->selected:c->white);
    }
    char info[256];snprintf(info,sizeof(info),"%d/%d  Y: Favorit  X: %s",selection+1,n,favorites_only?"Alle":"Favoriten");
    if(status[0])draw_text(c->renderer,c->font,status,20,SCREEN_H-82,c->gray);
    draw_text(c->renderer,c->font,info,20,SCREEN_H-55,c->gray);
    draw_text(c->renderer,c->font,"A/Rechts: Start  L1/R1: Seite  L2/R2: Anfang/Ende",20,SCREEN_H-30,c->gray);
    menu_font_restore(c->font);
}
