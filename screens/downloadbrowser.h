#ifndef SCREEN_DOWNLOAD_BROWSER_H
#define SCREEN_DOWNLOAD_BROWSER_H
#include "screen_context.h"
void downloadbrowser_handle_event(ScreenContext *ctx,const SDL_Event *e);
void downloadbrowser_render(ScreenContext *ctx);
void downloadbrowser_reset(void);
#endif
