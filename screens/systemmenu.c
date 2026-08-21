#include "systemmenu.h"
#include "../ui.h"
#include "../storage.h"
#include "downloadbrowser.h"
#include "logview.h"
#include <stdlib.h>
#include <stdio.h>

static int selection = 0;
static const char *items[] = {
    "Einstellungen",
    "Downloads",
    "Beenden",
    "Herunterfahren"
};
#define ITEM_COUNT ((int)(sizeof(items)/sizeof(items[0])))

static void move_selection(int delta)
{
    selection += delta;
    if (selection < 0) selection = ITEM_COUNT - 1;
    if (selection >= ITEM_COUNT) selection = 0;
}

static void activate(ScreenContext *c)
{
    switch (selection) {
        case 0: *c->screen = SCREEN_SYSTEM_INFO; break;
        case 1:
            if (downloads_enabled) {
                downloadbrowser_reset();
                *c->screen = SCREEN_DOWNLOADS;
            }
            break;
        case 2: *c->running = 0; break;
        case 3:
            c->shutdown_fn(c->music);
            *c->running = 0;
            break;
    }
}

void systemmenu_handle_event(ScreenContext *c, const SDL_Event *e)
{
    if (e->type == SDL_JOYBUTTONDOWN) {
        int b = e->jbutton.button;
        if (b == BUTTON_B) { *c->screen = SCREEN_PLAYER; return; }
        if (b == BUTTON_DPAD_UP) { move_selection(-1); return; }
        if (b == BUTTON_DPAD_DOWN) { move_selection(1); return; }
        if (b == BUTTON_A) { activate(c); return; }
    }

    if (e->type == SDL_JOYAXISMOTION && e->jaxis.axis == AXIS_Y) {
        if (!*c->axis_y_lock && e->jaxis.value < -AXIS_DEADZONE) {
            move_selection(-1);
            *c->axis_y_lock = 1;
        } else if (!*c->axis_y_lock && e->jaxis.value > AXIS_DEADZONE) {
            move_selection(1);
            *c->axis_y_lock = 1;
        }
        if (abs(e->jaxis.value) < AXIS_DEADZONE)
            *c->axis_y_lock = 0;
    }
}

void systemmenu_render(ScreenContext *c)
{
    draw_text(c->renderer, c->font, "Systemmenue", 20, 20, c->selected);

    int y = 90;
    for (int i = 0; i < ITEM_COUNT; i++) {
        int disabled = (i == 1 && !downloads_enabled);
        SDL_Color col = disabled ? c->gray : ((i == selection) ? c->selected : c->white);
        char label[128];
        if (disabled) snprintf(label,sizeof(label),"%s  [deaktiviert]",items[i]);
        else snprintf(label,sizeof(label),"%s",items[i]);
        draw_text(c->renderer, c->font, label, 45, y, col);
        y += 42;
    }

    draw_text(c->renderer, c->font,
              "A: Auswaehlen   B: Player   Y: Hoerspiele",
              20, SCREEN_H - 35, c->gray);
}
