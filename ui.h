#ifndef UI_H
#define UI_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stddef.h>
void draw_text(SDL_Renderer *r,TTF_Font *f,const char *text,int x,int y,SDL_Color c);
void draw_text_right(SDL_Renderer *r,TTF_Font *f,const char *text,int right_x,int y,SDL_Color c);
void format_time(double sec,char *buf,size_t n);
#endif
