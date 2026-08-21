#ifndef SCREEN_BLUETOOTH_H
#define SCREEN_BLUETOOTH_H
#include "screen_context.h"
void bluetoothscreen_reset(void);
void bluetoothscreen_handle_event(ScreenContext *c,const SDL_Event *e);
void bluetoothscreen_render(ScreenContext *c);
#endif
