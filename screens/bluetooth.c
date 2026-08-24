#include "bluetooth.h"
#include "../bluetooth.h"
#include "../bluetooth_discovery.h"
#include "../bluetooth_agent.h"
#include "../ui.h"
#include "../app_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BluetoothDevice devices[BT_MAX_DEVICES];
static int device_count=0;
static int selection=0;
static int message_until=0;
static char message[128]="";
static int discovery_mode=0;
static Uint32 last_discovery_refresh=0;

static void refresh(void){
    if(!bluetooth_adapter_powered()){
        device_count=0;
        selection=0;
        return;
    }
    device_count=discovery_mode?bluetooth_scan_all(devices,BT_MAX_DEVICES):bluetooth_scan_paired_trusted(devices,BT_MAX_DEVICES);
    if(selection>device_count+1)selection=device_count+1;
}

void bluetoothscreen_reset(void){
    if(discovery_mode)bluetooth_discovery_stop();
    bluetooth_agent_stop();
    discovery_mode=0;
    selection=0;
    message[0]='\0';
    message_until=0;
    last_discovery_refresh=0;
    bluetooth_log_if_changed();
    refresh();
}

static void set_message(const char *text){snprintf(message,sizeof(message),"%s",text?text:"");message_until=(int)SDL_GetTicks()+3000;}
static void move(int delta){int max=device_count+1;selection+=delta;if(selection<0)selection=max;if(selection>max)selection=0;}
static void toggle_auto(void){bluetooth_autoconnect=!bluetooth_autoconnect;if(bluetooth_save_config()!=0)set_message("config.ini konnte nicht gespeichert werden");else set_message(bluetooth_autoconnect?"Autoconnect: Ein":"Autoconnect: Aus");}

static void toggle_discovery(void)
{
    if(discovery_mode){
        bluetooth_discovery_stop();
        bluetooth_agent_stop();
        discovery_mode=0;
        set_message("Suche beendet");
    }else{
        if(bluetooth_agent_start()!=0){set_message("Bluetooth-Agent konnte nicht gestartet werden");return;}
        if(bluetooth_discovery_start()!=0){bluetooth_agent_stop();set_message("Suche konnte nicht gestartet werden");return;}
        discovery_mode=1;
        last_discovery_refresh=0;
        set_message("Suche gestartet");
    }
    refresh();
}

void bluetoothscreen_handle_event(ScreenContext *c,const SDL_Event *e)
{
    if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;
        if(b==BUTTON_B){if(discovery_mode)bluetooth_discovery_stop();bluetooth_agent_stop();discovery_mode=0;*c->screen=SCREEN_SYSTEM_INFO;return;}
        if(b==BUTTON_DPAD_UP){move(-1);return;}
        if(b==BUTTON_DPAD_DOWN){move(1);return;}
        if(selection==0&&(b==BUTTON_A||b==BUTTON_DPAD_LEFT||b==BUTTON_DPAD_RIGHT)){toggle_auto();return;}
        if(selection==1&&b==BUTTON_A){toggle_discovery();return;}
        if(b==BUTTON_A&&selection>=2&&selection<device_count+2){
            BluetoothDevice *d=&devices[selection-2];
            if(discovery_mode&&!d->paired){set_message("Noch nicht gekoppelt - Pairing folgt in Schritt 2");return;}
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
    bluetooth_agent_process();
    if(discovery_mode&&SDL_GetTicks()-last_discovery_refresh>=1000U){refresh();last_discovery_refresh=SDL_GetTicks();}

    draw_text(c->renderer,c->font,"Bluetooth",20,20,c->selected);
    char buf[256];snprintf(buf,sizeof(buf),"Autoconnect beim Start: < %s >",bluetooth_autoconnect?"Ein":"Aus");
    draw_text(c->renderer,c->font,buf,35,65,selection==0?c->selected:c->white);
    snprintf(buf,sizeof(buf),"Neue Geraete suchen: %s",discovery_mode?"[Suche laeuft]":"Start");
    draw_text(c->renderer,c->font,buf,35,92,selection==1?c->selected:c->white);

    int y=130;
    if(!bluetooth_adapter_present()){
        draw_text(c->renderer,c->font,"Kein Bluetooth-Adapter vorhanden",35,y,c->gray);
    }else if(!bluetooth_adapter_powered()){
        draw_text(c->renderer,c->font,"Bluetooth ist ausgeschaltet",35,y,c->gray);
    }else if(device_count==0){
        draw_text(c->renderer,c->font,discovery_mode?"Suche nach Geraeten ...":"Keine paired + trusted Geraete gefunden",35,y,c->gray);
    }

    int device_selection=selection>=2?selection-2:0;
    int first=0;
    if(device_selection>5)first=device_selection-5;
    if(first>device_count-6)first=device_count>6?device_count-6:0;
    if(first<0)first=0;
    for(int i=first;i<device_count&&i<first+6;i++){
        BluetoothDevice *d=&devices[i];SDL_Color col=(selection==i+2)?c->selected:c->white;
        const char *pair_state=(discovery_mode&&!d->paired)?"  [neu]":"";
        snprintf(buf,sizeof(buf),"%s%s%s%s",d->name,d->connected?"  [verbunden]":"",pair_state,!strcmp(d->mac,bluetooth_device_mac)?"  *":"");
        draw_text(c->renderer,c->font,buf,35,y,col);
        draw_text(c->renderer,c->font,d->mac,55,y+22,c->gray);y+=46;
    }
    if(message[0]&&(Sint32)((Uint32)message_until-SDL_GetTicks())>0)draw_text(c->renderer,c->font,message,35,SCREEN_H-65,c->selected);
    draw_text(c->renderer,c->font,"A: Auswaehlen/Start  Links/Rechts: Autoconnect  B: Zurueck",20,SCREEN_H-30,c->gray);
}
