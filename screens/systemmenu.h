#ifndef SCREEN_SYSTEM_MENU_H
#define SCREEN_SYSTEM_MENU_H
#include "screen_context.h"
void systemmenu_handle_event(ScreenContext *ctx, const SDL_Event *e);
void systemmenu_render(ScreenContext *ctx);
#endif
