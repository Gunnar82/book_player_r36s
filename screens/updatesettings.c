#include "updatesettings.h"
#include "../config.h"
#include "../update_config.h"
#include "../ui.h"
#include <stdio.h>
#include <stdlib.h>

#define ROW_H 32
#define TOP_Y 58
#define MAX_ROWS 11

typedef struct {
    const char *label;
    const char *value;
    int selectable;
} Row;

static int selection = 0;
static int scroll_offset = 0;

static int build_rows(Row *rows)
{
    int n = 0;
    static char tls_mode[32];

    rows[n++] = (Row){"Installiert", APP_VERSION, 0};
    rows[n++] = (Row){"Update URL", update_base_url[0] ? update_base_url : "--", 0};
    snprintf(tls_mode, sizeof(tls_mode), "%s", update_use_download_tls ? "Downloads" : "Eigenstaendig");
    rows[n++] = (Row){"TLS Quelle", tls_mode, 0};
    rows[n++] = (Row){"TLS Peer", update_config_verify_peer() ? "Pruefen" : "Nicht pruefen", 0};
    rows[n++] = (Row){"TLS Host", update_config_verify_host() ? "Pruefen" : "Nicht pruefen", 0};
    rows[n++] = (Row){"CA Zertifikat", update_config_ca_cert()[0] ? update_config_ca_cert() : "System-CA", 0};
    rows[n++] = (Row){"Client Zertifikat", update_config_client_cert()[0] ? update_config_client_cert() : "--", 0};
    rows[n++] = (Row){"Client Key", update_config_client_key()[0] ? update_config_client_key() : "--", 0};
    rows[n++] = (Row){"Key Passwort", update_config_client_key_password()[0] ? "gesetzt" : "nicht gesetzt", 0};
    rows[n++] = (Row){"Nach Updates suchen", "<  Start  >", 1};
    return n;
}

static void keep_visible(int count)
{
    int max_offset;
    if (selection < scroll_offset) {
        scroll_offset = selection;
    }
    if (selection >= scroll_offset + MAX_ROWS) {
        scroll_offset = selection - MAX_ROWS + 1;
    }
    max_offset = count - MAX_ROWS;
    if (max_offset < 0) {
        max_offset = 0;
    }
    if (scroll_offset < 0) {
        scroll_offset = 0;
    }
    if (scroll_offset > max_offset) {
        scroll_offset = max_offset;
    }
}

void updatesettings_reset(void)
{
    selection = 9;
    scroll_offset = 0;
    keep_visible(10);
}

void updatesettings_handle_event(ScreenContext *c, const SDL_Event *e)
{
    Row rows[16];
    int count = build_rows(rows);

    if (e->type == SDL_JOYBUTTONDOWN) {
        int b = e->jbutton.button;
        if (b == BUTTON_B) {
            *c->screen = SCREEN_SYSTEM_INFO;
            return;
        }
        if (b == BUTTON_DPAD_UP) {
            do {
                selection--;
                if (selection < 0) {
                    selection = count - 1;
                }
            } while (!rows[selection].selectable);
            keep_visible(count);
            return;
        }
        if (b == BUTTON_DPAD_DOWN) {
            do {
                selection++;
                if (selection >= count) {
                    selection = 0;
                }
            } while (!rows[selection].selectable);
            keep_visible(count);
            return;
        }
    }

    if (e->type == SDL_JOYAXISMOTION && e->jaxis.axis == AXIS_Y) {
        if (!*c->axis_y_lock && abs(e->jaxis.value) > AXIS_DEADZONE) {
            selection = 9;
            keep_visible(count);
            *c->axis_y_lock = 1;
        }
        if (abs(e->jaxis.value) < AXIS_DEADZONE) {
            *c->axis_y_lock = 0;
        }
    }
}

void updatesettings_render(ScreenContext *c)
{
    Row rows[16];
    int count = build_rows(rows);
    int end = scroll_offset + MAX_ROWS;
    int y = TOP_Y;

    if (end > count) {
        end = count;
    }
    draw_text(c->renderer, c->font, "Updates", 20, 20, c->selected);
    for (int i = scroll_offset; i < end; i++, y += ROW_H) {
        SDL_Color col = i == selection ? c->selected : (rows[i].selectable ? c->white : c->gray);
        char line[1100];
        snprintf(line, sizeof(line), "%-18s %s", rows[i].label, rows[i].value);
        draw_text(c->renderer, c->font, line, 25, y, col);
    }
    draw_text(c->renderer, c->font, "A: Update-Pruefung   B: Zurueck", 20, SCREEN_H - 30, c->gray);
}
