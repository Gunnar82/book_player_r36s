#include "systemmenu.h"
#include "../ui.h"
#include "../storage.h"
#include "../systemstats.h"
#include "../streaming.h"
#include "downloadbrowser.h"
#include "streams.h"
#include "logview.h"
#include <stdlib.h>
#include <stdio.h>

static int selection = 0;
static const char *items[] = {
    "Einstellungen",
    "Downloads",
    "Streams",
    "Akzentfarbe",
    "Beenden",
    "Herunterfahren"
};
#define ITEM_COUNT ((int)(sizeof(items)/sizeof(items[0])))

static int item_enabled(int index)
{
    if(index==1) return downloads_enabled && network_connection_active();
    if(index==2) return network_connection_active() && stream_xml_url[0];
    return 1;
}

static void move_selection(int delta)
{
    int start=selection;
    do {
        selection += delta;
        if(selection < 0) selection = ITEM_COUNT - 1;
        if(selection >= ITEM_COUNT) selection = 0;
        if(item_enabled(selection)) return;
    } while(selection != start);
}

static void activate(ScreenContext *c)
{
    if(!item_enabled(selection)) return;
    switch (selection) {
        case 0: *c->screen = SCREEN_SYSTEM_INFO; break;
        case 1:
            if (downloads_enabled && network_connection_active()) { downloadbrowser_reset(); *c->screen = SCREEN_DOWNLOADS; }
            break;
        case 2:
            if (network_connection_active() && stream_xml_url[0]) { streams_reset(); *c->screen = SCREEN_STREAMS; }
            break;
        case 3: ui_accent_cycle(1); break;
        case 4: *c->running = 0; break;
        case 5: c->shutdown_fn(c->music); *c->running = 0; break;
    }
}

void systemmenu_handle_event(ScreenContext *c, const SDL_Event *e)
{
    if (e->type == SDL_JOYBUTTONDOWN) {
        int b = e->jbutton.button;
        if (b == BUTTON_B || b == BUTTON_DPAD_LEFT) {
            if(selection==3 && b==BUTTON_DPAD_LEFT){ui_accent_cycle(-1);return;}
            *c->screen = SCREEN_PLAYER; return;
        }
        if (b == BUTTON_DPAD_RIGHT) { if(selection==3){ui_accent_cycle(1);return;} activate(c); return; }
        if (b == BUTTON_DPAD_UP) { move_selection(-1); return; }
        if (b == BUTTON_DPAD_DOWN) { move_selection(1); return; }
        if (b == BUTTON_A) { activate(c); return; }
    }

    if (e->type == SDL_JOYAXISMOTION && e->jaxis.axis == AXIS_Y) {
        if (!*c->axis_y_lock && e->jaxis.value < -AXIS_DEADZONE) { move_selection(-1); *c->axis_y_lock = 1; }
        else if (!*c->axis_y_lock && e->jaxis.value > AXIS_DEADZONE) { move_selection(1); *c->axis_y_lock = 1; }
        if (abs(e->jaxis.value) < AXIS_DEADZONE) *c->axis_y_lock = 0;
    }
}

void systemmenu_render(ScreenContext *c)
{
    if(!item_enabled(selection)) move_selection(1);
    menu_font_apply(c->font);
    draw_text(c->renderer, c->font, "Systemmenue", 20, 20, c->selected);

    int y = 90;
    int network_ok = network_connection_active();
    for (int i = 0; i < ITEM_COUNT; i++) {
        int disabled = 0;
        char label[128];
        if (i == 1) {
            disabled = (!downloads_enabled || !network_ok);
            if (!downloads_enabled) snprintf(label,sizeof(label),"Downloads  [deaktiviert]");
            else if (!network_ok) snprintf(label,sizeof(label),"Downloads  [kein Netzwerk]");
            else snprintf(label,sizeof(label),"%s",items[i]);
        } else if (i == 2) {
            disabled = (!network_ok || !stream_xml_url[0]);
            if (!stream_xml_url[0]) snprintf(label,sizeof(label),"Streams  [XML fehlt]");
            else if (!network_ok) snprintf(label,sizeof(label),"Streams  [kein Netzwerk]");
            else snprintf(label,sizeof(label),"%s",items[i]);
        } else if(i==3) {
            snprintf(label,sizeof(label),"Akzentfarbe  < %s >",ui_accent_name());
        } else snprintf(label,sizeof(label),"%s",items[i]);

        SDL_Color col = disabled ? c->gray : ((i == selection) ? c->selected : c->white);
        draw_text(c->renderer, c->font, label, 45, y, col);
        y += menu_line_height()+10;
    }

    draw_text(c->renderer, c->font,
              "Rechts/A: Auswaehlen   Links/B: Player   Y: Hoerspiele",
              20, SCREEN_H - 35, c->gray);
    menu_font_restore(c->font);
}
