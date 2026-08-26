#include "background_download.h"
#include "download.h"
#include "input_config.h"
#include "config.h"
#include "ui.h"
#include "screens/downloadbrowser.h"
#include "screens/player.h"
#ifdef BUILD_BATOCERA
#include "batocera_bluetooth.h"
#endif

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

int __real_input_normalize_event(SDL_Event *e);
void __real_downloadbrowser_handle_event(ScreenContext *c,const SDL_Event *e);
void __real_downloadbrowser_render(ScreenContext *c);
void __real_player_render(ScreenContext *c);
void __real_SDL_Quit(void);

static volatile int foreground_waiting;
static volatile int background_requested;
#ifdef BUILD_R36S
static int r36s_bluetooth_input_active;
void r36s_bluetooth_input_set_active(int active){r36s_bluetooth_input_active=active?1:0;}
#endif

static void format_rate(char *out,size_t n,double bps)
{
    if(bps<1.0)snprintf(out,n,"--");
    else if(bps>=1048576.0)snprintf(out,n,"%.2f MB/s",bps/1048576.0);
    else snprintf(out,n,"%.0f KB/s",bps/1024.0);
}

static void format_eta(char *out,size_t out_size,double seconds)
{
    if(seconds<0.0||seconds>359999.0){snprintf(out,out_size,"--:--");return;}
    long s=(long)(seconds+0.5);
    long h=s/3600;
    long m=(s%3600)/60;
    long sec=s%60;
    if(h>0)snprintf(out,out_size,"%ld:%02ld:%02ld",h,m,sec);
    else snprintf(out,out_size,"%02ld:%02ld",m,sec);
}

static void format_tail(char *out,size_t out_size,const char *text,size_t max_chars)
{
    if(!out||out_size==0)return;
    if(!text){out[0]='\0';return;}
    size_t n=strlen(text);
    if(n<=max_chars){snprintf(out,out_size,"%s",text);return;}
    size_t keep=max_chars>3?max_chars-3:0;
    snprintf(out,out_size,"...%s",text+n-keep);
}

static void format_folder_lines(char *line1,size_t line1_size,char *line2,size_t line2_size,const char *folder)
{
    line1[0]='\0';
    line2[0]='\0';
    if(!folder||!folder[0]){snprintf(line1,line1_size,"/");return;}

    char path[REMOTE_PATH_LEN+2];
    snprintf(path,sizeof(path),"%s%s",folder[0]=='/'?"":"/",folder);
    char *last=strrchr(path,'/');
    if(!last||last==path){
        format_tail(line1,line1_size,last&&last[1]?last+1:path,54);
        return;
    }

    char leaf[REMOTE_NAME_LEN+8];
    snprintf(leaf,sizeof(leaf),"%s",last+1);
    *last='\0';
    format_tail(line1,line1_size,path,54);
    format_tail(line2,line2_size,leaf,54);
}

static double percent(long long now,long long total)
{
    if(total<=0)return 0.0;
    double p=100.0*(double)now/(double)total;
    if(p<0.0)p=0.0;
    if(p>100.0)p=100.0;
    return p;
}

static double total_percent(const BackgroundDownloadStatus *s)
{
    return s?percent(s->total_now,s->total_size):0.0;
}

static void draw_bar(ScreenContext *c,int y,double pct)
{
    if(pct<0.0)pct=0.0;
    if(pct>100.0)pct=100.0;
    SDL_Rect bar={20,y,580,12};
    SDL_SetRenderDrawColor(c->renderer,70,70,70,255);
    SDL_RenderFillRect(c->renderer,&bar);
    SDL_Rect fill=bar;
    fill.w=(int)(bar.w*pct/100.0);
    SDL_SetRenderDrawColor(c->renderer,230,210,70,255);
    SDL_RenderFillRect(c->renderer,&fill);
}

int __wrap_input_normalize_event(SDL_Event *e)
{
    int rc=__real_input_normalize_event(e);
    if(!rc||!e)return rc;
#ifdef BUILD_BATOCERA
    batocera_bluetooth_remap_event(e);
#endif
#ifdef BUILD_R36S
    if(r36s_bluetooth_input_active&&
       (e->type==SDL_JOYBUTTONDOWN||e->type==SDL_JOYBUTTONUP)&&
       e->jbutton.button==BUTTON_X)
        e->jbutton.button=BUTTON_R1;
#endif
    if(foreground_waiting&&e->type==SDL_JOYBUTTONDOWN&&e->jbutton.button==BUTTON_Y){
        background_requested=1;
        return 0;
    }
    return rc;
}

