#include "systeminfo.h"
#include "../backlight.h"
#include "../state.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <stdlib.h>

static int page_offset = 0;
static int selected_line = 1;
static int sleep_timer_edit_minutes = -1;
static const int BRIGHTNESS_STEP = 5;
static const int LINE_H = 27;
static const int TOP_Y = 58;
static const int MAX_VISIBLE = 12;

typedef struct { char label[64]; char value[160]; int heading; } InfoLine;

static void add_line(InfoLine *lines,int *n,const char *label,const char *value,int heading){
    if(*n>=64)return;
    snprintf(lines[*n].label,sizeof(lines[*n].label),"%s",label?label:"");
    snprintf(lines[*n].value,sizeof(lines[*n].value),"%s",value?value:"");
    lines[*n].heading=heading; (*n)++;
}
static void format_uptime(char *out,size_t size){
    FILE *fp=fopen("/proc/uptime","r"); double seconds=0;
    if(fp){fscanf(fp,"%lf",&seconds);fclose(fp);} long sec=(long)seconds;
    int days=(int)(sec/86400);sec%=86400;int h=(int)(sec/3600);sec%=3600;int m=(int)(sec/60);sec%=60;
    if(days)snprintf(out,size,"%dd %02d:%02d:%02ld",days,h,m,sec);else snprintf(out,size,"%02d:%02d:%02ld",h,m,sec);
}
static void format_memory(char *out,size_t size){
    FILE *fp=fopen("/proc/meminfo","r"); unsigned long long total=0,avail=0,v; char key[64],unit[16];
    if(fp){while(fscanf(fp,"%63s %llu %15s",key,&v,unit)==3){if(!strcmp(key,"MemTotal:"))total=v;else if(!strcmp(key,"MemAvailable:"))avail=v;}fclose(fp);}
    if(!total){snprintf(out,size,"--");return;} snprintf(out,size,"%llu / %llu MB",(total-avail)/1024,total/1024);
}
static void format_free_space(const char *path,char *out,size_t size){
    struct statvfs v;if(statvfs(path,&v)!=0){snprintf(out,size,"nicht verfuegbar");return;}
    unsigned long long total=(unsigned long long)v.f_blocks*v.f_frsize,freeb=(unsigned long long)v.f_bavail*v.f_frsize;
    if(total>=1073741824ULL)snprintf(out,size,"%.1f GB frei / %.1f GB",freeb/1073741824.0,total/1073741824.0);
    else snprintf(out,size,"%.0f MB frei / %.0f MB",freeb/1048576.0,total/1048576.0);
}
static void format_audio_output(char *out,size_t size){
    const char *driver=SDL_GetCurrentAudioDriver();
    int count=SDL_GetNumAudioDevices(0);
    const char *device=(count>0)?SDL_GetAudioDeviceName(0,0):NULL;

    if(device&&device[0]&&driver&&driver[0])
        snprintf(out,size,"%s (%s)",device,driver);
    else if(device&&device[0])
        snprintf(out,size,"%s",device);
    else if(driver&&driver[0])
        snprintf(out,size,"%s",driver);
    else
        snprintf(out,size,"--");
}
static int build_lines(ScreenContext *c,InfoLine *lines){
    int n=0;char buf[160];
    add_line(lines,&n,"SYSTEM","",1); snprintf(buf,sizeof(buf),"%s",APP_VERSION); add_line(lines,&n,"Version",buf,0); struct utsname u;
    if(uname(&u)==0){add_line(lines,&n,"Kernel",u.release,0);add_line(lines,&n,"Architektur",u.machine,0);add_line(lines,&n,"Hostname",u.nodename,0);}
    format_uptime(buf,sizeof(buf));add_line(lines,&n,"Uptime",buf,0);
    add_line(lines,&n,"LEISTUNG","",1);
    if(*c->cpu_usage>=0)snprintf(buf,sizeof(buf),"%.0f %%",*c->cpu_usage);else strcpy(buf,"--");add_line(lines,&n,"CPU",buf,0);
    if(*c->cpu_temperature>=0)snprintf(buf,sizeof(buf),"%.1f °C",*c->cpu_temperature);else strcpy(buf,"--");add_line(lines,&n,"CPU-Temperatur",buf,0);
    format_memory(buf,sizeof(buf));add_line(lines,&n,"RAM",buf,0);
    if(*c->ram_usage>=0)snprintf(buf,sizeof(buf),"%.0f %%",*c->ram_usage);else strcpy(buf,"--");add_line(lines,&n,"RAM-Auslastung",buf,0);
    add_line(lines,&n,"AKKU / AUDIO","",1);
    if(*c->battery_percent>=0)snprintf(buf,sizeof(buf),"%d %%",*c->battery_percent);else strcpy(buf,"--");add_line(lines,&n,"Akku",buf,0);
    add_line(lines,&n,"Ladezustand",*c->battery_charging==1?"Laedt":"Entlaedt",0);
    snprintf(buf,sizeof(buf),"<  %d %%  >",(volume*100)/MIX_MAX_VOLUME);add_line(lines,&n,"Lautstaerke",buf,0);
    format_audio_output(buf,sizeof(buf));add_line(lines,&n,"Audio-Ausgabe",buf,0);
    add_line(lines,&n,"Display",is_display_off()?"Aus":"An",0);
    int brightness=get_brightness_percent();
    if(brightness>=0)snprintf(buf,sizeof(buf),"<  %d %%  >",brightness);else strcpy(buf,"--");
    add_line(lines,&n,"Helligkeit",buf,0);

    add_line(lines,&n,"TIMER","",1);
    int shown_sleep = sleep_timer_edit_minutes >= 0 ? sleep_timer_edit_minutes : *c->sleep_timer_minutes;
    if (sleep_timer_edit_minutes >= 0) {
        if (shown_sleep > 0) snprintf(buf,sizeof(buf),"<  %d min  >  A: OK",shown_sleep);
        else snprintf(buf,sizeof(buf),"<  Aus  >  A: OK");
    } else if (*c->sleep_timer_active) {
        Uint32 now = SDL_GetTicks();
        Uint32 remaining = (*c->sleep_timer_end_ticks > now) ? (*c->sleep_timer_end_ticks - now) : 0;
        int rem_min = (int)(remaining / 60000);
        int rem_sec = (int)((remaining / 1000) % 60);
        snprintf(buf,sizeof(buf),"<  %d min  >  %d:%02d",*c->sleep_timer_minutes,rem_min,rem_sec);
    } else if (*c->sleep_timer_minutes > 0) {
        snprintf(buf,sizeof(buf),"<  %d min  >",*c->sleep_timer_minutes);
    } else {
        snprintf(buf,sizeof(buf),"<  Aus  >");
    }
    add_line(lines,&n,"Sleeptimer",buf,0);

    if (idle_timer_minutes > 0)
        snprintf(buf,sizeof(buf),"<  %d min  >",idle_timer_minutes);
    else
        snprintf(buf,sizeof(buf),"<  Aus  >");
    add_line(lines,&n,"Idle-Timer",buf,0);

    add_line(lines,&n,"SPEICHER","",1);
    for(int i=0;i<*c->storage_path_count;i++){
        if(c->storage_paths[i].available){
            format_free_space(c->storage_paths[i].path,buf,sizeof(buf));
        }else{
            snprintf(buf,sizeof(buf),"nicht verfuegbar");
        }
        add_line(lines,&n,c->storage_paths[i].path,buf,0);
    }
    return n;
}
static int find_volume_line(InfoLine *lines,int count){for(int i=0;i<count;i++)if(!strcmp(lines[i].label,"Lautstaerke"))return i;return -1;}
static int find_brightness_line(InfoLine *lines,int count){for(int i=0;i<count;i++)if(!strcmp(lines[i].label,"Helligkeit"))return i;return -1;}
static int find_sleep_line(InfoLine *lines,int count){for(int i=0;i<count;i++)if(!strcmp(lines[i].label,"Sleeptimer"))return i;return -1;}
static int find_idle_line(InfoLine *lines,int count){for(int i=0;i<count;i++)if(!strcmp(lines[i].label,"Idle-Timer"))return i;return -1;}
static int next_selectable(InfoLine *lines,int count,int from,int dir){int i=from+dir;while(i>=0&&i<count){if(!lines[i].heading)return i;i+=dir;}return from;}
static void keep_visible(int count){int max=count-MAX_VISIBLE;if(max<0)max=0;if(selected_line<page_offset)page_offset=selected_line;if(selected_line>=page_offset+MAX_VISIBLE)page_offset=selected_line-MAX_VISIBLE+1;if(page_offset<0)page_offset=0;if(page_offset>max)page_offset=max;}
static void change_volume(int delta){
    volume += delta;
    if(volume < 0) volume = 0;
    if(volume > MIX_MAX_VOLUME) volume = MIX_MAX_VOLUME;
    Mix_VolumeMusic(volume);
    save_state();
}
static void change_brightness(int delta){int p=get_brightness_percent();if(p<0)return;p+=delta;if(p<10)p=10;if(p>100)p=100;set_brightness_percent(p);}
static void edit_sleep_timer(ScreenContext *c,int delta){
    if(sleep_timer_edit_minutes < 0) sleep_timer_edit_minutes = *c->sleep_timer_minutes;
    sleep_timer_edit_minutes += delta;
    if(sleep_timer_edit_minutes < SLEEP_MIN_MINUTES) sleep_timer_edit_minutes = SLEEP_MIN_MINUTES;
    if(sleep_timer_edit_minutes > SLEEP_MAX_MINUTES) sleep_timer_edit_minutes = SLEEP_MAX_MINUTES;
}
static void confirm_sleep_timer(ScreenContext *c){
    if(sleep_timer_edit_minutes < 0) return;
    *c->sleep_timer_minutes = sleep_timer_edit_minutes;
    if(*c->sleep_timer_minutes == 0){
        *c->sleep_timer_active = 0;
        *c->sleep_timer_end_ticks = 0;
    }else{
        *c->sleep_timer_active = 1;
        *c->sleep_timer_end_ticks = SDL_GetTicks() + (Uint32)(*c->sleep_timer_minutes * 60000);
    }
    sleep_timer_edit_minutes = -1;
}
static void change_idle_timer(int delta){
    idle_timer_minutes += delta;
    if(idle_timer_minutes < 0) idle_timer_minutes = 0;
    if(idle_timer_minutes > IDLE_TIMER_MAX_MINUTES) idle_timer_minutes = IDLE_TIMER_MAX_MINUTES;
    save_state();
}

