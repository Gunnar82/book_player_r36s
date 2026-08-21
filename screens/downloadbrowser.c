#include "downloadbrowser.h"
#include "../download.h"
#include "../storage.h"
#include "../ui.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ROW_H 30
#define TOP_Y 72
#define BOTTOM_Y 380
#define MAX_ROWS ((BOTTOM_Y-TOP_Y)/ROW_H)
#define MAX_SELECTIONS 256

static RemoteEntry entries[REMOTE_MAX_ENTRIES];
static int entry_count=0;
static int selection=0;
static int scroll_offset=0;
static int loaded=0;
static int loading=0;
static char relative_path[REMOTE_PATH_LEN]="";
static char status[256]="";
static int finished_download=0;
static RemoteSelection selected[MAX_SELECTIONS];
static int selected_count=0;

typedef struct {
    ScreenContext *screen;
    Uint32 total_start;
    Uint32 file_start;
    int last_file_index;
    char last_name[REMOTE_NAME_LEN];
} DownloadUiProgress;

static void keep_visible(void){int rows=MAX_ROWS;if(selection<scroll_offset)scroll_offset=selection;if(selection>=scroll_offset+rows)scroll_offset=selection-rows+1;int max=entry_count-rows;if(max<0)max=0;if(scroll_offset<0)scroll_offset=0;if(scroll_offset>max)scroll_offset=max;}
static void load_current(void){loading=1;status[0]='\0';entry_count=remote_fetch_listing(relative_path,entries,REMOTE_MAX_ENTRIES,status,sizeof(status));if(entry_count<0)entry_count=0;selection=0;scroll_offset=0;loaded=1;loading=0;finished_download=0;}
static void parent_dir(void){if(!relative_path[0])return;char *slash=strrchr(relative_path,'/');if(slash)*slash='\0';else relative_path[0]='\0';loaded=0;}
static void enter_directory(const char *name){if(relative_path[0])strncat(relative_path,"/",sizeof(relative_path)-strlen(relative_path)-1);strncat(relative_path,name,sizeof(relative_path)-strlen(relative_path)-1);loaded=0;}
static void full_path_for_entry(int idx,char *out,size_t out_size){if(idx<0||idx>=entry_count){if(out_size)out[0]='\0';return;}if(relative_path[0])snprintf(out,out_size,"%s/%s",relative_path,entries[idx].name);else snprintf(out,out_size,"%s",entries[idx].name);}
static int selected_index(const char *path){for(int i=0;i<selected_count;i++)if(!strcmp(selected[i].relative_path,path))return i;return -1;}
static int is_selected_entry(int idx){char path[REMOTE_PATH_LEN];full_path_for_entry(idx,path,sizeof(path));return selected_index(path)>=0;}
static void toggle_selected(int idx){
    if(idx<0||idx>=entry_count)return;
    char path[REMOTE_PATH_LEN];full_path_for_entry(idx,path,sizeof(path));
    int si=selected_index(path);
    if(si>=0){for(int i=si;i<selected_count-1;i++)selected[i]=selected[i+1];selected_count--;snprintf(status,sizeof(status),"%d ausgewaehlt",selected_count);return;}
    if(selected_count>=MAX_SELECTIONS){snprintf(status,sizeof(status),"Maximal %d Eintraege markierbar",MAX_SELECTIONS);return;}
    selected[selected_count].type=entries[idx].type;
    snprintf(selected[selected_count].relative_path,sizeof(selected[selected_count].relative_path),"%s",path);
    selected_count++;
    snprintf(status,sizeof(status),"%d ausgewaehlt",selected_count);
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

static void draw_bar(ScreenContext *c,int y,double pct)
{
    if(pct<0)pct=0;
    if(pct>100)pct=100;
    SDL_Rect bar={20,y,580,12};
    SDL_SetRenderDrawColor(c->renderer,70,70,70,255);
    SDL_RenderFillRect(c->renderer,&bar);
    SDL_Rect fill=bar;
    fill.w=(int)(bar.w*pct/100.0);
    SDL_SetRenderDrawColor(c->renderer,230,210,70,255);
    SDL_RenderFillRect(c->renderer,&fill);
}

static int progress_render(const char *name,int file_index,int file_count,long long file_now,long long file_total,long long total_now,long long total_size,void *userdata)
{
    DownloadUiProgress *ui=(DownloadUiProgress*)userdata;
    ScreenContext *c=ui->screen;
    SDL_Event e;
    while(SDL_PollEvent(&e)){
        if(e.type==SDL_QUIT)return 1;
        if(e.type==SDL_JOYBUTTONDOWN&&e.jbutton.button==BUTTON_B)return 1;
    }

    Uint32 now=SDL_GetTicks();
    if(ui->last_file_index!=file_index||strcmp(ui->last_name,name?name:"")){
        ui->last_file_index=file_index;
        snprintf(ui->last_name,sizeof(ui->last_name),"%s",name?name:"");
        ui->file_start=now;
    }

    double file_pct=file_total>0?100.0*(double)file_now/(double)file_total:0.0;
    double total_pct=total_size>0?100.0*(double)total_now/(double)total_size:0.0;
    if(file_pct>100)file_pct=100;
    if(total_pct>100)total_pct=100;

    double file_elapsed=(now-ui->file_start)/1000.0;
    double total_elapsed=(now-ui->total_start)/1000.0;
    double file_eta=-1.0,total_eta=-1.0;
    if(file_total>file_now&&file_now>0&&file_elapsed>0.5){
        double rate=file_now/file_elapsed;
        if(rate>0)file_eta=(file_total-file_now)/rate;
    }
    if(total_size>total_now&&total_now>0&&total_elapsed>0.5){
        double rate=total_now/total_elapsed;
        if(rate>0)total_eta=(total_size-total_now)/rate;
    }

    char file_eta_text[32],total_eta_text[32],line[512];
    format_eta(file_eta_text,sizeof(file_eta_text),file_eta);
    format_eta(total_eta_text,sizeof(total_eta_text),total_eta);

    SDL_SetRenderDrawColor(c->renderer,20,20,30,255);
    SDL_RenderClear(c->renderer);
    draw_text(c->renderer,c->font,"Download",20,18,c->selected);

    snprintf(line,sizeof(line),"Datei %d / %d: %s",file_index,file_count,name?name:"");
    draw_text(c->renderer,c->font,line,20,62,c->white);

    if(file_total>0)snprintf(line,sizeof(line),"Datei: %.1f / %.1f MB  (%.0f %%)",file_now/1048576.0,file_total/1048576.0,file_pct);
    else snprintf(line,sizeof(line),"Datei: %.1f MB",file_now/1048576.0);
    draw_text(c->renderer,c->font,line,20,108,c->white);
    draw_bar(c,142,file_pct);
    snprintf(line,sizeof(line),"Restzeit Datei: %s",file_eta_text);
    draw_text(c->renderer,c->font,line,20,164,c->gray);

    if(total_size>0)snprintf(line,sizeof(line),"Gesamt: %.1f / %.1f MB  (%.0f %%)",total_now/1048576.0,total_size/1048576.0,total_pct);
    else snprintf(line,sizeof(line),"Gesamt: %.1f MB",total_now/1048576.0);
    draw_text(c->renderer,c->font,line,20,218,c->white);
    draw_bar(c,252,total_pct);
    snprintf(line,sizeof(line),"Restzeit Gesamt: %s",total_eta_text);
    draw_text(c->renderer,c->font,line,20,274,c->gray);

    draw_text(c->renderer,c->font,"B: Abbrechen",20,410,c->gray);
    SDL_RenderPresent(c->renderer);
    return 0;
}

static void start_selected_download(ScreenContext *c)
{
    if(selected_count<=0){snprintf(status,sizeof(status),"Keine Auswahl. Y markiert Eintraege.");return;}
    char err[256]="";
    DownloadUiProgress ui={0};
    ui.screen=c;
    ui.total_start=SDL_GetTicks();
    ui.file_start=ui.total_start;
    ui.last_file_index=-1;
    Uint32 idle_before=c->idle_timer_remaining_ms?*c->idle_timer_remaining_ms:0;
    int rc=remote_download_selection(selected,selected_count,progress_render,&ui,err,sizeof(err));
    if(c->idle_timer_remaining_ms&&idle_before>0){
        Uint32 elapsed=SDL_GetTicks()-ui.total_start;
        uint64_t compensated=(uint64_t)idle_before+(uint64_t)elapsed;
        *c->idle_timer_remaining_ms=compensated>UINT32_MAX?UINT32_MAX:(Uint32)compensated;
    }
    if(rc>=0){snprintf(status,sizeof(status),"%d Dateien geladen. Neustart zum Einlesen.",rc);finished_download=1;selected_count=0;}
    else snprintf(status,sizeof(status),"%s",err[0]?err:"Download fehlgeschlagen");
}

void downloadbrowser_reset(void){relative_path[0]='\0';entry_count=0;selection=0;scroll_offset=0;loaded=0;status[0]='\0';finished_download=0;selected_count=0;}

void downloadbrowser_handle_event(ScreenContext *c,const SDL_Event *e){
    if(!loaded&&!loading)load_current();
    if(e->type==SDL_JOYBUTTONDOWN){
        int b=e->jbutton.button;
        if(b==BUTTON_B){if(relative_path[0]){parent_dir();return;}*c->screen=SCREEN_PLAYER;return;}
        if(b==BUTTON_DPAD_UP){selection--;if(selection<0)selection=entry_count>0?entry_count-1:0;keep_visible();return;}
        if(b==BUTTON_DPAD_DOWN){selection++;if(selection>=entry_count)selection=0;keep_visible();return;}
        if(b==BUTTON_L1){selection-=LIST_PAGE_SIZE;if(selection<0)selection=0;keep_visible();return;}
        if(b==BUTTON_R1){selection+=LIST_PAGE_SIZE;if(selection>=entry_count)selection=entry_count>0?entry_count-1:0;keep_visible();return;}
        if(b==BUTTON_L2){selection=0;keep_visible();return;}
        if(b==BUTTON_R2){selection=entry_count>0?entry_count-1:0;keep_visible();return;}
        if(b==BUTTON_Y&&entry_count>0){toggle_selected(selection);return;}
        if(b==BUTTON_X){start_selected_download(c);return;}
        if(b==BUTTON_A&&entry_count>0&&entries[selection].type==REMOTE_DIRECTORY){enter_directory(entries[selection].name);return;}
    }
    if(e->type==SDL_JOYAXISMOTION&&e->jaxis.axis==AXIS_Y){
        if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){selection--;if(selection<0)selection=entry_count>0?entry_count-1:0;keep_visible();*c->axis_y_lock=1;}
        else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){selection++;if(selection>=entry_count)selection=0;keep_visible();*c->axis_y_lock=1;}
        if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;
    }
}

