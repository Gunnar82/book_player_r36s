#ifndef QR_RENDER_H
#define QR_RENDER_H

#include <SDL2/SDL.h>

/* Rendert einen echten QR-Code (libqrencode) mit Quiet-Zone. */
int qr_render_url(SDL_Renderer *renderer,const char *url,int x,int y,int pixel_size);

#endif
