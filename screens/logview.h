#ifndef LOGVIEW_H
#define LOGVIEW_H
#include "screen_context.h"
void logview_handle_event(ScreenContext *c,const SDL_Event *e);
void logview_render(ScreenContext *c);
void logview_reset(void);
#endif
