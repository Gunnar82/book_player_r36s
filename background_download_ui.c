#include "background_download.h"
#include "download.h"
#include "input_config.h"
#include "config.h"
#include "ui.h"
#include "screens/downloadbrowser.h"
#include "screens/player.h"

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

typedef struct {
    ScreenContext *screen;
} ProgressUiHead;

static void format_rate(char *out,size_t n,double bps)
{
    if(bps<1.0)snprintf(out,n,"--");
    else if(bps>=1048576.0)snprintf(out,n,"%.1f MB/s",bps/1048576.0);
    else snprintf(out,n,"%.0f KB/s",bps/1024.0);
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

static double total_percent(const BackgroundDownloadStatus *s)
{
    if(!s||s->total_size<=0)return 0.0;
    double p=100.0*(double)s->total_now/(double)s->total_size;
    if(p<0.0)p=0.0;
    if(p>100.0)p=100.0;
    return p;
}

int __wrap_input_normalize_event(SDL_Event *e)
{
    int rc=__real_input_normalize_event(e);
    if(!rc||!e)return rc;
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

    foreground_waiting=1;
    background_requested=0;
    for(;;){
        BackgroundDownloadStatus s;
        background_download_get_status(&s);
        if(!s.active){
            foreground_waiting=0;
            background_download_wait();
            if(s.result<0&&error&&error_size)
                snprintf(error,error_size,"%s",s.error[0]?s.error:(s.cancelled?"Download abgebrochen":"Download fehlgeschlagen"));
            return s.result;
        }

        if(progress){
            int stop=progress(s.folder,s.filename,s.file_index,s.file_count,
                              s.file_now,s.file_total,s.total_now,s.total_size,userdata);
            if(stop){
                foreground_waiting=0;
                background_download_cancel();
                background_download_wait();
                if(error&&error_size)snprintf(error,error_size,"Download abgebrochen");
                return -1;
            }
            /* Der bestehende Progress-Screen kennt nur B. Solange er im
             * Vordergrund ist, ergaenzen wir dort die neue Y-Funktion. */
            if(userdata){
                ProgressUiHead *head=(ProgressUiHead*)userdata;
                if(head->screen&&head->screen->renderer&&head->screen->font){
                    draw_text(head->screen->renderer,head->screen->font,
                              "Y: Im Hintergrund",330,410,head->screen->gray);
                    SDL_RenderPresent(head->screen->renderer);
                }
            }
        }

        if(background_requested){
            foreground_waiting=0;
            if(error&&error_size)snprintf(error,error_size,"Download laeuft im Hintergrund");
            return -1;
        }
        SDL_Delay(30);
    }
}

static void render_active_download(ScreenContext *c,const BackgroundDownloadStatus *s)
{
    char line[768],rate[32],folder[120];
    double pct=total_percent(s);
    format_rate(rate,sizeof(rate),s->rate_bps);
    format_tail(folder,sizeof(folder),s->folder[0]?s->folder:"/",96);

    draw_text(c->renderer,c->font,"Download aktiv",20,20,c->selected);
    snprintf(line,sizeof(line),"Datei %d / %d: %s",s->file_index,s->file_count,s->filename[0]?s->filename:"...");
    draw_text(c->renderer,c->font,line,20,72,c->white);
    snprintf(line,sizeof(line),"Gesamt: %.1f / %.1f MB  (%.0f %%)",
             s->total_now/1048576.0,s->total_size/1048576.0,pct);
    draw_text(c->renderer,c->font,line,20,122,c->white);

    SDL_Rect bar={20,160,580,12};
    SDL_SetRenderDrawColor(c->renderer,70,70,70,255);
    SDL_RenderFillRect(c->renderer,&bar);
    SDL_Rect fill=bar; fill.w=(int)(bar.w*pct/100.0);
    SDL_SetRenderDrawColor(c->renderer,230,210,70,255);
    SDL_RenderFillRect(c->renderer,&fill);

    snprintf(line,sizeof(line),"Ordner: %s",folder);
    draw_text(c->renderer,c->font,line,20,210,c->gray);
    snprintf(line,sizeof(line),"Downloadrate: %s",rate);
    draw_text(c->renderer,c->font,line,20,252,c->gray);
    draw_text(c->renderer,c->font,"Y: Im Hintergrund   B: Abbrechen",20,SCREEN_H-35,c->gray);
}

void __wrap_downloadbrowser_render(ScreenContext *c)
{
    BackgroundDownloadStatus s;
    background_download_get_status(&s);
    if(s.active){render_active_download(c,&s);return;}
    __real_downloadbrowser_render(c);
}

void __wrap_downloadbrowser_handle_event(ScreenContext *c,const SDL_Event *e)
{
    BackgroundDownloadStatus s;
    background_download_get_status(&s);
    if(s.active&&e&&e->type==SDL_JOYBUTTONDOWN){
        if(e->jbutton.button==BUTTON_B){background_download_cancel();return;}
        if(e->jbutton.button==BUTTON_Y){*c->screen=SCREEN_PLAYER;return;}
        return;
    }
    __real_downloadbrowser_handle_event(c,e);
}

void __wrap_player_render(ScreenContext *c)
{
    __real_player_render(c);
    BackgroundDownloadStatus s;
    background_download_get_status(&s);
    if(!s.active)return;

    char rate[32],line[160];
    format_rate(rate,sizeof(rate),s.rate_bps);
    snprintf(line,sizeof(line),"DL %.0f%% · %s · %d/%d",
             total_percent(&s),rate,s.file_index,s.file_count);
    draw_text(c->renderer,c->font,line,20,SCREEN_H-62,c->selected);
}

void __wrap_SDL_Quit(void)
{
    /* SDL-Thread und Mutex muessen verschwinden, bevor SDL selbst beendet wird. */
    background_download_shutdown();
    __real_SDL_Quit();
}
