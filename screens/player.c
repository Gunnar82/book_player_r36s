#include "../battery.h"
#include "player.h"
#include "../audio.h"
#include "../state.h"
#include "../backlight.h"
#include "../ui.h"
#include "../media_feedback.h"
#include "../streaming.h"
#include "../qr_render.h"
#include "streams.h"
#include <stdlib.h>

static Uint32 stream_meta_last_update=0;
static char stream_meta_station[256]="";
static char stream_meta_title[512]="";
static char stream_meta_extra[128]="";
static char stream_meta_description[512]="";



static Uint32 stream_title_scroll_started=0;
static char stream_title_scroll_text[256]="";

static void draw_stream_title_marquee(SDL_Renderer *r,TTF_Font *font,const char *text,
                                      int x,int y,int max_width,SDL_Color color){
 if(!text||!text[0])return;

 int w=0,h=0;
 if(TTF_SizeUTF8(font,text,&w,&h)!=0){
  draw_text(r,font,text,x,y,color);
  return;
 }

 if(w<=max_width){
  draw_text(r,font,text,x,y,color);
  return;
 }

 if(strcmp(stream_title_scroll_text,text)){
  snprintf(stream_title_scroll_text,sizeof(stream_title_scroll_text),"%s",text);
  stream_title_scroll_started=SDL_GetTicks();
 }

 const Uint32 pause_ms=1500;
 const int gap_px=80;
 const int speed_px_s=45;
 Uint32 elapsed=SDL_GetTicks()-stream_title_scroll_started;

 int cycle=w+gap_px;
 int offset=0;
 if(elapsed>pause_ms){
  offset=(int)(((Uint64)(elapsed-pause_ms)*speed_px_s)/1000ULL)%cycle;
 }

 SDL_Rect old_clip;
 SDL_RenderGetClipRect(r,&old_clip);
 SDL_Rect clip={x,y,max_width,h+4};
 SDL_RenderSetClipRect(r,&clip);

 draw_text(r,font,text,x-offset,y,color);
 draw_text(r,font,text,x-offset+cycle,y,color);

 if(old_clip.w>0&&old_clip.h>0)SDL_RenderSetClipRect(r,&old_clip);
 else SDL_RenderSetClipRect(r,NULL);
}

static void draw_stream_text_fit(SDL_Renderer *r,TTF_Font *font,const char *text,
                                 int x,int y,int max_width,SDL_Color color){
 if(!text||!text[0])return;
 char buf[768];
 snprintf(buf,sizeof(buf),"%s",text);

 int w=0,h=0;
 if(TTF_SizeUTF8(font,buf,&w,&h)!=0||w<=max_width){
  draw_text(r,font,buf,x,y,color);
  return;
 }

 size_t n=strlen(buf);
 while(n>4){
  n--;
  buf[n]='\0';
  if(n>=3){buf[n-3]='.';buf[n-2]='.';buf[n-1]='.';}
  if(TTF_SizeUTF8(font,buf,&w,&h)==0&&w<=max_width)break;
 }
 draw_text(r,font,buf,x,y,color);
}

static void start_current(ScreenContext *c,double resume){
 if(*c->track_count<=0)return;
 if(*c->music){Mix_HaltMusic();Mix_FreeMusic(*c->music);*c->music=NULL;}
 *c->music=play_track(c->tracks,*c->track_index,resume,c->base_position,c->started_ticks,c->paused);
 if(*c->music){*c->duration=get_duration(*c->music);*c->last_save=SDL_GetTicks();int pi=ensure_book_progress(c->book_paths[*c->book_index]);if(pi>=0){touch_book_progress(pi);save_state();}}
}

static void change_track(ScreenContext *c,int direction){
 if(*c->track_count<=0)return;
 *c->track_index+=direction;
 if(*c->track_index<0)*c->track_index=*c->track_count-1;
 if(*c->track_index>=*c->track_count)*c->track_index=0;
 start_current(c,0);
}

