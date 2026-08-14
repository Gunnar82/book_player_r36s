#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H
#include "screen_context.h"
void menu_handle_event(ScreenContext *ctx, const SDL_Event *e);
void menu_render(ScreenContext *ctx);
#endif
