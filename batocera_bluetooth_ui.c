#include "batocera_bluetooth.h"
#include "bluetooth.h"
#include "screens/bluetooth.h"
#include "ui.h"
#include "app_log.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

static BluetoothDevice saved[BT_MAX_DEVICES];
static BluetoothDevice live[BT_MAX_DEVICES];
static int saved_count;
static int live_count;
static int selection;
static int scanning;
static int bluetooth_ui_active;
static Uint32 last_scan_refresh;
static char message[160];
static Uint32 message_until;
static SDL_Thread *pair_thread;
static SDL_atomic_t pair_done;
static int pair_result;
static char pair_mac[18];

static void set_message(const char *text){snprintf(message,sizeof(message),"%s",text?text:"");message_until=SDL_GetTicks()+3500U;}
static void refresh_saved(void){saved_count=batocera_bluetooth_list(saved,BT_MAX_DEVICES);int max=scanning?(live_count>0?live_count-1:0):saved_count+2;if(selection>max)selection=max;}
static void refresh_live(void){live_count=batocera_bluetooth_live_devices(live,BT_MAX_DEVICES);if(live_count<=0)selection=0;else if(selection>=live_count)selection=live_count-1;}
static int pair_worker(void *unused){(void)unused;pair_result=batocera_bluetooth_pair(pair_mac);SDL_AtomicSet(&pair_done,1);return 0;}
static void finish_pair_if_ready(void){if(!pair_thread||!SDL_AtomicGet(&pair_done))return;SDL_WaitThread(pair_thread,NULL);pair_thread=NULL;batocera_bluetooth_stop_live_devices();scanning=0;selection=2;refresh_saved();set_message(pair_result==0?"Geraet gekoppelt":"Koppeln fehlgeschlagen");}
static void start_scan(void){if(!bluetooth_adapter_powered()){set_message("Bluetooth ist ausgeschaltet");return;}live_count=0;selection=0;if(batocera_bluetooth_start_live_devices()!=0){set_message("Suche konnte nicht gestartet werden");return;}scanning=1;last_scan_refresh=0;set_message("Suche laeuft ...");}
static void stop_scan(void){if(scanning)batocera_bluetooth_stop_live_devices();scanning=0;live_count=0;selection=2;refresh_saved();}

void __wrap_bluetoothscreen_reset(void){if(scanning)batocera_bluetooth_stop_live_devices();scanning=0;selection=0;live_count=0;message[0]='\0';message_until=0;bluetooth_ui_active=1;refresh_saved();}
int batocera_bluetooth_remap_event(SDL_Event *e){if(!bluetooth_ui_active||!e||e->type!=SDL_JOYBUTTONDOWN)return 0;if(e->jbutton.button==BUTTON_X){e->jbutton.button=BUTTON_R1;return 1;}return 0;}

static void toggle_power(void){int was_on=bluetooth_adapter_powered();if(was_on){if(scanning)stop_scan();set_message("Bluetooth wird ausgeschaltet ...");int rc=batocera_bluetooth_disable();set_message(rc==0?"Bluetooth ausgeschaltet":"Ausschalten fehlgeschlagen");}else{set_message("Bluetooth wird eingeschaltet ...");int rc=batocera_bluetooth_enable();set_message(rc==0?"Bluetooth eingeschaltet":"Einschalten fehlgeschlagen");}refresh_saved();}
static void toggle_auto(void){bluetooth_autoconnect=!bluetooth_autoconnect;if(bluetooth_save_config()!=0)set_message("config.ini konnte nicht gespeichert werden");else set_message(bluetooth_autoconnect?"Autoconnect: Ein":"Autoconnect: Aus");}
static void connect_toggle(BluetoothDevice *d){if(!d)return;int rc;if(d->connected){set_message("Trenne Verbindung ...");rc=batocera_bluetooth_disconnect(d->mac);set_message(rc==0?"Verbindung getrennt":"Trennen fehlgeschlagen");}else{snprintf(bluetooth_device_mac,sizeof(bluetooth_device_mac),"%s",d->mac);bluetooth_save_config();set_message("Verbinde ...");rc=batocera_bluetooth_connect(d->mac);set_message(rc==0?"Verbindung erfolgreich":"Verbindung fehlgeschlagen");}refresh_saved();}
static void remove_saved(BluetoothDevice *d){if(!d)return;if(batocera_bluetooth_remove(d->mac)==0){if(!strcmp(bluetooth_device_mac,d->mac)){bluetooth_device_mac[0]='\0';bluetooth_save_config();}set_message("Geraet entfernt");}else set_message("Entfernen fehlgeschlagen");refresh_saved();}
static void move_selection(int delta){int max;if(scanning)max=live_count>0?live_count-1:0;else max=saved_count+2;selection+=delta;if(selection<0)selection=max;if(selection>max)selection=0;}

