#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/reboot.h>
#include "config.h"
#include "scanner.h"
#include "audio.h"
#include "state.h"
#include "battery.h"
#include "backlight.h"
#include "led.h"
#include "download.h"
#include "media_keys.h"
#include "media_feedback.h"
#include "mpris_bridge.h"
#include "app_log.h"
#include "ui.h"
#include "screens.h"
#include "screens/screen_context.h"
#include "screens/menu.h"
#include "screens/player.h"
#include "screens/sleeptimer.h"
#include "screens/systemmenu.h"
#include "screens/systeminfo.h"
#include "screens/buttondebug.h"
#include "screens/downloadbrowser.h"
#include "screens/logview.h"

static volatile sig_atomic_t running=1;
static void handle_signal(int sig){(void)sig;running=0;}
static void shutdown_system(Mix_Music **music){if(*music){Mix_HaltMusic();Mix_FreeMusic(*music);*music=NULL;}save_state();sync();reboot(RB_POWER_OFF);}
static Uint32 idle_remaining_ms=0;
static Uint32 idle_last_ticks=0;
static int idle_was_playing=0;

int main(void){
 signal(SIGINT,handle_signal);signal(SIGTERM,handle_signal);
 app_log_init();app_log_printf("INFO","Programmstart %s",APP_VERSION);
 load_config();load_state();led_init();backlight_init();
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK)<0){app_log_printf("ERROR","SDL_Init: %s",SDL_GetError());return 1;}
 if(TTF_Init()<0){app_log_printf("ERROR","TTF_Init: %s",TTF_GetError());return 1;}
 if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048)<0){app_log_printf("ERROR","Mix_OpenAudio: %s",Mix_GetError());return 1;}
 SDL_Window *window=SDL_CreateWindow(APP_NAME,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,SCREEN_W,SCREEN_H,SDL_WINDOW_SHOWN);
 SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
 TTF_Font *font=TTF_OpenFont(FONT_PATH,24);if(!font){app_log_printf("ERROR","Font: %s",TTF_GetError());return 1;}
 SDL_Joystick *joy=NULL;if(SDL_NumJoysticks()>0){joy=SDL_JoystickOpen(0);if(joy)app_log_printf("INFO","Controller: %s",SDL_JoystickName(joy));}
 media_keys_init();mpris_bridge_init();

 char book_names[MAX_BOOKS][256],book_paths[MAX_BOOKS][512];int book_count=scan_books(book_names,book_paths,MAX_BOOKS);int book_index=0;
 Track tracks[MAX_TRACKS];double track_durations[MAX_TRACKS];int track_count=0,track_index=0;double book_duration=0;
 Mix_Music *music=NULL;double duration=0,base_position=0;Uint32 started_ticks=0,last_save=0;int paused=0;
 int battery_percent=-1,battery_charging=-1;Uint32 last_battery_update=0;int axis_x_lock=0,axis_y_lock=0;
 Screen screen=SCREEN_BOOKS;int sleep_timer_active=0,sleep_timer_minutes=SLEEP_DEFAULT_MINUTES;Uint32 sleep_timer_end_ticks=0;int sleep_timer_blink_on=0;
 idle_remaining_ms=(Uint32)idle_timer_minutes*60U*1000U;idle_last_ticks=SDL_GetTicks();

 ScreenContext c={0};c.renderer=renderer;c.font=font;c.screen=&screen;c.running=(int*)&running;c.book_names=book_names;c.book_paths=book_paths;c.book_count=&book_count;c.book_index=&book_index;c.tracks=tracks;c.track_durations=track_durations;c.track_count=&track_count;c.track_index=&track_index;c.book_duration=&book_duration;c.music=&music;c.duration=&duration;c.base_position=&base_position;c.started_ticks=&started_ticks;c.last_save=&last_save;c.paused=&paused;c.battery_percent=&battery_percent;c.battery_charging=&battery_charging;c.axis_x_lock=&axis_x_lock;c.axis_y_lock=&axis_y_lock;c.sleep_timer_active=&sleep_timer_active;c.sleep_timer_minutes=&sleep_timer_minutes;c.sleep_timer_end_ticks=&sleep_timer_end_ticks;c.sleep_timer_blink_on=&sleep_timer_blink_on;c.idle_timer_remaining_ms=&idle_remaining_ms;c.shutdown_fn=shutdown_system;c.white=(SDL_Color){240,240,240,255};c.gray=(SDL_Color){150,150,150,255};c.selected=(SDL_Color){255,220,60,255};

 while(running){Uint32 now=SDL_GetTicks();
  if(now-last_battery_update>=5000){battery_update(&battery_percent,&battery_charging);last_battery_update=now;}
  SDL_Event e;while(SDL_PollEvent(&e)){
   if(e.type==SDL_QUIT){running=0;continue;}
   if(media_keys_handle_event(&e))continue;
   if(e.type==SDL_KEYDOWN){int key=e.key.keysym.sym;if(key==KEY_VOLUME_UP||key==KEY_VOLUME_DOWN){int delta=(key==KEY_VOLUME_UP)?VOLUME_STEP:-VOLUME_STEP;volume+=delta;if(volume<0)volume=0;if(volume>MIX_MAX_VOLUME)volume=MIX_MAX_VOLUME;Mix_VolumeMusic(volume);media_feedback_show_volume((volume*100)/MIX_MAX_VOLUME);continue;}}
   if(e.type==SDL_JOYBUTTONDOWN){int b=e.jbutton.button;if(b==BUTTON_X){screen=(screen==SCREEN_SYSTEM_MENU)?SCREEN_PLAYER:SCREEN_SYSTEM_MENU;continue;}if(b==BUTTON_Y){screen=SCREEN_BOOKS;continue;}}
   switch(screen){case SCREEN_BOOKS:menu_handle_event(&c,&e);break;case SCREEN_TRACKS:menu_handle_event(&c,&e);break;case SCREEN_PLAYER:player_handle_event(&c,&e);break;case SCREEN_SLEEP_TIMER:sleeptimer_handle_event(&c,&e);break;case SCREEN_SYSTEM_MENU:systemmenu_handle_event(&c,&e);break;case SCREEN_SYSTEM_INFO:systeminfo_handle_event(&c,&e);break;case SCREEN_BUTTON_DEBUG:buttondebug_handle_event(&c,&e);break;case SCREEN_DOWNLOADS:downloadbrowser_handle_event(&c,&e);break;case SCREEN_LOG:logview_handle_event(&c,&e);break;}
  }
  int playing=(music&&!paused&&Mix_PlayingMusic());if(playing){idle_last_ticks=now;idle_was_playing=1;}else{if(idle_was_playing){idle_last_ticks=now;idle_was_playing=0;}if(idle_timer_minutes>0&&idle_remaining_ms>0){Uint32 d=now-idle_last_ticks;if(d>=idle_remaining_ms)idle_remaining_ms=0;else idle_remaining_ms-=d;idle_last_ticks=now;if(idle_remaining_ms==0){app_log_printf("INFO","Idle-Timer abgelaufen, fahre herunter");shutdown_system(&music);running=0;}}}
  if(sleep_timer_active&&now>=sleep_timer_end_ticks){app_log_printf("INFO","Sleeptimer abgelaufen");sleep_timer_active=0;if(music)Mix_HaltMusic();paused=1;}
  if(music&&!paused&&now-last_save>=SAVE_INTERVAL_MS){int pi=ensure_book_progress(book_paths[book_index]);if(pi>=0){progress[pi].track=track_index;progress[pi].position=get_position(base_position,started_ticks,paused);touch_book_progress(pi);}save_state();last_save=now;}
  SDL_SetRenderDrawColor(renderer,18,18,18,255);SDL_RenderClear(renderer);
  switch(screen){case SCREEN_BOOKS:case SCREEN_TRACKS:menu_render(&c);break;case SCREEN_PLAYER:player_render(&c);break;case SCREEN_SLEEP_TIMER:sleeptimer_render(&c);break;case SCREEN_SYSTEM_MENU:systemmenu_render(&c);break;case SCREEN_SYSTEM_INFO:systeminfo_render(&c);break;case SCREEN_BUTTON_DEBUG:buttondebug_render(&c);break;case SCREEN_DOWNLOADS:downloadbrowser_render(&c);break;case SCREEN_LOG:logview_render(&c);break;}
  SDL_RenderPresent(renderer);SDL_Delay(10);
 }
 app_log_printf("INFO","Programmende");mpris_bridge_shutdown();media_keys_shutdown();if(music)Mix_FreeMusic(music);if(joy)SDL_JoystickClose(joy);TTF_CloseFont(font);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);Mix_CloseAudio();TTF_Quit();SDL_Quit();return 0;
}
