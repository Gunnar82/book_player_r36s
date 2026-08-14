#ifndef SCREEN_BUTTONDEBUG_H
#define SCREEN_BUTTONDEBUG_H

#include "screen_context.h"

void buttondebug_handle_event(ScreenContext *ctx, const SDL_Event *e);
void buttondebug_render(ScreenContext *ctx);

#endif