void __wrap_bluetoothscreen_handle_event(ScreenContext *c,const SDL_Event *e){finish_pair_if_ready();if(!e)return;if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;if(scanning){if(b==BUTTON_B){stop_scan();return;}if(pair_thread)return;if(b==BUTTON_DPAD_UP){move_selection(-1);return;}if(b==BUTTON_DPAD_DOWN){move_selection(1);return;}if(b==BUTTON_A&&live_count>0){snprintf(pair_mac,sizeof(pair_mac),"%s",live[selection].mac);SDL_AtomicSet(&pair_done,0);pair_result=-1;pair_thread=SDL_CreateThread(pair_worker,"bt-pair",NULL);if(!pair_thread)set_message("Pairing-Thread konnte nicht starten");else set_message("Kopple Geraet ...");return;}return;}if(b==BUTTON_B){bluetooth_ui_active=0;*c->screen=SCREEN_SYSTEM_INFO;return;}if(b==BUTTON_DPAD_UP){move_selection(-1);return;}if(b==BUTTON_DPAD_DOWN){move_selection(1);return;}if(selection==0&&(b==BUTTON_A||b==BUTTON_DPAD_LEFT||b==BUTTON_DPAD_RIGHT)){toggle_power();return;}if(selection==1&&(b==BUTTON_A||b==BUTTON_DPAD_LEFT||b==BUTTON_DPAD_RIGHT)){toggle_auto();return;}if(selection==2&&b==BUTTON_A){start_scan();return;}if(selection>=3&&selection<saved_count+3){BluetoothDevice *d=&saved[selection-3];if(b==BUTTON_A){connect_toggle(d);return;}if(b==BUTTON_R1){remove_saved(d);return;}}}if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_Y){if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){move_selection(-1);*c->axis_y_lock=1;}else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){move_selection(1);*c->axis_y_lock=1;}if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;}}

static void draw_device_icon(SDL_Renderer *r,const char *type,int x,int y,SDL_Color col)
{
    SDL_SetRenderDrawColor(r,col.r,col.g,col.b,col.a);
    if(type&&!strcasecmp(type,"audio")){
        /* Kopfhoerer: Bogen plus zwei Ohrmuscheln. */
        SDL_Rect top={x+4,y+2,12,2}, left={x+2,y+7,3,10}, right={x+15,y+7,3,10};
        SDL_RenderFillRect(r,&top);SDL_RenderFillRect(r,&left);SDL_RenderFillRect(r,&right);
        SDL_RenderDrawLine(r,x+3,y+8,x+5,y+3);SDL_RenderDrawLine(r,x+16,y+3,x+18,y+8);
    }else if(type&&!strcasecmp(type,"phone")){
        /* Telefon/Smartphone. */
        SDL_Rect body={x+5,y+1,10,18};SDL_RenderDrawRect(r,&body);
        SDL_RenderDrawLine(r,x+8,y+4,x+12,y+4);SDL_RenderDrawPoint(r,x+10,y+16);
    }else{
        /* Unbekannter Geraetetyp. */
        SDL_RenderDrawRect(r,&(SDL_Rect){x+3,y+2,15,15});
        SDL_RenderDrawLine(r,x+7,y+6,x+9,y+4);SDL_RenderDrawLine(r,x+9,y+4,x+13,y+4);
        SDL_RenderDrawLine(r,x+13,y+4,x+15,y+6);SDL_RenderDrawLine(r,x+15,y+6,x+15,y+8);
        SDL_RenderDrawLine(r,x+15,y+8,x+11,y+11);SDL_RenderDrawLine(r,x+11,y+11,x+11,y+13);
        SDL_RenderDrawPoint(r,x+11,y+16);
    }
}

