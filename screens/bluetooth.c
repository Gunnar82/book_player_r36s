#include "bluetooth.h"
#include "../bluetooth.h"
#include "../ui.h"
#include "../app_log.h"
#include <stdio.h>
#include <string.h>

static BluetoothDevice devices[BT_MAX_DEVICES];
static int device_count=0;
static int selection=0;
static int message_until=0;
static char message[128]="";

static void refresh(void){device_count=bluetooth_scan_paired_trusted(devices,BT_MAX_DEVICES);if(selection>device_count)selection=device_count;}
void bluetoothscreen_reset(void){selection=0;message[0]='\0';message_until=0;refresh();}

static void set_message(const char *text){snprintf(message,sizeof(message),"%s",text?text:"");message_until=(int)SDL_GetTicks()+3000;}
static void move(int delta){int max=device_count;selection+=delta;if(selection<0)selection=max;if(selection>max)selection=0;}
static void toggle_auto(void){bluetooth_autoconnect=!bluetooth_autoconnect;if(bluetooth_save_config()!=0)set_message("config.ini konnte nicht gespeichert werden");else set_message(bluetooth_autoconnect?"Autoconnect: Ein":"Autoconnect: Aus");}

void bluetoothscreen_handle_event(ScreenContext *c,const SDL_Event *e)
{
    if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;
        if(b==BUTTON_B){*c->screen=SCREEN_SYSTEM_INFO;return;}
        if(b==BUTTON_DPAD_UP){move(-1);return;}
        if(b==BUTTON_DPAD_DOWN){move(1);return;}
        if(selection==0&&(b==BUTTON_A||b==BUTTON_DPAD_LEFT||b==BUTTON_DPAD_RIGHT)){toggle_auto();return;}
        if(b==BUTTON_A&&selection>0&&selection<=device_count){
            BluetoothDevice *d=&devices[selection-1];
            snprintf(bluetooth_device_mac,sizeof(bluetooth_device_mac),"%s",d->mac);
            if(bluetooth_save_config()!=0){set_message("Auswahl konnte nicht gespeichert werden");return;}
            set_message("Verbinde ...");
            int rc=bluetooth_connect_device(d->mac);refresh();
            set_message(rc==0?"Verbindung erfolgreich":"Verbindung fehlgeschlagen");return;
        }
    }
    if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_Y){
        if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){move(-1);*c->axis_y_lock=1;}
        else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){move(1);*c->axis_y_lock=1;}
        if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;
    }
}

void bluetoothscreen_render(ScreenContext *c)
{
    draw_text(c->renderer,c->font,"Bluetooth",20,20,c->selected);
    char buf[256];snprintf(buf,sizeof(buf),"Autoconnect beim Start: < %s >",bluetooth_autoconnect?"Ein":"Aus");
    draw_text(c->renderer,c->font,buf,35,65,selection==0?c->selected:c->white);
    int y=110;
    if(!bluetooth_adapter_present()){draw_text(c->renderer,c->font,"Kein Bluetooth-Adapter vorhanden",35,y,c->gray);}
    else if(device_count==0){draw_text(c->renderer,c->font,"Keine paired + trusted Geraete gefunden",35,y,c->gray);}
    int first=0;
    if(selection>7)first=selection-7;
    if(first>device_count-8)first=device_count>8?device_count-8:0;
    if(first<0)first=0;
    for(int i=first;i<device_count&&i<first+8;i++){
        BluetoothDevice *d=&devices[i];SDL_Color col=(selection==i+1)?c->selected:c->white;
        snprintf(buf,sizeof(buf),"%s%s%s",d->name,d->connected?"  [verbunden]":"",!strcmp(d->mac,bluetooth_device_mac)?"  *":"");draw_text(c->renderer,c->font,buf,35,y,col);
        draw_text(c->renderer,c->font,d->mac,55,y+22,c->gray);y+=46;
    }
    if(message[0]&&(Sint32)((Uint32)message_until-SDL_GetTicks())>0)draw_text(c->renderer,c->font,message,35,SCREEN_H-65,c->selected);
    draw_text(c->renderer,c->font,"A: Verbinden/Auswahl  Links/Rechts: Autoconnect  B: Zurueck",20,SCREEN_H-30,c->gray);
}