static void seek_relative(ScreenContext *c,double seconds){
 if(!*c->music)return;
 double p=get_position(*c->base_position,*c->started_ticks,*c->paused)+seconds;
 if(p<0)p=0;
 if(*c->duration>0&&p>*c->duration)p=*c->duration;
 if(Mix_SetMusicPosition(p)==0){
  *c->base_position=p;
  *c->started_ticks=SDL_GetTicks();
 }
}

void player_handle_event(ScreenContext *c,const SDL_Event *e){
 if(streaming_session_active()){
  if(e->type==SDL_JOYBUTTONDOWN){
   int b=e->jbutton.button;
   if(b==BUTTON_B||b==BUTTON_DPAD_LEFT){
    streaming_stop();
    stream_meta_last_update=0;
    stream_meta_station[0]='\0';
    stream_meta_title[0]='\0';
    stream_meta_extra[0]='\0';
    stream_meta_description[0]='\0';
    streams_reset();
    *c->screen=SCREEN_STREAMS;
    return;
   }
   if(b==BUTTON_A||b==BUTTON_START){
    if(streaming_is_active())streaming_toggle_pause();
    return;
   }
   return;
  }
  /* Auch Achsenbewegungen im Stream-Modus nicht an die lokale Wiedergabe geben. */
  if(e->type==SDL_JOYAXISMOTION)return;
 }

 if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;
  if(b==BUTTON_B){int pi=ensure_book_progress(c->book_paths[*c->book_index]);if(pi>=0){progress[pi].track=*c->track_index;progress[pi].position=get_position(*c->base_position,*c->started_ticks,*c->paused);}save_state();*c->screen=SCREEN_TRACKS;return;}
  if(b==BUTTON_A){if(!*c->music)start_current(c,*c->base_position);else if(*c->paused){Mix_ResumeMusic();*c->started_ticks=SDL_GetTicks();*c->paused=0;}else if(Mix_PlayingMusic()){*c->base_position=get_position(*c->base_position,*c->started_ticks,0);Mix_PauseMusic();*c->paused=1;}return;}
  if(b==BUTTON_DPAD_LEFT){change_track(c,-1);return;}
  if(b==BUTTON_DPAD_RIGHT){change_track(c,1);return;}
  if(b==BUTTON_DPAD_UP){seek_relative(c,SEEK_STEP);return;}
  if(b==BUTTON_DPAD_DOWN){seek_relative(c,-SEEK_STEP);return;}
 }
 if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_X){
  if(!*c->axis_x_lock&&e->jaxis.value<-AXIS_DEADZONE){change_track(c,-1);*c->axis_x_lock=1;}
  else if(!*c->axis_x_lock&&e->jaxis.value>AXIS_DEADZONE){change_track(c,1);*c->axis_x_lock=1;}
  if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_x_lock=0;
 }
 if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_Y){
  if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){seek_relative(c,SEEK_STEP);*c->axis_y_lock=1;}
  else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){seek_relative(c,-SEEK_STEP);*c->axis_y_lock=1;}
  if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;
 }
}