void downloadbrowser_render(ScreenContext *c){
    if(!loaded&&!loading)load_current();
    draw_text(c->renderer,c->font,"Downloads",20,20,c->selected);
    char pathline[2200];if(relative_path[0])snprintf(pathline,sizeof(pathline),"/%s",relative_path);else snprintf(pathline,sizeof(pathline),"/");draw_text(c->renderer,c->font,pathline,20,48,c->gray);
    if(entry_count==0){if(status[0])draw_text(c->renderer,c->font,status,20,100,c->gray);else draw_text(c->renderer,c->font,"Kein Listing verfuegbar",20,100,c->gray);draw_text(c->renderer,c->font,relative_path[0]?"B: Hoeher":"B: Hauptbildschirm",20,SCREEN_H-25,c->gray);return;}
    int end=scroll_offset+MAX_ROWS;if(end>entry_count)end=entry_count;int y=TOP_Y;
    for(int i=scroll_offset;i<end;i++,y+=ROW_H){
        SDL_Color col=i==selection?c->selected:c->white;
        char label[680];const char *mark=is_selected_entry(i)?"[x]":"[ ]";
        if(entries[i].type==REMOTE_DIRECTORY)snprintf(label,sizeof(label),"%s %s/",mark,entries[i].name);
        else if(entries[i].size>0)snprintf(label,sizeof(label),"%s %s  (%.1f MB)",mark,entries[i].name,entries[i].size/1048576.0);
        else snprintf(label,sizeof(label),"%s %s",mark,entries[i].name);
        draw_text(c->renderer,c->font,label,20,y,col);
    }
    char sel[96];snprintf(sel,sizeof(sel),"%d ausgewaehlt",selected_count);draw_text(c->renderer,c->font,sel,20,392,c->selected);
    if(status[0])draw_text(c->renderer,c->font,status,170,392,finished_download?c->selected:c->gray);
    draw_text(c->renderer,c->font,relative_path[0]?"A: Oeffnen  B: Hoeher  Y: Markieren  X: Auswahl laden":"A: Oeffnen  B: Hauptbildschirm  Y: Markieren  X: Auswahl laden",20,SCREEN_H-25,c->gray);
}
