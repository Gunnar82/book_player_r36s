#include "bluetooth.h"
#include "../bluetooth.h"
#include "../bluez_discovery.h"
#include "../batocera_pair_agent.h"
#include "../ui.h"
#include "../app_log.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#ifdef BUILD_R36S
void r36s_bluetooth_input_set_active(int active);
#endif

static BluetoothDevice devices[BT_MAX_DEVICES];
static BluetoothDevice live[BT_MAX_DEVICES];
static int device_count=0,live_count=0,selection=0,scanning=0;
static int message_until=0;static char message[128]="";
static Uint32 last_refresh;
static SDL_Thread *pair_thread;static SDL_atomic_t pair_done;static int pair_result;static char pair_mac[18];

static void set_message(const char *text){snprintf(message,sizeof(message),"%s",text?text:"");message_until=(int)SDL_GetTicks()+3500;}
static void refresh(void){if(!bluetooth_adapter_powered()){device_count=0;selection=0;return;}device_count=bluetooth_scan_paired_trusted(devices,BT_MAX_DEVICES);if(!scanning&&selection>device_count+1)selection=device_count+1;}
static void refresh_live(void){live_count=bluez_discovery_devices(live,BT_MAX_DEVICES);if(live_count<=0)selection=0;else if(selection>=live_count)selection=live_count-1;}
static int pair_worker(void *unused){(void)unused;pair_result=batocera_pair_agent_pair(pair_mac);SDL_AtomicSet(&pair_done,1);return 0;}
static void finish_pair(void){if(!pair_thread||!SDL_AtomicGet(&pair_done))return;SDL_WaitThread(pair_thread,NULL);pair_thread=NULL;bluez_discovery_stop();scanning=0;selection=1;refresh();if(pair_result==0){snprintf(bluetooth_device_mac,sizeof(bluetooth_device_mac),"%s",pair_mac);bluetooth_save_config();set_message(bluetooth_connect_device(pair_mac)==0?"Geraet gekoppelt und verbunden":"Gekoppelt, Verbindung fehlgeschlagen");}else set_message("Koppeln fehlgeschlagen");}
static void start_scan(void){if(!bluetooth_adapter_powered()){set_message("Bluetooth ist ausgeschaltet");return;}live_count=0;selection=0;if(bluez_discovery_start()!=0){set_message("Suche konnte nicht gestartet werden");return;}scanning=1;last_refresh=0;set_message("Suche laeuft ...");}
static void stop_scan(void){bluez_discovery_stop();scanning=0;live_count=0;selection=1;refresh();}
void bluetoothscreen_reset(void){if(scanning)bluez_discovery_stop();scanning=0;selection=0;live_count=0;message[0]='\0';message_until=0;
#ifdef BUILD_R36S
r36s_bluetooth_input_set_active(1);
#endif
bluetooth_log_if_changed();refresh();}
static void move(int delta){int max=scanning?(live_count>0?live_count-1:0):device_count+1;selection+=delta;if(selection<0)selection=max;if(selection>max)selection=0;}
static void toggle_auto(void){bluetooth_autoconnect=!bluetooth_autoconnect;if(bluetooth_save_config()!=0)set_message("config.ini konnte nicht gespeichert werden");else set_message(bluetooth_autoconnect?"Autoconnect: Ein":"Autoconnect: Aus");}
static void remove_device(BluetoothDevice *d){if(!d)return;if(bluetooth_remove_device(d->mac)==0){if(!strcmp(bluetooth_device_mac,d->mac)){bluetooth_device_mac[0]='\0';bluetooth_save_config();}set_message("Geraet entfernt");refresh();}else set_message("Entfernen fehlgeschlagen");}

void bluetoothscreen_handle_event(ScreenContext *c,const SDL_Event *e)
{
    finish_pair();if(!e)return;
    if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;
        if(scanning){
            if(pair_thread){BatoceraPairState ps=batocera_pair_agent_state();if(ps==BATOCERA_PAIR_CONFIRM){if(b==BUTTON_A){batocera_pair_agent_respond(1);set_message("Code bestaetigt");return;}if(b==BUTTON_B){batocera_pair_agent_respond(0);set_message("Kopplung abgelehnt");return;}}else if(b==BUTTON_B){batocera_pair_agent_cancel();set_message("Kopplung wird abgebrochen ...");return;}return;}
            if(b==BUTTON_B){stop_scan();return;}if(b==BUTTON_DPAD_UP){move(-1);return;}if(b==BUTTON_DPAD_DOWN){move(1);return;}
            if(b==BUTTON_A&&live_count>0){snprintf(pair_mac,sizeof(pair_mac),"%s",live[selection].mac);SDL_AtomicSet(&pair_done,0);pair_result=-1;pair_thread=SDL_CreateThread(pair_worker,"bt-pair",NULL);if(!pair_thread)set_message("Pairing-Thread konnte nicht starten");else set_message("Kopple Geraet ...");return;}return;
        }
        if(b==BUTTON_B){
#ifdef BUILD_R36S
            r36s_bluetooth_input_set_active(0);
#endif
            *c->screen=SCREEN_SYSTEM_INFO;return;
        }if(b==BUTTON_DPAD_UP){move(-1);return;}if(b==BUTTON_DPAD_DOWN){move(1);return;}
        if(selection==0&&(b==BUTTON_A||b==BUTTON_DPAD_LEFT||b==BUTTON_DPAD_RIGHT)){toggle_auto();return;}
        if(selection==1&&b==BUTTON_A){start_scan();return;}
        if(selection>=2&&selection<device_count+2){BluetoothDevice *d=&devices[selection-2];if(b==BUTTON_R1){remove_device(d);return;}if(b==BUTTON_A){snprintf(bluetooth_device_mac,sizeof(bluetooth_device_mac),"%s",d->mac);if(bluetooth_save_config()!=0){set_message("Auswahl konnte nicht gespeichert werden");return;}set_message("Verbinde ...");int rc=bluetooth_connect_device(d->mac);refresh();set_message(rc==0?"Verbindung erfolgreich":"Verbindung fehlgeschlagen");return;}}
    }
    if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_Y){if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){move(-1);*c->axis_y_lock=1;}else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){move(1);*c->axis_y_lock=1;}if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;}
}