void player_render(ScreenContext *c){
 if(streaming_session_active()){
  int running=streaming_is_active();
  Uint32 now=SDL_GetTicks();

  if(running && (stream_meta_last_update==0 || now-stream_meta_last_update>=500U)){
   stream_meta_station[0]='\0';
   stream_meta_title[0]='\0';
   stream_meta_extra[0]='\0';
   stream_meta_description[0]='\0';
   streaming_get_metadata(stream_meta_station,sizeof(stream_meta_station),
                          stream_meta_title,sizeof(stream_meta_title),
                          stream_meta_extra,sizeof(stream_meta_extra));
   streaming_get_description(stream_meta_description,sizeof(stream_meta_description));
   if(!stream_meta_station[0])
    snprintf(stream_meta_station,sizeof(stream_meta_station),"%s",streaming_current_name());
   stream_meta_last_update=now;
  }

  if(!stream_meta_station[0])
   snprintf(stream_meta_station,sizeof(stream_meta_station),"%s",streaming_current_name());

  draw_stream_text_fit(c->renderer,c->font,stream_meta_station[0]?stream_meta_station:"Stream",20,50,SCREEN_W-220,c->gray);

  if(running){
   const int qr_x=SCREEN_W-175;
   const int qr_y=160;
   const int qr_size=155;
   const int text_max=qr_x-35;

   draw_stream_title_marquee(c->renderer,c->font,
                            stream_meta_title[0]?stream_meta_title:"Online Stream",
                            20,100,SCREEN_W-40,c->selected);
   draw_stream_text_fit(c->renderer,c->font,
                        stream_meta_extra[0]?stream_meta_extra:"LIVE",
                        20,145,text_max,c->white);
   if(stream_meta_description[0])
    draw_stream_text_fit(c->renderer,c->font,stream_meta_description,
                         20,185,text_max,c->gray);

   const char *stream_url=streaming_current_url();
   if(stream_url&&stream_url[0]){
    int real=qr_render_url(c->renderer,stream_url,qr_x,qr_y,qr_size);
    if(real>0)
     draw_text(c->renderer,c->font,"QR: Stream-URL",qr_x,qr_y+real+8,c->gray);
   }

   char idle[96];
   if(idle_timer_minutes>0 && c->idle_timer_remaining_ms){
    Uint32 rem=*c->idle_timer_remaining_ms;
    int mins=(int)(rem/60000),secs=(int)((rem/1000)%60);
    snprintf(idle,sizeof(idle),streaming_is_paused()?"Idle: %d:%02d":"Idle: %d:%02d (pausiert)",mins,secs);
    draw_text(c->renderer,c->font,idle,20,225,c->gray);
   }

   draw_text(c->renderer,c->font,streaming_is_paused()?"PAUSE":"LIVE",20,270,c->white);
   draw_text(c->renderer,c->font,"A/START: Pause/Play   B/Links: Streams",20,SCREEN_H-35,c->gray);
  }else{
   draw_text(c->renderer,c->font,"Stream konnte nicht wiedergegeben werden",20,100,c->selected);
   draw_text(c->renderer,c->font,"Backend wurde beendet",20,145,c->gray);
   draw_text(c->renderer,c->font,"B/Links: Zurueck zu Streams",20,SCREEN_H-35,c->gray);
  }
  return;
 }
 if(*c->track_count<=0){draw_text(c->renderer,c->font,"Keine Hoerspiele gefunden",20,100,c->gray);media_feedback_render(c->renderer,c->font,c->selected,c->gray);return;}
 double pos=get_position(*c->base_position,*c->started_ticks,*c->paused);if(pos<0)pos=0;
 double pct=*c->duration>0?(pos/ *c->duration)*100.0:0;if(pct>100)pct=100;
 char ptxt[64];format_time(pos,ptxt,sizeof(ptxt));
 char battery[64],vol[64];
 if(c->battery_percent&&*c->battery_percent>=0){
  if(c->battery_charging&&*c->battery_charging==1){
   int rem=get_battery_charge_remaining_minutes();
   if(rem==0)snprintf(battery,sizeof(battery),"Akku: %d %%  voll",*c->battery_percent);
   else if(rem>0&&rem<60)snprintf(battery,sizeof(battery),"Akku: %d %%  ~%d min bis voll",*c->battery_percent,rem);
   else if(rem>=60)snprintf(battery,sizeof(battery),"Akku: %d %%  ~%dh%02d bis voll",*c->battery_percent,rem/60,rem%60);
   else snprintf(battery,sizeof(battery),"Akku: %d %% (laedt)",*c->battery_percent);
  }else{
   int rem=get_battery_remaining_minutes();
   if(rem>=0)snprintf(battery,sizeof(battery),"Akku: %d %%  ~%dh%02d",*c->battery_percent,rem/60,rem%60);
   else snprintf(battery,sizeof(battery),"Akku: %d %%",*c->battery_percent);
  }
 }else snprintf(battery,sizeof(battery),"Akku: --");
 snprintf(vol,sizeof(vol),"Lautstaerke: %d %%",(volume*100)/MIX_MAX_VOLUME);
 draw_text(c->renderer,c->font,battery,20,20,c->gray);
 draw_text_right(c->renderer,c->font,vol,SCREEN_W-20,20,c->gray);
 draw_text(c->renderer,c->font,c->book_names[*c->book_index],20,50,c->gray);
 const char *track_display=c->tracks[*c->track_index].name;if(*c->music){const char *id3_title=Mix_GetMusicTitleTag(*c->music);if(id3_title&&id3_title[0])track_display=id3_title;}
 draw_text(c->renderer,c->font,track_display,20,100,c->selected);
 char tr[64];snprintf(tr,sizeof(tr),"Track %d / %d",*c->track_index+1,*c->track_count);draw_text(c->renderer,c->font,tr,20,142,c->white);
 if(*c->duration>0){
  char total[32],time[128];format_time(*c->duration,total,sizeof(total));
  snprintf(time,sizeof(time),"Track: %s / %s  (%.0f %%)",ptxt,total,pct);
  draw_text(c->renderer,c->font,time,20,178,c->white);
  SDL_Rect bar={20,212,580,10};SDL_SetRenderDrawColor(c->renderer,70,70,70,255);SDL_RenderFillRect(c->renderer,&bar);
  SDL_Rect fill=bar;fill.w=(int)(bar.w*pct/100.0);SDL_SetRenderDrawColor(c->renderer,230,210,70,255);SDL_RenderFillRect(c->renderer,&fill);
 }else{draw_text(c->renderer,c->font,ptxt,20,178,c->white);draw_text(c->renderer,c->font,"Track-Gesamtzeit nicht verfuegbar",20,212,c->gray);}
 if(*c->book_duration>0.0){
  double book_pos=pos;for(int i=0;i<*c->track_index;i++)book_pos+=c->track_durations[i];if(book_pos<0)book_pos=0;if(book_pos>*c->book_duration)book_pos=*c->book_duration;
  double book_pct=(book_pos/ *c->book_duration)*100.0;char bp[32],bt[32],overall[128];format_time(book_pos,bp,sizeof(bp));format_time(*c->book_duration,bt,sizeof(bt));
  snprintf(overall,sizeof(overall),"Gesamt: %s / %s  (%.0f %%)",bp,bt,book_pct);draw_text(c->renderer,c->font,overall,20,255,c->white);
  SDL_Rect bbar={20,286,580,6};SDL_SetRenderDrawColor(c->renderer,70,70,70,255);SDL_RenderFillRect(c->renderer,&bbar);SDL_Rect bfill=bbar;bfill.w=(int)(bbar.w*book_pct/100.0);SDL_SetRenderDrawColor(c->renderer,230,210,70,255);SDL_RenderFillRect(c->renderer,&bfill);
 }else{draw_text(c->renderer,c->font,"Hoerspiel-Gesamtzeit nicht verfuegbar",20,255,c->gray);}
 int timer_y=315;
 if(c->sleep_timer_active&&*c->sleep_timer_active&&c->sleep_timer_end_ticks){Uint32 now=SDL_GetTicks();Uint32 rem=*c->sleep_timer_end_ticks>now?*c->sleep_timer_end_ticks-now:0;char sleep[64];int mins=(int)(rem/60000),secs=(int)((rem/1000)%60);snprintf(sleep,sizeof(sleep),"Sleeptimer: %d:%02d",mins,secs);draw_text(c->renderer,c->font,sleep,20,timer_y,c->gray);timer_y+=24;}
 if(idle_timer_minutes>0 && c->idle_timer_remaining_ms){Uint32 rem=*c->idle_timer_remaining_ms;char idle[64];int mins=(int)(rem/60000),secs=(int)((rem/1000)%60);int playing=(*c->music && !*c->paused && Mix_PlayingMusic());snprintf(idle,sizeof(idle),playing?"Idle: %d:%02d (pausiert)":"Idle: %d:%02d",mins,secs);draw_text(c->renderer,c->font,idle,20,timer_y,c->gray);}
 draw_text(c->renderer,c->font,*c->paused?"PAUSE":"Wiedergabe",20,360,c->white);
 draw_text(c->renderer,c->font,"A: Play/Pause   B: Zurueck   X: System   Y: Hoerspiele",20,395,c->gray);
 draw_text(c->renderer,c->font,"Hoch/Runter: +15s/-15s   Links/Rechts: Track -/+",20,425,c->gray);
 media_feedback_render(c->renderer,c->font,c->selected,c->gray);
}