void systeminfo_handle_event(ScreenContext *c,const SDL_Event *e){
    InfoLine lines[64];int count=build_lines(c,lines);if(selected_line>=count)selected_line=count-1;if(selected_line<0)selected_line=0;if(count>0&&lines[selected_line].heading)selected_line=next_selectable(lines,count,selected_line,1);
    if(e->type==SDL_JOYBUTTONDOWN){int b=e->jbutton.button;
        if(b==BUTTON_B){sleep_timer_edit_minutes=-1;page_offset=0;selected_line=1;*c->screen=SCREEN_MENU;return;}
        if(b==BUTTON_L1){for(int i=0;i<MAX_VISIBLE;i++)selected_line=next_selectable(lines,count,selected_line,-1);keep_visible(count);return;}
        if(b==BUTTON_L2){selected_line=0;while(selected_line<count&&lines[selected_line].heading)selected_line++;page_offset=0;return;}
        if(b==BUTTON_R1){for(int i=0;i<MAX_VISIBLE;i++)selected_line=next_selectable(lines,count,selected_line,1);keep_visible(count);return;}
        if(b==BUTTON_R2){selected_line=count-1;while(selected_line>0&&lines[selected_line].heading)selected_line--;keep_visible(count);return;}
        if(b==BUTTON_DPAD_UP){selected_line=next_selectable(lines,count,selected_line,-1);keep_visible(count);return;}
        if(b==BUTTON_DPAD_DOWN){selected_line=next_selectable(lines,count,selected_line,1);keep_visible(count);return;}
        int vl=find_volume_line(lines,count);
        int bl=find_brightness_line(lines,count);
        int sl=find_sleep_line(lines,count);
        int il=find_idle_line(lines,count);
        if(b==BUTTON_A&&selected_line==sl){confirm_sleep_timer(c);return;}
        if(b==BUTTON_DPAD_LEFT&&selected_line==vl){change_volume(-VOLUME_STEP);return;}
        if(b==BUTTON_DPAD_RIGHT&&selected_line==vl){change_volume(VOLUME_STEP);return;}
        if(b==BUTTON_DPAD_LEFT&&selected_line==bl){change_brightness(-BRIGHTNESS_STEP);return;}
        if(b==BUTTON_DPAD_RIGHT&&selected_line==bl){change_brightness(BRIGHTNESS_STEP);return;}
        if(b==BUTTON_DPAD_LEFT&&selected_line==sl){edit_sleep_timer(c,-SLEEP_STEP_MINUTES);return;}
        if(b==BUTTON_DPAD_RIGHT&&selected_line==sl){edit_sleep_timer(c,SLEEP_STEP_MINUTES);return;}
        if(b==BUTTON_DPAD_LEFT&&selected_line==il){change_idle_timer(-IDLE_TIMER_STEP_MINUTES);return;}
        if(b==BUTTON_DPAD_RIGHT&&selected_line==il){change_idle_timer(IDLE_TIMER_STEP_MINUTES);return;}
    }
    if(e->type==SDL_JOYAXISMOTION){
        if(e->jaxis.axis==AXIS_Y){if(!*c->axis_y_lock&&e->jaxis.value<-AXIS_DEADZONE){selected_line=next_selectable(lines,count,selected_line,-1);keep_visible(count);*c->axis_y_lock=1;}else if(!*c->axis_y_lock&&e->jaxis.value>AXIS_DEADZONE){selected_line=next_selectable(lines,count,selected_line,1);keep_visible(count);*c->axis_y_lock=1;}if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_y_lock=0;}
        if(e->jaxis.axis==AXIS_X){
            int vl=find_volume_line(lines,count), bl=find_brightness_line(lines,count), sl=find_sleep_line(lines,count), il=find_idle_line(lines,count);
            if(selected_line==vl){if(!*c->axis_x_lock&&e->jaxis.value<-AXIS_DEADZONE){change_volume(-VOLUME_STEP);*c->axis_x_lock=1;}else if(!*c->axis_x_lock&&e->jaxis.value>AXIS_DEADZONE){change_volume(VOLUME_STEP);*c->axis_x_lock=1;}}
            else if(selected_line==bl){if(!*c->axis_x_lock&&e->jaxis.value<-AXIS_DEADZONE){change_brightness(-BRIGHTNESS_STEP);*c->axis_x_lock=1;}else if(!*c->axis_x_lock&&e->jaxis.value>AXIS_DEADZONE){change_brightness(BRIGHTNESS_STEP);*c->axis_x_lock=1;}}
            else if(selected_line==sl){if(!*c->axis_x_lock&&e->jaxis.value<-AXIS_DEADZONE){edit_sleep_timer(c,-SLEEP_STEP_MINUTES);*c->axis_x_lock=1;}else if(!*c->axis_x_lock&&e->jaxis.value>AXIS_DEADZONE){edit_sleep_timer(c,SLEEP_STEP_MINUTES);*c->axis_x_lock=1;}}
            else if(selected_line==il){if(!*c->axis_x_lock&&e->jaxis.value<-AXIS_DEADZONE){change_idle_timer(-IDLE_TIMER_STEP_MINUTES);*c->axis_x_lock=1;}else if(!*c->axis_x_lock&&e->jaxis.value>AXIS_DEADZONE){change_idle_timer(IDLE_TIMER_STEP_MINUTES);*c->axis_x_lock=1;}}
            if(abs(e->jaxis.value)<AXIS_DEADZONE)*c->axis_x_lock=0;
        }
    }
}
void systeminfo_render(ScreenContext *c){
    InfoLine lines[64];int count=build_lines(c,lines),max_offset=count-MAX_VISIBLE;if(max_offset<0)max_offset=0;if(page_offset>max_offset)page_offset=max_offset;keep_visible(count);
    draw_text(c->renderer,c->font,"System",20,20,c->selected);int y=TOP_Y;
    for(int i=page_offset;i<count&&i<page_offset+MAX_VISIBLE;i++){if(lines[i].heading)draw_text(c->renderer,c->font,lines[i].label,25,y,c->selected);else{SDL_Color col=i==selected_line?c->selected:c->gray;draw_text(c->renderer,c->font,lines[i].label,35,y,col);draw_text_right(c->renderer,c->font,lines[i].value,SCREEN_W-35,y,i==selected_line?c->selected:c->white);}y+=LINE_H;}
    if(count>MAX_VISIBLE){int track_h=SCREEN_H-95,thumb_h=(track_h*MAX_VISIBLE)/count;if(thumb_h<18)thumb_h=18;int travel=track_h-thumb_h,thumb_y=55+(max_offset?(travel*page_offset)/max_offset:0);SDL_SetRenderDrawColor(c->renderer,70,70,80,255);SDL_Rect rail={SCREEN_W-8,55,2,track_h};SDL_RenderFillRect(c->renderer,&rail);SDL_SetRenderDrawColor(c->renderer,230,210,70,255);SDL_Rect thumb={SCREEN_W-8,thumb_y,2,thumb_h};SDL_RenderFillRect(c->renderer,&thumb);}
    draw_text(c->renderer,c->font,"B: Zurueck  A: OK  Hoch/Runter: Auswahl  Wert: Links/Rechts",20,SCREEN_H-25,c->gray);
}
