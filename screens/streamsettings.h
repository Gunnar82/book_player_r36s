#ifndef STREAMSETTINGS_H
#define STREAMSETTINGS_H
#include "screen_context.h"
void streamsettings_reset(void);
void streamsettings_handle_event(ScreenContext *c,const SDL_Event *e);
void streamsettings_render(ScreenContext *c);
#endif
