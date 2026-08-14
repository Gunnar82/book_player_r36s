#ifndef SCREEN_SLEEP_TIMER_H
#define SCREEN_SLEEP_TIMER_H
#include "screen_context.h"
void sleeptimer_handle_event(ScreenContext *ctx, const SDL_Event *e);
void sleeptimer_render(ScreenContext *ctx);
#endif