int __wrap_remote_download_selection(const RemoteSelection *selection,
                                     int selection_count,
                                     DownloadProgressFn progress,
                                     void *userdata,
                                     char *error,
                                     size_t error_size)
{
    if(background_download_start(selection,selection_count,error,error_size)!=0)return -1;
    foreground_waiting=1;background_requested=0;
    for(;;){
        BackgroundDownloadStatus s;background_download_get_status(&s);
        if(!s.active){foreground_waiting=0;background_download_wait();if(s.result<0&&error&&error_size)snprintf(error,error_size,"%s",s.error[0]?s.error:(s.cancelled?"Download abgebrochen":"Download fehlgeschlagen"));return s.result;}
        if(progress){int stop=progress(s.folder,s.filename,s.file_index,s.file_count,s.file_now,s.file_total,s.total_now,s.total_size,userdata);if(stop){foreground_waiting=0;background_download_cancel();background_download_wait();if(error&&error_size)snprintf(error,error_size,"Download abgebrochen");return -1;}}
        if(background_requested){foreground_waiting=0;if(error&&error_size)snprintf(error,error_size,"Download laeuft im Hintergrund");return -1;}
        SDL_Delay(30);
    }
}

static void render_active_download(ScreenContext *c,const BackgroundDownloadStatus *s)
{
    char line[768],rate[32],file_eta[32],total_eta[32],folder_line1[96],folder_line2[96];
    double file_pct=percent(s->file_now,s->file_total);
    double total_pct=total_percent(s);
    double file_seconds=-1.0,total_seconds=-1.0;

    if(s->rate_bps>0.0){
        if(s->file_total>s->file_now)
            file_seconds=(double)(s->file_total-s->file_now)/s->rate_bps;
        if(s->total_size>s->total_now)
            total_seconds=(double)(s->total_size-s->total_now)/s->rate_bps;
    }

    format_rate(rate,sizeof(rate),s->rate_bps);
    format_eta(file_eta,sizeof(file_eta),file_seconds);
    format_eta(total_eta,sizeof(total_eta),total_seconds);
    format_folder_lines(folder_line1,sizeof(folder_line1),folder_line2,sizeof(folder_line2),s->folder);

    draw_text(c->renderer,c->font,"Download",20,18,c->selected);

    snprintf(line,sizeof(line),"Datei %d / %d: %s",s->file_index,s->file_count,s->filename[0]?s->filename:"...");
    draw_text(c->renderer,c->font,line,20,62,c->white);

    if(s->file_total>0)snprintf(line,sizeof(line),"Datei: %.1f / %.1f MB  (%.0f %%)",s->file_now/1048576.0,s->file_total/1048576.0,file_pct);
    else snprintf(line,sizeof(line),"Datei: %.1f MB",s->file_now/1048576.0);
    draw_text(c->renderer,c->font,line,20,108,c->white);
    draw_bar(c,142,file_pct);
    snprintf(line,sizeof(line),"Restzeit Datei: %s",file_eta);
    draw_text(c->renderer,c->font,line,20,164,c->gray);

    if(s->total_size>0)snprintf(line,sizeof(line),"Gesamt: %.1f / %.1f MB  (%.0f %%)",s->total_now/1048576.0,s->total_size/1048576.0,total_pct);
    else snprintf(line,sizeof(line),"Gesamt: %.1f MB",s->total_now/1048576.0);
    draw_text(c->renderer,c->font,line,20,218,c->white);
    draw_bar(c,252,total_pct);
    snprintf(line,sizeof(line),"Restzeit Gesamt: %s",total_eta);
    draw_text(c->renderer,c->font,line,20,274,c->gray);

    snprintf(line,sizeof(line),"Ordner: %s",folder_line1);
    draw_text(c->renderer,c->font,line,20,312,c->gray);
    if(folder_line2[0])draw_text(c->renderer,c->font,folder_line2,95,334,c->gray);
    snprintf(line,sizeof(line),"Downloadrate: %s",rate);
    draw_text(c->renderer,c->font,line,20,360,c->gray);

    draw_text(c->renderer,c->font,"B: Abbrechen   Y: Im Hintergrund",20,410,c->gray);
}

void __wrap_downloadbrowser_render(ScreenContext *c){BackgroundDownloadStatus s;background_download_get_status(&s);if(s.active){render_active_download(c,&s);return;}__real_downloadbrowser_render(c);}
void __wrap_downloadbrowser_handle_event(ScreenContext *c,const SDL_Event *e){BackgroundDownloadStatus s;background_download_get_status(&s);if(s.active&&e&&e->type==SDL_JOYBUTTONDOWN){if(e->jbutton.button==BUTTON_B){background_download_cancel();return;}if(e->jbutton.button==BUTTON_Y){*c->screen=SCREEN_PLAYER;return;}return;}__real_downloadbrowser_handle_event(c,e);}
void __wrap_player_render(ScreenContext *c){__real_player_render(c);BackgroundDownloadStatus s;background_download_get_status(&s);if(!s.active)return;char rate[32],line[160];format_rate(rate,sizeof(rate),s.rate_bps);snprintf(line,sizeof(line),"DL %.0f%% · %s · %d/%d",total_percent(&s),rate,s.file_index,s.file_count);draw_text(c->renderer,c->font,line,20,335,c->selected);}
void __wrap_SDL_Quit(void){
#ifdef BUILD_BATOCERA
    batocera_bluetooth_stop_live_devices();
#endif
    background_download_shutdown();__real_SDL_Quit();
}
