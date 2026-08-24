#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "types.h"
#include "state.h"
#include "backlight.h"
#include "battery.h"
#include "battery_bluez.h"
#include "led.h"
#include "scanner.h"
#include "audio.h"
#include "ui.h"
#include "screens.h"
#include "storage.h"
#include "systemstats.h"
#include "media_keys.h"
#include "mpris_bridge.h"
#include "bluetooth.h"
#include "hfp_gateway.h"
#include "pbap_phonebook.h"
#include "streaming.h"
#include "app_log.h"
#include "input_config.h"
#include "screens/menu.h"
#include "screens/tracks.h"
#include "screens/player.h"
#include "screens/sleeptimer.h"
#include "screens/systeminfo.h"
#include "screens/buttondebug.h"
#include "screens/systemmenu.h"
#include "screens/downloadbrowser.h"
#include "screens/streams.h"
#include "screens/downloadsettings.h"
#include "screens/streamsettings.h"
#include "screens/bluetooth.h"
#include "screens/logview.h"

static const int LOCK_CANDIDATE_BUTTONS[]={BUTTON_DPAD_UP,BUTTON_DPAD_DOWN,BUTTON_DPAD_LEFT,BUTTON_DPAD_RIGHT,BUTTON_A,BUTTON_B,BUTTON_X,BUTTON_Y};
static const char *LOCK_CANDIDATE_NAMES[]={"Hoch","Runter","Links","Rechts","A","B","X","Y"};
#define LOCK_CANDIDATE_COUNT (int)(sizeof(LOCK_CANDIDATE_BUTTONS)/sizeof(LOCK_CANDIDATE_BUTTONS[0]))
static const char *button_name(int button){for(int i=0;i<LOCK_CANDIDATE_COUNT;i++)if(LOCK_CANDIDATE_BUTTONS[i]==button)return LOCK_CANDIDATE_NAMES[i];return "?";}
static void generate_unlock_sequence(int *sequence,int length){for(int i=0;i<length;i++){int idx=rand()%LOCK_CANDIDATE_COUNT;sequence[i]=LOCK_CANDIDATE_BUTTONS[idx];}}

static int copy_checked(char *dst,size_t dst_size,const char *src){
 if(!dst||dst_size==0)return -1;
 if(!src){dst[0]='\0';return 0;}
 size_t n=strlen(src);
 if(n>=dst_size){dst[0]='\0';return -1;}
 memcpy(dst,src,n+1);
 return 0;
}

static void do_shutdown(Mix_Music **music){save_state();if(*music){Mix_HaltMusic();Mix_FreeMusic(*music);*music=NULL;}set_display_off(0);led_set(0);sync();system("/usr/bin/busctl call org.freedesktop.login1 /org/freedesktop/login1 org.freedesktop.login1.Manager PowerOff b false");}

static void filename_without_extension(const char *path,char *out,size_t out_size)
{
 const char *name=path?strrchr(path,'/'):NULL;name=name?name+1:(path?path:"");snprintf(out,out_size,"%s",name);char *dot=strrchr(out,'.');if(dot&&dot!=out)*dot='\0';
}
static void parent_directory_name(const char *path,char *out,size_t out_size)
{
 char tmp[512];snprintf(tmp,sizeof(tmp),"%s",path?path:"");size_t n=strlen(tmp);while(n>1&&tmp[n-1]=='/')tmp[--n]='\0';char *slash=strrchr(tmp,'/');if(!slash||slash==tmp){out[0]='\0';return;}*slash='\0';char *name=strrchr(tmp,'/');name=name?name+1:tmp;snprintf(out,out_size,"%s",name);
}

int main(int argc,char **argv)
{
 (void)argc;
 (void)argv;
 srand((unsigned int)time(NULL));app_log_clear();app_logf("Start %s %s",APP_NAME,APP_VERSION);setup_state_path();load_state();usage_app_starts++;load_playback_config();load_download_config();streaming_load_config();input_config_load();bluetooth_load_config();
 network_log_if_changed();
 bluetooth_log_if_changed();int bluetooth_present=bluetooth_adapter_present();if(bluetooth_present)bluetooth_autoconnect_start();else app_logf("Bluetooth: kein Adapter, Funktionen deaktiviert");
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK)!=0){fprintf(stderr,"SDL_Init: %s\n",SDL_GetError());return 1;}
 if(TTF_Init()!=0){fprintf(stderr,"TTF_Init: %s\n",TTF_GetError());SDL_Quit();return 1;}
 if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048)!=0){fprintf(stderr,"Mix_OpenAudio: %s\n",Mix_GetError());TTF_Quit();SDL_Quit();return 1;}
 Mix_VolumeMusic(volume);SDL_Joystick *joy=NULL;if(SDL_NumJoysticks()>0)joy=SDL_JoystickOpen(0);
