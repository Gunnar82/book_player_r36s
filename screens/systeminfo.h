#ifndef SCREEN_SYSTEMINFO_H
#define SCREEN_SYSTEMINFO_H
#include "screen_context.h"
void systeminfo_handle_event(ScreenContext *ctx, const SDL_Event *e);
void systeminfo_render(ScreenContext *ctx);
#endif
