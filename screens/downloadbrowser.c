#include "downloadbrowser.h"
#include "../download.h"
#include "../storage.h"
#include "../ui.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int progress_render(const char *name,int file_index,int file_count,long long file_now,long long file_total,long long total_now,long long total_size,void *userdata){
    ScreenContext *c=(ScreenContext*)userdata;SDL_Event e;
    while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)return 1;if(e.type==SDL_JOYBUTTONDOWN && e.jbutton.button==BUTTON_B)return 1;}
    SDL_SetRenderDrawColor(c->renderer,20,20,30,255);SDL_RenderClear(c->renderer);draw_text(c->renderer,c->font,"Download",20,20,c->selected);
    char line[256];snprintf(line,sizeof(line),"%d / %d  %s",file_index,file_count,name?name:"");draw_text(c->renderer,c->font,line,20,90,c->white);
    if(file_total>0){snprintf(line,sizeof(line),"Datei: %.1f / %.1f MB",file_now/1048576.0,file_total/1048576.0);draw_text(c->renderer,c->font,line,20,135,c->white);}
    if(total_size>0){double pct=100.0*(double)total_now/(double)total_size;if(pct>100)pct=100;snprintf(line,sizeof(line),"Ordner: %.1f / %.1f MB  (%.0f %%)",total_now/1048576.0,total_size/1048576.0,pct);draw_text(c->renderer,c->font,line,20,180,c->white);SDL_Rect bar={20,225,580,10};SDL_SetRenderDrawColor(c->renderer,70,70,70,255);SDL_RenderFillRect(c->renderer,&bar);SDL_Rect fill=bar;fill.w=(int)(bar.w*pct/100.0);SDL_SetRenderDrawColor(c->renderer,230,210,70,255);SDL_RenderFillRect(c->renderer,&fill);}
    draw_text(c->renderer,c->font,"B: Abbrechen",20,390,c->gray);SDL_RenderPresent(c->renderer);return 0;
}

static void start_selected_download(ScreenContext *c){
    if(selected_count<=0){snprintf(status,sizeof(status),"Keine Auswahl. Y markiert Eintraege.");return;}
    char err[256]="";
    int rc=remote_download_selection(selected,selected_count,progress_render,c,err,sizeof(err));
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
