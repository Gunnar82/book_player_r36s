#ifndef MEDIA_FEEDBACK_H
#define MEDIA_FEEDBACK_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "media_keys.h"

void media_feedback_show(MediaKeyAction action,int keycode,const char *source);
void media_feedback_render(SDL_Renderer *renderer,TTF_Font *font,SDL_Color fg,SDL_Color dim);

#endif
