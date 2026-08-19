#include "ui.h"
#include <stdio.h>

void draw_text(SDL_Renderer *r,TTF_Font *f,const char *text,int x,int y,SDL_Color c){
 if(!text||!text[0])return;SDL_Surface *s=TTF_RenderUTF8_Blended(f,text,c);if(!s)return;SDL_Texture *t=SDL_CreateTextureFromSurface(r,s);if(t){SDL_Rect d={x,y,s->w,s->h};SDL_RenderCopy(r,t,NULL,&d);SDL_DestroyTexture(t);}SDL_FreeSurface(s);
}
void draw_text_right(SDL_Renderer *r,TTF_Font *f,const char *text,int right_x,int y,SDL_Color c){
 if(!text||!text[0])return;int w=0,h=0;if(TTF_SizeUTF8(f,text,&w,&h)==0)draw_text(r,f,text,right_x-w,y,c);
}
void format_time(double sec,char *buf,size_t n){if(sec<0)sec=0;long s=(long)sec;long h=s/3600;long m=(s%3600)/60;long z=s%60;if(h>0)snprintf(buf,n,"%ld:%02ld:%02ld",h,m,z);else snprintf(buf,n,"%02ld:%02ld",m,z);}
