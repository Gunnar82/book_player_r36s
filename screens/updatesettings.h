#ifndef UPDATESETTINGS_H
#define UPDATESETTINGS_H

#include "screen_context.h"

void updatesettings_reset(void);
void updatesettings_handle_event(ScreenContext *c, const SDL_Event *e);
void updatesettings_render(ScreenContext *c);

#endif
