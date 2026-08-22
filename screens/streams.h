#ifndef SCREEN_STREAMS_H
#define SCREEN_STREAMS_H
#include "screen_context.h"
void streams_reset(void);
void streams_handle_event(ScreenContext *c,const SDL_Event *e);
void streams_render(ScreenContext *c);
#endif