#ifdef BUILD_BATOCERA
 SDL_ShowCursor(SDL_DISABLE);
#endif
 SDL_Window *window=SDL_CreateWindow("Hoerspiel Player",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,SCREEN_W,SCREEN_H,SDL_WINDOW_FULLSCREEN);if(!window){fprintf(stderr,"SDL_CreateWindow: %s\n",SDL_GetError());if(joy)SDL_JoystickClose(joy);Mix_CloseAudio();TTF_Quit();SDL_Quit();return 1;}
 SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);if(!renderer){fprintf(stderr,"SDL_CreateRenderer: %s\n",SDL_GetError());SDL_DestroyWindow(window);if(joy)SDL_JoystickClose(joy);Mix_CloseAudio();TTF_Quit();SDL_Quit();return 1;}
 TTF_Font *font=TTF_OpenFont(FONT_PATH,20);if(!font)fprintf(stderr,"Font: %s\n",TTF_GetError());
 init_backlight();init_battery();int battery_percent=get_battery_percent();int battery_charging=is_battery_charging();BatteryBluez *battery_bluez=NULL;if(bluetooth_present)battery_bluez_init(&battery_bluez,battery_percent);Uint32 last_battery_check=SDL_GetTicks();double cpu_usage=get_cpu_usage(),ram_usage=get_ram_usage(),cpu_temperature=get_cpu_temperature();Uint32 last_systemstats_check=SDL_GetTicks();SDL_Color white={255,255,255,255},selected={255,220,80,255},gray={160,160,160,255};
 char book_names[MAX_BOOKS][256],book_paths[MAX_BOOKS][512],book_roots[MAX_BOOKS][512];int book_track_counts[MAX_BOOKS];memset(book_track_counts,0,sizeof(book_track_counts));int book_count=0;StoragePath storage_paths[MAX_STORAGE_PATHS];int storage_path_count=get_storage_paths(storage_paths,MAX_STORAGE_PATHS);
 for(int si=0;si<storage_path_count&&book_count<MAX_BOOKS;si++){
  if(!storage_paths[si].available)continue;
  char names[MAX_BOOKS][256],paths[MAX_BOOKS][512];
  int count=scan_books_recursive(storage_paths[si].path,names,paths);
  for(int i=0;i<count&&book_count<MAX_BOOKS;i++){
   if(copy_checked(book_names[book_count],sizeof(book_names[book_count]),names[i])!=0)continue;
   if(copy_checked(book_paths[book_count],sizeof(book_paths[book_count]),paths[i])!=0)continue;
   if(copy_checked(book_roots[book_count],sizeof(book_roots[book_count]),storage_paths[si].path)!=0)continue;
   book_count++;
  }
 }
 for(int i=0;i<book_count;i++){Track tmp_tracks[MAX_TRACKS];book_track_counts[i]=scan_tracks(book_paths[i],tmp_tracks);unsigned int dial_id=ensure_book_dial_id(book_paths[i]);app_logf("HFP Buch-ID: %u = %s",dial_id,book_names[i]);}
 pbap_phonebook_sync(book_names,book_paths,book_count);
 save_state();
 Track tracks[MAX_TRACKS];int track_count=0;ScreenId screen=SCREEN_PLAYER;int book_index=0,track_index=0;Mix_Music *music=NULL;double base_position=0.0;Uint32 started_ticks=0,last_save=SDL_GetTicks();int paused=0;double duration=0.0,track_durations[MAX_TRACKS]={0},book_duration=0.0;int running=1,axis_y_lock=0,axis_x_lock=0;Uint32 last_activity=SDL_GetTicks();
 int idle_setting_seen=idle_timer_minutes;Uint32 idle_timer_remaining_ms=idle_timer_minutes>0?(Uint32)idle_timer_minutes*60000U:0U,idle_timer_last_tick=SDL_GetTicks();
 int resume_book=-1,resume_progress=-1;long long newest_played=-1;for(int i=0;i<book_count;i++){int pi=find_book_progress(book_paths[i]);if(pi<0)continue;if(progress[pi].last_played>newest_played){newest_played=progress[pi].last_played;resume_book=i;resume_progress=pi;}}
 if(resume_book>=0&&resume_progress>=0){track_count=scan_tracks(book_paths[resume_book],tracks);book_duration=get_track_durations(tracks,track_count,track_durations);if(track_count>0){book_index=resume_book;track_index=progress[resume_progress].track;if(track_index<0||track_index>=track_count)track_index=0;base_position=progress[resume_progress].position;started_ticks=SDL_GetTicks();paused=1;last_save=SDL_GetTicks();}}
 int sleep_timer_active=0,sleep_timer_minutes=SLEEP_DEFAULT_MINUTES;Uint32 sleep_timer_end_ticks=0;int shutdown_tracks_remaining=shutdown_after_tracks,shutdown_tracks_setting_seen=shutdown_after_tracks;int locked=0,unlock_sequence[UNLOCK_SEQUENCE_LEN],unlock_progress=0;Uint32 unlock_wrong_flash_until=0,lock_screen_visible_until=0;MediaKeys media_keys;media_keys_init(&media_keys);MprisBridge mpris;memset(&mpris,0,sizeof(mpris));if(bluetooth_present)mpris_bridge_init(&mpris);HfpGateway *hfp_gateway=NULL;if(bluetooth_present&&hfp_gateway_init(&hfp_gateway)<0)app_logf("HFP IPC konnte nicht gestartet werden");Uint32 last_bluetooth_check=SDL_GetTicks(),usage_last_tick=SDL_GetTicks();

 Uint32 frame_last_present=0;
