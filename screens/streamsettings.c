#include "streamsettings.h"
#include "../streaming.h"
#include "../ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROW_H 32
#define TOP_Y 58
#define MAX_ROWS 11

typedef struct { const char *label; const char *value; int selectable; } Row;

static int selection=0;
static int scroll_offset=0;
static char message[160]="";
static Uint32 message_until=0;

static void set_message(const char *s){
    snprintf(message,sizeof(message),"%s",s?s:"");
    message_until=SDL_GetTicks()+3000U;
}

static int build_rows(Row *r){
    int n=0;
    static char cert_mode[64];
    static char backend[64];

    snprintf(cert_mode,sizeof(cert_mode),"<  %s  >",streaming_cert_mode_name());
    snprintf(backend,sizeof(backend),"%s",streaming_backend_name());

    r[n++]=(Row){"XML Quelle",stream_xml_url[0]?stream_xml_url:"--",0};
    r[n++]=(Row){"Zertifikat",cert_mode,1};
    r[n++]=(Row){"Backend",backend,0};

    if(stream_cert_mode==STREAM_CERT_DOWNLOADS){
        r[n++]=(Row){"TLS Daten","aus Downloads",0};
    }else if(stream_cert_mode==STREAM_CERT_SEPARATE){
        r[n++]=(Row){"CA Zertifikat",stream_ca_cert[0]?stream_ca_cert:"System-CA",0};
        r[n++]=(Row){"Client Zertifikat",stream_client_cert[0]?stream_client_cert:"--",0};
        r[n++]=(Row){"Client Key",stream_client_key[0]?stream_client_key:"--",0};
        r[n++]=(Row){"Key Passwort",stream_client_key_password[0]?"gesetzt":"nicht gesetzt",0};
    }else{
        r[n++]=(Row){"TLS Daten","keine Client-Zertifikate",0};
    }

    return n;
}

static void keep_visible(int count){
    if(selection<0)selection=0;
    if(selection>=count)selection=count-1;
    if(selection<scroll_offset)scroll_offset=selection;
    if(selection>=scroll_offset+MAX_ROWS)scroll_offset=selection-MAX_ROWS+1;
    int max=count-MAX_ROWS;
    if(max<0)max=0;
    if(scroll_offset<0)scroll_offset=0;
    if(scroll_offset>max)scroll_offset=max;
}

static void move_selection(int delta){
    Row rows[16];
    int count=build_rows(rows);
    if(count<=0)return;

    int next=selection;
    do{
        next+=delta;
        if(next<0)next=count-1;
        if(next>=count)next=0;
    }while(!rows[next].selectable && next!=selection);

    selection=next;
    keep_visible(count);
}

static void change_cert_mode(int direction){
    int old=stream_cert_mode;
    if(direction<0)stream_cert_mode=(stream_cert_mode+2)%3;
    else stream_cert_mode=(stream_cert_mode+1)%3;

    if(streaming_save_cert_mode()!=0){
        stream_cert_mode=old;
        set_message("config.ini konnte nicht gespeichert werden");
    }else{
        char msg[96];
        snprintf(msg,sizeof(msg),"Zertifikat: %s",streaming_cert_mode_name());
        set_message(msg);
    }

    Row rows[16];
    keep_visible(build_rows(rows));
}

void streamsettings_reset(void){
    selection=1; /* Zertifikatsmodus ist die einzige editierbare Zeile. */
    scroll_offset=0;
    message[0]='\0';
    message_until=0;
}

void streamsettings_handle_event(ScreenContext *c,const SDL_Event *e){
    Row rows[16];
    int count=build_rows(rows);
    keep_visible(count);

    if(e->type==SDL_JOYBUTTONDOWN){
        int b=e->jbutton.button;

        if(b==BUTTON_B||(b==BUTTON_DPAD_LEFT&&!rows[selection].selectable)){
            *c->screen=SCREEN_SYSTEM_INFO;
            return;
        }

        if(b==BUTTON_DPAD_UP){move_selection(-1);return;}
        if(b==BUTTON_DPAD_DOWN){move_selection(1);return;}

        if(rows[selection].selectable){
            if(b==BUTTON_DPAD_LEFT){change_cert_mode(-1);return;}
            if(b==BUTTON_DPAD_RIGHT||b==BUTTON_A){change_cert_mode(1);return;}
        }

        if(b==BUTTON_B){
            *c->screen=SCREEN_SYSTEM_INFO;
            return;
        }
    }

    if(e->type==SDL_JOYAXISMOTION){
        if(e->jaxis.axis==AXIS_Y){
            if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){
                move_selection(-1);
                *c->axis_y_lock=1;
            }else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){
                move_selection(1);
                *c->axis_y_lock=1;
            }
            if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;
        }

        if(e->jaxis.axis==AXIS_X){
            if(rows[selection].selectable&&!*c->axis_x_lock){
                if(e->jaxis.value<-AXIS_DEADZONE){
                    change_cert_mode(-1);
                    *c->axis_x_lock=1;
                }else if(e->jaxis.value>AXIS_DEADZONE){
                    change_cert_mode(1);
                    *c->axis_x_lock=1;
                }
            }
            if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_x_lock=0;
        }
    }
}

void streamsettings_render(ScreenContext *c){
    Row rows[16];
    int count=build_rows(rows);
    keep_visible(count);

    draw_text(c->renderer,c->font,"Streams",20,20,c->selected);

    int end=scroll_offset+MAX_ROWS;
    if(end>count)end=count;

    int y=TOP_Y;
    for(int i=scroll_offset;i<end;i++,y+=ROW_H){
        SDL_Color col=(i==selection)?c->selected:(rows[i].selectable?c->white:c->gray);
        char line[600];
        snprintf(line,sizeof(line),"%-18s %s",rows[i].label,rows[i].value);
        draw_text(c->renderer,c->font,line,25,y,col);
    }

    if(message[0]&&(Sint32)(message_until-SDL_GetTicks())>0)
        draw_text(c->renderer,c->font,message,25,SCREEN_H-62,c->selected);

    draw_text(c->renderer,c->font,
              "Links/Rechts/A: Zertifikat   B: Zurueck",
              20,SCREEN_H-30,c->gray);
}