static void render_scanning(ScreenContext *c)
{
    Uint32 now=SDL_GetTicks();if(now-last_scan_refresh>=500U){refresh_live();last_scan_refresh=now;}finish_pair_if_ready();
    draw_text(c->renderer,c->font,"Bluetooth - Geraete suchen",20,20,c->selected);
    if(pair_thread)draw_text(c->renderer,c->font,"Kopplung laeuft ...",35,58,c->selected);else draw_text(c->renderer,c->font,"Suche laeuft ...",35,58,c->gray);
    int y=95;if(live_count==0)draw_text(c->renderer,c->font,"Noch keine Geraete gefunden",35,y,c->gray);
    for(int i=0;i<live_count&&i<8;i++){
        SDL_Color col=i==selection?c->selected:c->white;
        draw_device_icon(c->renderer,live[i].type,35,y+1,col);
        draw_text(c->renderer,c->font,live[i].name,62,y,col);
        draw_text(c->renderer,c->font,live[i].mac,62,y+22,c->gray);
        y+=46;
    }
    if(message[0]&&(Sint32)(message_until-now)>0)draw_text(c->renderer,c->font,message,35,SCREEN_H-65,c->selected);
    draw_text(c->renderer,c->font,pair_thread?"Bitte warten   B nach Abschluss: Zurueck":"A: Koppeln   B: Suche beenden",20,SCREEN_H-30,c->gray);
}

void __wrap_bluetoothscreen_render(ScreenContext *c)
{
    if(scanning){render_scanning(c);return;}finish_pair_if_ready();
    draw_text(c->renderer,c->font,"Bluetooth",20,20,c->selected);char buf[320];int powered=bluetooth_adapter_powered();
    snprintf(buf,sizeof(buf),"Bluetooth: < %s >",powered?"Ein":"Aus");draw_text(c->renderer,c->font,buf,35,60,selection==0?c->selected:c->white);
    snprintf(buf,sizeof(buf),"Autoconnect beim Start: < %s >",bluetooth_autoconnect?"Ein":"Aus");draw_text(c->renderer,c->font,buf,35,92,selection==1?c->selected:c->white);
    draw_text(c->renderer,c->font,"Geraete suchen",35,124,selection==2?c->selected:c->white);
    int y=164;if(!batocera_bluetooth_available())draw_text(c->renderer,c->font,"batocera-bluetooth nicht gefunden",35,y,c->selected);else if(!powered)draw_text(c->renderer,c->font,"Bluetooth ist ausgeschaltet",35,y,c->gray);else if(saved_count==0)draw_text(c->renderer,c->font,"Keine gespeicherten Geraete",35,y,c->gray);
    int first=0;if(selection>9)first=selection-9;if(first>saved_count-6)first=saved_count>6?saved_count-6:0;if(first<0)first=0;
    for(int i=first;i<saved_count&&i<first+6;i++){BluetoothDevice *d=&saved[i];SDL_Color col=selection==i+3?c->selected:c->white;snprintf(buf,sizeof(buf),"%s%s%s",d->name,d->connected?" [verbunden]":"",!strcmp(d->mac,bluetooth_device_mac)?" *":"");draw_text(c->renderer,c->font,buf,35,y,col);draw_text(c->renderer,c->font,d->mac,55,y+22,c->gray);y+=46;}
    Uint32 now=SDL_GetTicks();if(message[0]&&(Sint32)(message_until-now)>0)draw_text(c->renderer,c->font,message,35,SCREEN_H-65,c->selected);
    draw_text(c->renderer,c->font,"A: Aktion  X: Entfernen  Links/Rechts: Schalter  B: Zurueck",20,SCREEN_H-30,c->gray);
}