static void draw_icon(SDL_Renderer *r,const char *type,int x,int y,SDL_Color col){SDL_SetRenderDrawColor(r,col.r,col.g,col.b,col.a);if(type&&!strcasecmp(type,"audio")){SDL_Rect a={x+4,y+2,12,2},l={x+2,y+7,3,10},rr={x+15,y+7,3,10};SDL_RenderFillRect(r,&a);SDL_RenderFillRect(r,&l);SDL_RenderFillRect(r,&rr);SDL_RenderDrawLine(r,x+3,y+8,x+5,y+3);SDL_RenderDrawLine(r,x+16,y+3,x+18,y+8);}else if(type&&!strcasecmp(type,"phone")){SDL_Rect q={x+5,y+1,10,18};SDL_RenderDrawRect(r,&q);SDL_RenderDrawPoint(r,x+10,y+16);}else{SDL_RenderDrawRect(r,&(SDL_Rect){x+3,y+2,15,15});SDL_RenderDrawLine(r,x+7,y+6,x+9,y+4);SDL_RenderDrawLine(r,x+9,y+4,x+13,y+4);SDL_RenderDrawLine(r,x+13,y+4,x+15,y+7);SDL_RenderDrawLine(r,x+15,y+7,x+11,y+11);SDL_RenderDrawPoint(r,x+11,y+15);}}

void bluetoothscreen_render(ScreenContext *c)
{
    finish_pair();Uint32 now=SDL_GetTicks();
    if(scanning){if(now-last_refresh>=500){refresh_live();last_refresh=now;}draw_text(c->renderer,c->font,"Bluetooth - Geraete suchen",20,20,c->selected);BatoceraPairState ps=pair_thread?batocera_pair_agent_state():BATOCERA_PAIR_IDLE;if(ps==BATOCERA_PAIR_CONFIRM){char code[80];snprintf(code,sizeof(code),"Code: %06u",batocera_pair_agent_passkey());draw_text(c->renderer,c->font,"Bluetooth-Code vergleichen",35,58,c->selected);draw_text(c->renderer,c->font,code,35,92,c->white);draw_text(c->renderer,c->font,"Stimmt der Code auf beiden Geraeten ueberein?",35,126,c->white);draw_text(c->renderer,c->font,"A: Ja / bestaetigen    B: Nein / ablehnen",20,SCREEN_H-30,c->gray);return;}draw_text(c->renderer,c->font,pair_thread?"Kopplung laeuft ...":"Suche laeuft ...",35,58,pair_thread?c->selected:c->gray);int y=95;if(live_count==0)draw_text(c->renderer,c->font,"Noch keine Geraete gefunden",35,y,c->gray);for(int i=0;i<live_count&&i<8;i++){SDL_Color col=i==selection?c->selected:c->white;draw_icon(c->renderer,live[i].type,35,y+1,col);draw_text(c->renderer,c->font,live[i].name,62,y,col);draw_text(c->renderer,c->font,live[i].mac,62,y+22,c->gray);y+=46;}draw_text(c->renderer,c->font,pair_thread?"B: Kopplung abbrechen":"A: Koppeln   B: Suche beenden",20,SCREEN_H-30,c->gray);return;}
    draw_text(c->renderer,c->font,"Bluetooth",20,20,c->selected);char buf[256];snprintf(buf,sizeof(buf),"Autoconnect beim Start: < %s >",bluetooth_autoconnect?"Ein":"Aus");draw_text(c->renderer,c->font,buf,35,60,selection==0?c->selected:c->white);draw_text(c->renderer,c->font,"Geraete suchen",35,94,selection==1?c->selected:c->white);int y=136;if(!bluetooth_adapter_present())draw_text(c->renderer,c->font,"Kein Bluetooth-Adapter vorhanden",35,y,c->gray);else if(!bluetooth_adapter_powered())draw_text(c->renderer,c->font,"Bluetooth ist ausgeschaltet",35,y,c->gray);else if(device_count==0)draw_text(c->renderer,c->font,"Keine gekoppelten Geraete",35,y,c->gray);
    int first=0;if(selection>8)first=selection-8;if(first>device_count-7)first=device_count>7?device_count-7:0;if(first<0)first=0;for(int i=first;i<device_count&&i<first+7;i++){BluetoothDevice *d=&devices[i];SDL_Color col=(selection==i+2)?c->selected:c->white;snprintf(buf,sizeof(buf),"%s%s%s",d->name,d->connected?" [verbunden]":"",!strcmp(d->mac,bluetooth_device_mac)?" *":"");draw_text(c->renderer,c->font,buf,35,y,col);draw_text(c->renderer,c->font,d->mac,55,y+22,c->gray);y+=46;}
    if(message[0]&&(Sint32)((Uint32)message_until-now)>0)draw_text(c->renderer,c->font,message,35,SCREEN_H-65,c->selected);draw_text(c->renderer,c->font,"A: Aktion  X/R1: Entfernen  Links/Rechts: Autoconnect  B: Zurueck",20,SCREEN_H-30,c->gray);
}
