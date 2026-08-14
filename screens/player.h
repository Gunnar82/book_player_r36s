#ifndef SCREEN_PLAYER_H
#define SCREEN_PLAYER_H
#include "screen_context.h"
void player_handle_event(ScreenContext *ctx, const SDL_Event *e);
void player_render(ScreenContext *ctx);
#endif