Uint32 last_connectivity_state_check=0;
 while(running){
  if(SDL_GetTicks()-last_bluetooth_check>=2000){
   int now_present=bluetooth_adapter_present();
   if(now_present!=bluetooth_present){
    bluetooth_present=now_present;
    menu_invalidate();
    if(bluetooth_present){
     app_logf("Bluetooth: Adapter erkannt, Funktionen aktiviert");
     battery_bluez_init(&battery_bluez,battery_percent);
     mpris_bridge_init(&mpris);
     if(hfp_gateway_init(&hfp_gateway)<0)app_logf("HFP IPC konnte nicht gestartet werden");
     bluetooth_autoconnect_start();
    }else{
     app_logf("Bluetooth: Adapter entfernt, Funktionen deaktiviert");
     hfp_gateway_close(hfp_gateway);hfp_gateway=NULL;
     battery_bluez_close(battery_bluez);battery_bluez=NULL;
     mpris_bridge_close(&mpris);memset(&mpris,0,sizeof(mpris));
     if(screen==SCREEN_BLUETOOTH)screen=SCREEN_SYSTEM_INFO;
    }
   }
   last_bluetooth_check=SDL_GetTicks();
  }
  SDL_Event repeat_e;
  int repeat_enabled=(screen==SCREEN_MENU||screen==SCREEN_TRACKS||screen==SCREEN_SYSTEM_INFO||
                      screen==SCREEN_SYSTEM_MENU||screen==SCREEN_DOWNLOADS||screen==SCREEN_STREAMS||
                      screen==SCREEN_LOG||screen==SCREEN_BLUETOOTH||screen==SCREEN_DOWNLOAD_SETTINGS||screen==SCREEN_STREAM_SETTINGS);
  if(input_repeat_event(&repeat_e,SDL_GetTicks(),repeat_enabled)){
   if((repeat_e.jbutton.button==BUTTON_L1||repeat_e.jbutton.button==BUTTON_R1) && screen!=SCREEN_STREAMS){
    /* L1/R1-Haltewiederholung nur in der Streams-Liste. */
   }else SDL_PushEvent(&repeat_e);
  }

  SDL_Event e;
  while(SDL_PollEvent(&e)){
   if(e.type==SDL_QUIT){running=0;continue;}
   /* Eingaben auf interne R36S-Belegung normalisieren. Im R36S-Profil bleiben Achsen ignoriert. */
   if(!input_normalize_event(&e))continue;
   if(e.type==SDL_JOYBUTTONDOWN||e.type==SDL_KEYDOWN){last_activity=SDL_GetTicks();idle_timer_remaining_ms=idle_timer_minutes>0?(Uint32)idle_timer_minutes*60000U:0U;idle_timer_last_tick=SDL_GetTicks();if(is_display_off()){set_display_off(0);continue;}}
   if(screen==SCREEN_BUTTON_DEBUG){if(e.type==SDL_JOYBUTTONDOWN&&e.jbutton.button==BUTTON_A){ScreenContext debug_ctx={0};debug_ctx.screen=&screen;buttondebug_handle_event(&debug_ctx,&e);}continue;}
   if(e.type==SDL_JOYBUTTONDOWN&&e.jbutton.button==BUTTON_SELECT&&!locked){locked=1;generate_unlock_sequence(unlock_sequence,UNLOCK_SEQUENCE_LEN);unlock_progress=0;lock_screen_visible_until=0;screen=SCREEN_PLAYER;continue;}
   if(locked){if(e.type==SDL_JOYBUTTONDOWN){Uint32 lock_now=SDL_GetTicks();lock_screen_visible_until=lock_now+5000U;if(e.jbutton.button==unlock_sequence[unlock_progress]){unlock_progress++;if(unlock_progress>=UNLOCK_SEQUENCE_LEN){locked=0;unlock_progress=0;lock_screen_visible_until=0;}}else{unlock_progress=0;unlock_wrong_flash_until=lock_now+400;}}continue;}
   if(e.type==SDL_KEYDOWN){if(e.key.keysym.scancode==(SDL_Scancode)KEY_VOLUME_UP){volume+=VOLUME_STEP;if(volume>MIX_MAX_VOLUME)volume=MIX_MAX_VOLUME;Mix_VolumeMusic(volume);save_state();continue;}if(e.key.keysym.scancode==(SDL_Scancode)KEY_VOLUME_DOWN){volume-=VOLUME_STEP;if(volume<0)volume=0;Mix_VolumeMusic(volume);save_state();continue;}}
   if(e.type==SDL_JOYBUTTONDOWN){
    if(screen==SCREEN_PLAYER&&streaming_is_active()){
        /* Stream-Tasten werden ausschliesslich in player_handle_event behandelt. */
    }else{
if(screen!=SCREEN_DOWNLOADS&&screen!=SCREEN_STREAMS&&e.jbutton.button==BUTTON_X){screen=SCREEN_SYSTEM_MENU;continue;}if(e.jbutton.button==BUTTON_START){if(!music&&track_count>0){music=play_track(tracks,track_index,base_position,&base_position,&started_ticks,&paused);if(music){duration=get_duration(music);last_save=SDL_GetTicks();}}else if(music){if(paused){Mix_ResumeMusic();started_ticks=SDL_GetTicks();paused=0;}else if(Mix_PlayingMusic()){base_position=get_position(base_position,started_ticks,0);Mix_PauseMusic();paused=1;}}continue;}if(screen!=SCREEN_DOWNLOADS&&screen!=SCREEN_STREAMS&&e.jbutton.button==BUTTON_Y){screen=(screen==SCREEN_PLAYER)?SCREEN_MENU:SCREEN_PLAYER;continue;}}}
   ScreenContext screen_ctx={renderer,font,white,selected,gray,&screen,&running,&book_index,&track_index,&book_count,&track_count,book_names,book_paths,tracks,&music,&base_position,&started_ticks,&duration,track_durations,&book_duration,&paused,&last_save,&axis_y_lock,&axis_x_lock,&sleep_timer_active,&sleep_timer_minutes,&sleep_timer_end_ticks,storage_paths,&storage_path_count,book_roots,book_track_counts,&cpu_usage,&ram_usage,&cpu_temperature,&battery_percent,&battery_charging,&idle_timer_remaining_ms,do_shutdown};
   switch(screen){case SCREEN_MENU:menu_handle_event(&screen_ctx,&e);break;case SCREEN_TRACKS:tracks_handle_event(&screen_ctx,&e);break;case SCREEN_PLAYER:player_handle_event(&screen_ctx,&e);break;case SCREEN_SLEEP_TIMER:sleeptimer_handle_event(&screen_ctx,&e);break;case SCREEN_SYSTEM_INFO:systeminfo_handle_event(&screen_ctx,&e);break;case SCREEN_BUTTON_DEBUG:buttondebug_handle_event(&screen_ctx,&e);break;case SCREEN_SYSTEM_MENU:systemmenu_handle_event(&screen_ctx,&e);break;case SCREEN_DOWNLOADS:downloadbrowser_handle_event(&screen_ctx,&e);idle_timer_last_tick=SDL_GetTicks();break;case SCREEN_STREAMS:streams_handle_event(&screen_ctx,&e);break;case SCREEN_LOG:logview_handle_event(&screen_ctx,&e);break;case SCREEN_BLUETOOTH:bluetoothscreen_handle_event(&screen_ctx,&e);break;case SCREEN_DOWNLOAD_SETTINGS:downloadsettings_handle_event(&screen_ctx,&e);break;case SCREEN_STREAM_SETTINGS:streamsettings_handle_event(&screen_ctx,&e);break;}
  }
  double mpris_pos=music?get_position(base_position,started_ticks,paused):base_position;
  char mpris_title[256]="",mpris_album_base[256]="",mpris_album[320]="",mpris_artist[256]="";
  if(track_count>0&&track_index>=0&&track_index<track_count){
   const char *tag_title=music?Mix_GetMusicTitleTag(music):NULL;const char *tag_album=music?Mix_GetMusicAlbumTag(music):NULL;const char *tag_artist=music?Mix_GetMusicArtistTag(music):NULL;
   if(tag_title&&tag_title[0])snprintf(mpris_title,sizeof(mpris_title),"%s",tag_title);else filename_without_extension(tracks[track_index].path,mpris_title,sizeof(mpris_title));
   if(tag_album&&tag_album[0])snprintf(mpris_album_base,sizeof(mpris_album_base),"%s",tag_album);else if(book_count>0&&book_index>=0&&book_index<book_count)snprintf(mpris_album_base,sizeof(mpris_album_base),"%s",book_names[book_index]);
   if(tag_artist&&tag_artist[0])snprintf(mpris_artist,sizeof(mpris_artist),"%s",tag_artist);else if(book_count>0&&book_index>=0&&book_index<book_count)parent_directory_name(book_paths[book_index],mpris_artist,sizeof(mpris_artist));
  }
  int overall_percent=0;if(book_duration>0.0){double book_pos=mpris_pos;for(int i=0;i<track_index&&i<track_count;i++)book_pos+=track_durations[i];if(book_pos<0)book_pos=0;if(book_pos>book_duration)book_pos=book_duration;overall_percent=(int)((book_pos/book_duration)*100.0+0.5);}
  if(mpris_album_base[0])snprintf(mpris_album,sizeof(mpris_album),"%s (%d%%)",mpris_album_base,overall_percent);
  int mpris_playing=music&&!paused&&Mix_PlayingMusic();
  if(bluetooth_present)mpris_bridge_update(&mpris,mpris_album,mpris_title,mpris_artist,track_count>0?track_index+1:0,track_count,duration,mpris_pos,music!=NULL,paused,mpris_playing,(double)volume/MIX_MAX_VOLUME);
  if(bluetooth_present)
   hfp_gateway_process(hfp_gateway);
  char hfp_number[64];
  MediaKeyAction hfp_action=MEDIA_KEY_NONE;
  if(bluetooth_present&&hfp_gateway_poll_dial(hfp_gateway,hfp_number,sizeof(hfp_number))){
   char *end=NULL;unsigned long dial=strtoul(hfp_number,&end,10);
   if(!hfp_number[0]||!end||*end||dial>999999999UL){app_logf("HFP: ungueltige Buchnummer %s",hfp_number);}
   else{int pi=find_book_progress_by_dial_id((unsigned int)dial);int target=-1;if(pi>=0){for(int i=0;i<book_count;i++)if(!strcmp(book_paths[i],progress[pi].path)){target=i;break;}}
    if(target<0)app_logf("HFP: kein Hoerspiel fuer Nummer %s",hfp_number);
    else{app_logf("HFP Hoerspiel: %s -> %s",hfp_number,book_names[target]);
     if(music){int oldpi=ensure_book_progress(book_paths[book_index]);if(oldpi>=0){progress[oldpi].track=track_index;progress[oldpi].position=get_position(base_position,started_ticks,paused);touch_book_progress(oldpi);}Mix_HaltMusic();Mix_FreeMusic(music);music=NULL;}
     book_index=target;track_count=scan_tracks(book_paths[book_index],tracks);book_duration=get_track_durations(tracks,track_count,track_durations);track_index=0;base_position=0.0;paused=1;duration=0.0;
     if(track_count>0){music=play_track(tracks,track_index,0.0,&base_position,&started_ticks,&paused);if(music){duration=get_duration(music);last_save=SDL_GetTicks();int newpi=ensure_book_progress(book_paths[book_index]);if(newpi>=0){progress[newpi].track=0;progress[newpi].position=0.0;touch_book_progress(newpi);}save_state();screen=SCREEN_PLAYER;last_activity=SDL_GetTicks();idle_timer_remaining_ms=idle_timer_minutes>0?(Uint32)idle_timer_minutes*60000U:0U;idle_timer_last_tick=SDL_GetTicks();}}
     else app_logf("HFP: Hoerspiel hat keine abspielbaren Tracks: %s",book_names[target]);
    }
   }
  }
  MediaKeyAction media_actions[32];int media_action_count=media_keys_poll(&media_keys,media_actions,32);if(bluetooth_present)media_action_count+=mpris_bridge_poll(&mpris,media_actions+media_action_count,32-media_action_count);if(hfp_action!=MEDIA_KEY_NONE&&media_action_count<32)media_actions[media_action_count++]=hfp_action;if(media_action_count>0){last_activity=SDL_GetTicks();idle_timer_remaining_ms=idle_timer_minutes>0?(Uint32)idle_timer_minutes*60000U:0U;idle_timer_last_tick=SDL_GetTicks();}
  if(screen!=SCREEN_BUTTON_DEBUG){for(int mi=0;mi<media_action_count;mi++){MediaKeyAction action=media_actions[mi];if(action==MEDIA_KEY_DISPLAY_TOGGLE){toggle_display_hw();last_activity=SDL_GetTicks();continue;}if(action==MEDIA_KEY_PREVIOUS||action==MEDIA_KEY_NEXT){if(track_count<=0)continue;if(music){int pi=ensure_book_progress(book_paths[book_index]);if(pi>=0){progress[pi].track=track_index;progress[pi].position=get_position(base_position,started_ticks,paused);touch_book_progress(pi);}Mix_HaltMusic();Mix_FreeMusic(music);music=NULL;}if(action==MEDIA_KEY_PREVIOUS){track_index--;if(track_index<0)track_index=track_count-1;}else{track_index++;if(track_index>=track_count)track_index=0;}base_position=0.0;music=play_track(tracks,track_index,0.0,&base_position,&started_ticks,&paused);if(music){duration=get_duration(music);last_save=SDL_GetTicks();int pi=ensure_book_progress(book_paths[book_index]);if(pi>=0){progress[pi].track=track_index;progress[pi].position=0.0;touch_book_progress(pi);}save_state();}continue;}if(action==MEDIA_KEY_PLAY_PAUSE){if(!music&&track_count>0){music=play_track(tracks,track_index,base_position,&base_position,&started_ticks,&paused);if(music){duration=get_duration(music);last_save=SDL_GetTicks();}}else if(music){if(paused){Mix_ResumeMusic();started_ticks=SDL_GetTicks();paused=0;}else if(Mix_PlayingMusic()){base_position=get_position(base_position,started_ticks,0);Mix_PauseMusic();paused=1;}}continue;}if(action==MEDIA_KEY_PLAY){if(!music&&track_count>0){music=play_track(tracks,track_index,base_position,&base_position,&started_ticks,&paused);if(music){duration=get_duration(music);last_save=SDL_GetTicks();}}else if(music&&paused){Mix_ResumeMusic();started_ticks=SDL_GetTicks();paused=0;}continue;}if(action==MEDIA_KEY_PAUSE){if(music&&!paused&&Mix_PlayingMusic()){base_position=get_position(base_position,started_ticks,0);Mix_PauseMusic();paused=1;}continue;}if(action==MEDIA_KEY_STOP){if(music){base_position=get_position(base_position,started_ticks,paused);int pi=ensure_book_progress(book_paths[book_index]);if(pi>=0){progress[pi].track=track_index;progress[pi].position=base_position;touch_book_progress(pi);}save_state();Mix_HaltMusic();Mix_FreeMusic(music);music=NULL;paused=1;started_ticks=SDL_GetTicks();}continue;}}}
  if(music&&!paused&&!Mix_PlayingMusic()){int pi=ensure_book_progress(book_paths[book_index]);int was_last_track=(track_index>=track_count-1);if(pi>=0){progress[pi].track=track_index;progress[pi].position=0.0;touch_book_progress(pi);}if(shutdown_after_tracks!=shutdown_tracks_setting_seen){shutdown_tracks_setting_seen=shutdown_after_tracks;shutdown_tracks_remaining=shutdown_after_tracks;}if(shutdown_tracks_remaining>0){shutdown_tracks_remaining--;if(shutdown_tracks_remaining==0){Mix_FreeMusic(music);music=NULL;do_shutdown(&music);running=0;continue;}}if(was_last_track){if(shutdown_at_book_end){Mix_FreeMusic(music);music=NULL;do_shutdown(&music);running=0;continue;}if(!repeat_book){Mix_FreeMusic(music);music=NULL;base_position=0.0;paused=1;if(pi>=0){progress[pi].track=track_count-1;progress[pi].position=track_durations[track_count-1];touch_book_progress(pi);save_state();}continue;}track_index=0;}else track_index++;Mix_FreeMusic(music);music=play_track(tracks,track_index,0.0,&base_position,&started_ticks,&paused);if(music)duration=get_duration(music);}
  if(music&&SDL_GetTicks()-last_save>=SAVE_INTERVAL_MS){int pi=ensure_book_progress(book_paths[book_index]);if(pi>=0){progress[pi].track=track_index;progress[pi].position=get_position(base_position,started_ticks,paused);touch_book_progress(pi);}save_state();last_save=SDL_GetTicks();}
  {Uint32 usage_now=SDL_GetTicks();Uint32 usage_elapsed=usage_now-usage_last_tick;if(usage_elapsed>=1000U){unsigned long long sec=usage_elapsed/1000U;usage_runtime_seconds+=sec;if(music&&!paused&&Mix_PlayingMusic())usage_playback_seconds+=sec;usage_last_tick+=(Uint32)(sec*1000ULL);}}
  if(locked&&lock_screen_visible_until&&SDL_GetTicks()>=lock_screen_visible_until){lock_screen_visible_until=0;unlock_progress=0;}
  if(idle_timer_minutes!=idle_setting_seen){idle_setting_seen=idle_timer_minutes;idle_timer_remaining_ms=idle_timer_minutes>0?(Uint32)idle_timer_minutes*60000U:0U;idle_timer_last_tick=SDL_GetTicks();}
  {Uint32 now=SDL_GetTicks();if(idle_timer_minutes<=0){idle_timer_remaining_ms=0;idle_timer_last_tick=now;}else if((music&&!paused&&Mix_PlayingMusic())||(streaming_is_active()&&!streaming_is_paused()))idle_timer_last_tick=now;else{Uint32 elapsed=now-idle_timer_last_tick;idle_timer_last_tick=now;if(elapsed>=idle_timer_remaining_ms){idle_timer_remaining_ms=0;do_shutdown(&music);running=0;}else idle_timer_remaining_ms-=elapsed;}}
  if(sleep_timer_active&&SDL_GetTicks()>=sleep_timer_end_ticks){do_shutdown(&music);running=0;}
  if(sleep_timer_active){Uint32 now=SDL_GetTicks(),rem=sleep_timer_end_ticks>now?sleep_timer_end_ticks-now:0;if(rem<=LED_BLINK_THRESHOLD_SEC*1000U){int blink_on=((now/(LED_BLINK_PERIOD_MS/2))%2)==0;led_set(blink_on);}else led_set(0);}else led_set(0);
  if(display_timeout_seconds>0&&!is_display_off()&&SDL_GetTicks()-last_activity>=(Uint32)display_timeout_seconds*1000U)set_display_off(1);
  if(bluetooth_present)
   battery_bluez_process(battery_bluez);
  if(SDL_GetTicks()-last_connectivity_state_check>=5000){
   network_log_if_changed();
   bluetooth_log_if_changed();
   last_connectivity_state_check=SDL_GetTicks();
  }
  if(SDL_GetTicks()-last_battery_check>=5000){
   battery_percent=get_battery_percent();
   battery_charging=is_battery_charging();
   if(bluetooth_present)battery_bluez_set_percent(battery_bluez,battery_percent);
   last_battery_check=SDL_GetTicks();
  }
  if(SDL_GetTicks()-last_systemstats_check>=2000){cpu_usage=get_cpu_usage();ram_usage=get_ram_usage();cpu_temperature=get_cpu_temperature();last_systemstats_check=SDL_GetTicks();}
  SDL_SetRenderDrawColor(renderer,15,15,20,255);SDL_RenderClear(renderer);ScreenContext screen_ctx={renderer,font,white,selected,gray,&screen,&running,&book_index,&track_index,&book_count,&track_count,book_names,book_paths,tracks,&music,&base_position,&started_ticks,&duration,track_durations,&book_duration,&paused,&last_save,&axis_y_lock,&axis_x_lock,&sleep_timer_active,&sleep_timer_minutes,&sleep_timer_end_ticks,storage_paths,&storage_path_count,book_roots,book_track_counts,&cpu_usage,&ram_usage,&cpu_temperature,&battery_percent,&battery_charging,&idle_timer_remaining_ms,do_shutdown};
  if(locked&&lock_screen_visible_until&&SDL_GetTicks()<lock_screen_visible_until){draw_text(renderer,font,"Tastensperre",20,40,selected);draw_text(renderer,font,"Zum Entsperren:",20,100,white);int x=20;for(int i=0;i<UNLOCK_SEQUENCE_LEN;i++){const char *name=button_name(unlock_sequence[i]);SDL_Color col=i<unlock_progress?selected:white;draw_text(renderer,font,name,x,150,col);int text_w=0,text_h=0;if(TTF_SizeUTF8(font,name,&text_w,&text_h)!=0)text_w=60;x+=text_w+28;}if(SDL_GetTicks()<unlock_wrong_flash_until)draw_text(renderer,font,"Falsche Taste",20,210,gray);draw_text(renderer,font,"5 s ohne Eingabe: zurueck zur Wiedergabe",20,270,gray);}else if(locked){
   player_render(&screen_ctx);
   /* Kleines Schloss oben rechts, getrennt von der Lautstaerkeanzeige. */
   SDL_Rect lock_body={SCREEN_W-36,54,16,13};
   SDL_Rect lock_shackle={SCREEN_W-33,47,10,10};
   SDL_SetRenderDrawColor(renderer,selected.r,selected.g,selected.b,255);
   SDL_RenderDrawRect(renderer,&lock_shackle);
   SDL_RenderFillRect(renderer,&lock_body);
  }else{switch(screen){case SCREEN_MENU:menu_render(&screen_ctx);break;case SCREEN_TRACKS:tracks_render(&screen_ctx);break;case SCREEN_PLAYER:player_render(&screen_ctx);break;case SCREEN_SLEEP_TIMER:sleeptimer_render(&screen_ctx);break;case SCREEN_SYSTEM_INFO:systeminfo_render(&screen_ctx);break;case SCREEN_BUTTON_DEBUG:buttondebug_render(&screen_ctx);break;case SCREEN_SYSTEM_MENU:systemmenu_render(&screen_ctx);break;case SCREEN_DOWNLOADS:downloadbrowser_render(&screen_ctx);break;case SCREEN_STREAMS:streams_render(&screen_ctx);break;case SCREEN_LOG:logview_render(&screen_ctx);break;case SCREEN_BLUETOOTH:bluetoothscreen_render(&screen_ctx);break;case SCREEN_DOWNLOAD_SETTINGS:downloadsettings_render(&screen_ctx);break;case SCREEN_STREAM_SETTINGS:streamsettings_render(&screen_ctx);break;}}
  #ifdef BUILD_BATOCERA
  if(display_needs_software_blank()){
   SDL_SetRenderDrawColor(renderer,0,0,0,255);
   SDL_RenderClear(renderer);
  }
#endif
  SDL_RenderPresent(renderer);
  {
   Uint32 now=SDL_GetTicks();
   Uint32 elapsed=now-frame_last_present;
   if(frame_last_present!=0 && elapsed<33U)SDL_Delay(33U-elapsed);
   frame_last_present=SDL_GetTicks();
  }SDL_Delay(10);
 }
 save_state();streaming_stop();hfp_gateway_close(hfp_gateway);battery_bluez_close(battery_bluez);mpris_bridge_close(&mpris);media_keys_close(&media_keys);if(music){Mix_HaltMusic();Mix_FreeMusic(music);}if(joy)SDL_JoystickClose(joy);if(font)TTF_CloseFont(font);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);Mix_CloseAudio();TTF_Quit();SDL_Quit();return 0;
}
