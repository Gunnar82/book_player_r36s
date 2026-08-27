#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define UI_IMPLEMENTATION
#include "ui.h"
#include "state.h"
#include "storage.h"
#include "config.h"

typedef struct { const char *key; const char *name; SDL_Color color; } AccentDef;
static const AccentDef accents[] = {
    {"yellow", "Gelb",    {255,220, 80,255}},
    {"orange", "Orange", {255,150, 50,255}},
    {"red",    "Rot",    {240, 90, 90,255}},
    {"green",  "Gruen",  {100,220,120,255}},
    {"blue",   "Blau",   {100,170,255,255}},
    {"cyan",   "Tuerkis",{ 80,220,220,255}},
    {"violet", "Violett",{190,130,255,255}}
};
#define ACCENT_COUNT ((int)(sizeof(accents)/sizeof(accents[0])))
static int accent_index=0;
static int accent_loaded=0;
static TTF_Font *menu_fonts[4]={NULL,NULL,NULL,NULL};
static int menu_font_active=0;

static void trim_line(char *s){if(!s)return;char *p=s;while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;if(p!=s)memmove(s,p,strlen(p)+1);size_t n=strlen(s);while(n&&(s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\r'||s[n-1]=='\n'))s[--n]='\0';}
static void accent_load(void){if(accent_loaded)return;accent_loaded=1;accent_index=0;FILE *fp=fopen(get_storage_config_path(),"r");if(!fp)return;char line[1200];int in_ui=0;while(fgets(line,sizeof(line),fp)){trim_line(line);if(!line[0]||line[0]=='#'||line[0]==';')continue;if(line[0]=='['){in_ui=!strcmp(line,"[ui]");continue;}if(!in_ui)continue;if(!strncmp(line,"accent_color=",13)){char *v=line+13;trim_line(v);for(int i=0;i<ACCENT_COUNT;i++)if(!strcasecmp(v,accents[i].key)||!strcasecmp(v,accents[i].name)){accent_index=i;break;}break;}}fclose(fp);}
static int accent_save(void){const char *path=get_storage_config_path();FILE *fp=fopen(path,"r");char **lines=NULL;size_t count=0,cap=0;char line[1200];if(fp){while(fgets(line,sizeof(line),fp)){if(count==cap){size_t nc=cap?cap*2:32;char **tmp=realloc(lines,nc*sizeof(*tmp));if(!tmp){fclose(fp);goto fail;}lines=tmp;cap=nc;}lines[count]=strdup(line);if(!lines[count]){fclose(fp);goto fail;}count++;}fclose(fp);}char tmp_path[1200];snprintf(tmp_path,sizeof(tmp_path),"%s.tmp",path);fp=fopen(tmp_path,"w");if(!fp)goto fail;int in_ui=0,have_ui=0,wrote=0;for(size_t i=0;i<count;i++){char check[1200];snprintf(check,sizeof(check),"%s",lines[i]);trim_line(check);if(check[0]=='['){if(in_ui&&!wrote){fprintf(fp,"accent_color=%s\n",accents[accent_index].key);wrote=1;}in_ui=!strcmp(check,"[ui]");if(in_ui)have_ui=1;fputs(lines[i],fp);continue;}if(in_ui&&!strncmp(check,"accent_color=",13)){fprintf(fp,"accent_color=%s\n",accents[accent_index].key);wrote=1;continue;}fputs(lines[i],fp);}if(in_ui&&!wrote)fprintf(fp,"accent_color=%s\n",accents[accent_index].key);else if(!have_ui){if(count&&lines[count-1][0]&&lines[count-1][strlen(lines[count-1])-1]!='\n')fputc('\n',fp);fprintf(fp,"\n[ui]\naccent_color=%s\n",accents[accent_index].key);}if(fflush(fp)!=0||fsync(fileno(fp))!=0){fclose(fp);unlink(tmp_path);goto fail;}if(fclose(fp)!=0){unlink(tmp_path);goto fail;}if(rename(tmp_path,path)!=0){unlink(tmp_path);goto fail;}for(size_t i=0;i<count;i++)free(lines[i]);free(lines);return 0;fail:for(size_t i=0;i<count;i++)free(lines[i]);free(lines);return -1;}
SDL_Color ui_accent_color(void){accent_load();return accents[accent_index].color;}
const char *ui_accent_name(void){accent_load();return accents[accent_index].name;}
void ui_accent_cycle(int delta){accent_load();accent_index=(accent_index+delta)%ACCENT_COUNT;if(accent_index<0)accent_index+=ACCENT_COUNT;accent_save();}
static SDL_Color substitute_accent(SDL_Color color){if((color.r==255&&color.g==220&&color.b==80)||(color.r==230&&color.g==210&&color.b==70)){SDL_Color a=ui_accent_color();a.a=color.a;return a;}return color;}
int ui_set_render_draw_color(SDL_Renderer *renderer,Uint8 r,Uint8 g,Uint8 b,Uint8 a){SDL_Color c={r,g,b,a};c=substitute_accent(c);return SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);}

int menu_font_pixels(void){static const int sizes[]={18,20,26,32};int i=menu_font_size;if(i<0)i=0;if(i>3)i=3;return sizes[i];}
int menu_line_height(void){return menu_font_pixels()+8;}
static int menu_font_index(void){int i=menu_font_size;if(i<0)i=0;if(i>3)i=3;return i;}
TTF_Font *menu_font_get(TTF_Font *fallback){int i=menu_font_index();if(!menu_fonts[i])menu_fonts[i]=TTF_OpenFont(FONT_PATH,menu_font_pixels());return menu_fonts[i]?menu_fonts[i]:fallback;}
static TTF_Font *resolve_font(TTF_Font *fallback){return menu_font_active?menu_font_get(fallback):fallback;}
void menu_font_apply(TTF_Font *font){(void)font;menu_font_active=1;}
void menu_font_restore(TTF_Font *font){(void)font;menu_font_active=0;}

void draw_text(SDL_Renderer *r,TTF_Font *font,const char *text,int x,int y,SDL_Color color){font=resolve_font(font);if(!font)return;color=substitute_accent(color);SDL_Surface *s=TTF_RenderUTF8_Blended(font,text,color);if(!s)return;SDL_Texture *t=SDL_CreateTextureFromSurface(r,s);if(t){SDL_Rect dst={x,y,s->w,s->h};SDL_RenderCopy(r,t,NULL,&dst);SDL_DestroyTexture(t);}SDL_FreeSurface(s);}
void draw_text_right(SDL_Renderer *r,TTF_Font *font,const char *text,int right_x,int y,SDL_Color color){font=resolve_font(font);if(!font)return;int w=0,h=0;if(TTF_SizeUTF8(font,text,&w,&h)!=0)return;draw_text(r,font,text,right_x-w,y,color);}
void draw_scrollbar(SDL_Renderer *r,int x,int y,int height,int total_items,int visible_items,int first_visible){if(!r||height<=0||total_items<=visible_items||visible_items<=0)return;int max_first=total_items-visible_items;if(first_visible<0)first_visible=0;if(first_visible>max_first)first_visible=max_first;SDL_Rect track={x,y,5,height};SDL_SetRenderDrawColor(r,70,70,80,255);SDL_RenderFillRect(r,&track);int thumb_h=(height*visible_items)/total_items;if(thumb_h<18)thumb_h=18;if(thumb_h>height)thumb_h=height;int travel=height-thumb_h;int thumb_y=y+(max_first>0?(travel*first_visible)/max_first:0);SDL_Rect thumb={x,thumb_y,5,thumb_h};SDL_Color a=ui_accent_color();SDL_SetRenderDrawColor(r,a.r,a.g,a.b,255);SDL_RenderFillRect(r,&thumb);}
void format_time(double seconds,char *out,size_t size){if(seconds<0)seconds=0;int total=(int)seconds;int h=total/3600;int m=(total%3600)/60;int s=total%60;if(h)snprintf(out,size,"%d:%02d:%02d",h,m,s);else snprintf(out,size,"%02d:%02d",m,s);}
