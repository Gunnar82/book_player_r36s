#ifndef DOWNLOADSETTINGS_H
#define DOWNLOADSETTINGS_H
#include "screen_context.h"
void downloadsettings_reset(void);
void downloadsettings_handle_event(ScreenContext *c,const SDL_Event *e);
void downloadsettings_render(ScreenContext *c);
#endif
