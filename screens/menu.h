#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H
#include "screen_context.h"

void menu_handle_event(ScreenContext *ctx, const SDL_Event *e);
void menu_render(ScreenContext *ctx);

/*
 * main.c bindet backlight.h vor menu.h ein. Dadurch koennen wir den
 * bisherigen globalen X-Aufruf (toggle_display) ohne Eingriff in main.c
 * auf das Zusatzmenue umleiten. In menu.c selbst ist BACKLIGHT_H beim
 * Einbinden dieses Headers noch nicht definiert, daher bleibt die echte
 * Backlight-Funktion dort unangetastet.
 */
#ifdef BACKLIGHT_H
#define toggle_display() do { screen = SCREEN_SLEEP_TIMER; } while (0)
#endif

#endif
