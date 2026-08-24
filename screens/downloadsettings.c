#include "downloadsettings.h"
#include "../storage.h"
#include "../ui.h"
#include "../systemstats.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROW_H 32
#define TOP_Y 58
#define MAX_ROWS 11

typedef struct { const char *label; const char *value; int selectable; } Row;
static int selection=0;
static int scroll_offset=0;
static char message[128]="";
static Uint32 message_until=0;

static void set_message(const char *s){snprintf(message,sizeof(message),"%s",s?s:"");message_until=SDL_GetTicks()+3000;}
static int build_rows(Row *r){
    int n=0;
    static char enabled[32];
    snprintf(enabled,sizeof(enabled),"<  %s  >",downloads_enabled?"An":"Aus");
    r[n++]=(Row){"Downloads",enabled,1};
    r[n++]=(Row){"Base URL",download_base_url[0]?download_base_url:"--",0};
    r[n++]=(Row){"Zielpfad",download_target_path[0]?download_target_path:"--",0};
    r[n++]=(Row){"TLS Peer",download_verify_peer?"Pruefen":"Nicht pruefen",0};
    r[n++]=(Row){"TLS Host",download_verify_host?"Pruefen":"Nicht pruefen",0};
    r[n++]=(Row){"CA Zertifikat",download_ca_cert[0]?download_ca_cert:"System-CA",0};
    r[n++]=(Row){"Client Zertifikat",download_client_cert[0]?download_client_cert:"--",0};
    r[n++]=(Row){"Client Key",download_client_key[0]?download_client_key:"--",0};
    r[n++]=(Row){"Key Passwort",download_client_key_password[0]?"gesetzt":"nicht gesetzt",0};
    return n;
}
static void keep_visible(int count){if(selection<scroll_offset)scroll_offset=selection;if(selection>=scroll_offset+MAX_ROWS)scroll_offset=selection-MAX_ROWS+1;int max=count-MAX_ROWS;if(max<0)max=0;if(scroll_offset<0)scroll_offset=0;if(scroll_offset>max)scroll_offset=max;}
static void toggle_downloads(void){downloads_enabled=!downloads_enabled;if(save_download_enabled()!=0)set_message("config.ini konnte nicht gespeichert werden");else set_message(downloads_enabled?"Downloads: An":"Downloads: Aus");}
void downloadsettings_reset(void){selection=0;scroll_offset=0;message[0]='\0';message_until=0;}

void downloadsettings_handle_event(ScreenContext *c,const SDL_Event *e){
    Row rows[16];int count=build_rows(rows);
    if(e->type==SDL_JOYBUTTONDOWN){
        int b=e->jbutton.button;
        if(b==BUTTON_B){*c->screen=SCREEN_SYSTEM_INFO;return;}
        if(b==BUTTON_DPAD_UP){selection--;if(selection<0)selection=count-1;keep_visible(count);return;}
        if(b==BUTTON_DPAD_DOWN){selection++;if(selection>=count)selection=0;keep_visible(count);return;}
        if(selection==0&&(b==BUTTON_A||b==BUTTON_DPAD_LEFT||b==BUTTON_DPAD_RIGHT)){toggle_downloads();return;}
    }
    if(e->type==SDL_JOYAXISMOTION){
        if(e->jaxis.axis==AXIS_Y){
            if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){selection--;if(selection<0)selection=count-1;keep_visible(count);*c->axis_y_lock=1;}
            else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){selection++;if(selection>=count)selection=0;keep_visible(count);*c->axis_y_lock=1;}
            if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;
        }
        if(e->jaxis.axis==AXIS_X){
            if(selection==0&&!*c->axis_x_lock&&abs(e->jaxis.value)>AXIS_DEADZONE){toggle_downloads();*c->axis_x_lock=1;}
            if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_x_lock=0;
        }
    }
}

void downloadsettings_render(ScreenContext *c){
    Row rows[16];int count=build_rows(rows);
    draw_text(c->renderer,c->font,"Downloads",20,20,c->selected);
    int end=scroll_offset+MAX_ROWS;if(end>count)end=count;
    int y=TOP_Y;
    for(int i=scroll_offset;i<end;i++,y+=ROW_H){
        SDL_Color col=(i==selection)?c->selected:(rows[i].selectable?c->white:c->gray);
        char line[520];
        snprintf(line,sizeof(line),"%-18s %s",rows[i].label,rows[i].value);
        draw_text(c->renderer,c->font,line,25,y,col);
    }
    if(message[0]&&(Sint32)(message_until-SDL_GetTicks())>0)draw_text(c->renderer,c->font,message,25,SCREEN_H-62,c->selected);
    draw_text(c->renderer,c->font,"A/Links/Rechts: An/Aus   B: Zurueck",20,SCREEN_H-30,c->gray);
}
