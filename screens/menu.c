#include "menu.h"
#include "../scanner.h"
#include "../audio.h"
#include "../state.h"
#include "../backlight.h"
#include "../ui.h"
#include "../bluetooth.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BROWSER_ENTRIES 512
#define MENU_TOP_Y 20
#define MENU_BOTTOM_Y (SCREEN_H - 82)
#define MENU_ROW_H (menu_line_height())

typedef enum {
    BROWSER_HEADING = 0,
    BROWSER_PARENT,
    BROWSER_DIRECTORY,
    BROWSER_BOOK,
    BROWSER_PLAY_CURRENT
} BrowserEntryType;

typedef struct {
    BrowserEntryType type;
    char label[256];
    char path[512];
} BrowserEntry;

static BrowserEntry entries[MAX_BROWSER_ENTRIES];
static int entry_count = 0;
static int selection = 0;
static int scroll_offset = 0;
static int dirty = 1;

void menu_invalidate(void)
{
    dirty = 1;
}
static char current_path[512] = "";
static char current_root[512] = "";
static char return_selection_path[512] = "";

static int entry_selectable(int i)
{
    return i >= 0 && i < entry_count && entries[i].type != BROWSER_HEADING;
}

static void add_entry(BrowserEntryType type, const char *label,
                      const char *path)
{
    if (entry_count >= MAX_BROWSER_ENTRIES) return;
    entries[entry_count].type = type;
    snprintf(entries[entry_count].label, sizeof(entries[entry_count].label),
             "%s", label ? label : "");
    snprintf(entries[entry_count].path, sizeof(entries[entry_count].path),
             "%s", path ? path : "");
    entry_count++;
}

static int find_book_index(ScreenContext *c, const char *path)
{
    for (int i = 0; i < *c->book_count; i++)
        if (!strcmp(c->book_paths[i], path)) return i;
    return -1;
}

static void add_directory_children(const char *path)
{
    char names[MAX_BOOKS][256];
    char paths[MAX_BOOKS][512];
    int count = scan_books(path, names, paths);

    for (int i = 0; i < count; i++) {
        int has_audio = directory_has_audio(paths[i]);
        int has_dirs = directory_has_subdirectories(paths[i]);

        if (has_dirs) {
            char label[256];
            snprintf(label, sizeof(label), "%s/", names[i]);
            add_entry(BROWSER_DIRECTORY, label, paths[i]);
        } else if (has_audio) {
            char label[256];
            unsigned int dial_id = ensure_book_dial_id(paths[i]);
            if (bluetooth_adapter_present() && dial_id >= 1001)
                snprintf(label, sizeof(label), "%s [%u]", names[i], dial_id);
            else
                snprintf(label, sizeof(label), "%s", names[i]);
            add_entry(BROWSER_BOOK, label, paths[i]);
        }
    }
}

static void build_root(ScreenContext *c)
{
    for (int si = 0; si < *c->storage_path_count; si++) {
        if (!c->storage_paths[si].available) continue;

        int before = entry_count;
        add_entry(BROWSER_HEADING, c->storage_paths[si].path, "");

        if (directory_has_audio(c->storage_paths[si].path))
            add_entry(BROWSER_PLAY_CURRENT, "[Dieses Hoerspiel abspielen]",
                      c->storage_paths[si].path);

        add_directory_children(c->storage_paths[si].path);

        if (entry_count == before + 1)
            entry_count = before;
    }
}

static void build_current_directory(void)
{
    add_entry(BROWSER_PARENT, "↵ Zurueck", "");

    if (directory_has_audio(current_path))
        add_entry(BROWSER_PLAY_CURRENT, "[Dieses Hoerspiel abspielen]",
                  current_path);

    add_directory_children(current_path);
}

static void rebuild(ScreenContext *c)
{
    if (!dirty) return;
    entry_count = 0;

    if (!current_path[0]) build_root(c);
    else build_current_directory();

    if (selection >= entry_count) selection = entry_count - 1;
    if (selection < 0) selection = 0;

    if (return_selection_path[0]) {
        for (int i = 0; i < entry_count; i++) {
            if (entry_selectable(i) && entries[i].path[0] &&
                !strcmp(entries[i].path, return_selection_path)) {
                selection = i;
                break;
            }
        }
        return_selection_path[0] = '\0';
    }

    if (entry_count > 0 && !entry_selectable(selection)) {
        int found = -1;
        for (int i = selection; i < entry_count; i++)
            if (entry_selectable(i)) { found = i; break; }
        if (found < 0)
            for (int i = selection - 1; i >= 0; i--)
                if (entry_selectable(i)) { found = i; break; }
        if (found >= 0) selection = found;
    }

    dirty = 0;
}

static int next_selectable(int from, int direction)
{
    if (entry_count <= 0) return from;

    int i = from + direction;
    while (i >= 0 && i < entry_count) {
        if (entry_selectable(i)) return i;
        i += direction;
    }

    if (direction < 0) {
        for (i = entry_count - 1; i >= 0; i--)
            if (entry_selectable(i)) return i;
    } else {
        for (i = 0; i < entry_count; i++)
            if (entry_selectable(i)) return i;
    }

    return from;
}

static int visible_rows(void)
{
    int rows = (MENU_BOTTOM_Y - MENU_TOP_Y) / MENU_ROW_H;
    if (current_path[0]) rows--;
    if (rows < 1) rows = 1;
    return rows;
}

static void keep_selection_visible(void)
{
    int rows = visible_rows();
    if (selection < scroll_offset) scroll_offset = selection;
    if (selection >= scroll_offset + rows)
        scroll_offset = selection - rows + 1;

    int max_scroll = entry_count - rows;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll_offset < 0) scroll_offset = 0;
    if (scroll_offset > max_scroll) scroll_offset = max_scroll;
}

static void move_selection(int direction, int steps)
{
    for (int n = 0; n < steps; n++)
        selection = next_selectable(selection, direction);
    keep_selection_visible();
}

static void go_to_start(void)
{
    for (int i = 0; i < entry_count; i++) {
        if (entry_selectable(i)) { selection = i; break; }
    }
    keep_selection_visible();
}

static void go_to_end(void)
{
    for (int i = entry_count - 1; i >= 0; i--) {
        if (entry_selectable(i)) { selection = i; break; }
    }
    keep_selection_visible();
}

static void parent_directory(void)
{
    if (!current_path[0]) return;

    snprintf(return_selection_path, sizeof(return_selection_path), "%s", current_path);

    if (!strcmp(current_path, current_root)) {
        current_path[0] = '\0';
        current_root[0] = '\0';
    } else {
        char *slash = strrchr(current_path, '/');
        if (slash && slash > current_path) *slash = '\0';

        if (!strcmp(current_path, current_root) ||
            strlen(current_path) < strlen(current_root)) {
            current_path[0] = '\0';
            current_root[0] = '\0';
        }
    }

    selection = 0;
    scroll_offset = 0;
    dirty = 1;
}

static void enter_directory(ScreenContext *c, const char *path)
{
    snprintf(current_path, sizeof(current_path), "%s", path);

    if (!current_root[0]) {
        for (int si = 0; si < *c->storage_path_count; si++) {
            const char *root = c->storage_paths[si].path;
            size_t len = strlen(root);
            if (!strncmp(path, root, len) &&
                (path[len] == '\0' || path[len] == '/')) {
                snprintf(current_root, sizeof(current_root), "%s", root);
                break;
            }
        }
    }

    selection = 0;
    scroll_offset = 0;
    dirty = 1;
}

static void open_book(ScreenContext *c, const char *path)
{
    int bi = find_book_index(c, path);
    if (bi < 0) return;

    *c->book_index = bi;
    *c->track_count = scan_tracks(c->book_paths[bi], c->tracks);
    *c->book_duration = get_track_durations(c->tracks, *c->track_count,
                                             c->track_durations);
    *c->track_index = 0;

    int pi = find_book_progress(c->book_paths[bi]);
    if (pi >= 0 && progress[pi].track >= 0 &&
        progress[pi].track < *c->track_count)
        *c->track_index = progress[pi].track;

    *c->screen = SCREEN_TRACKS;
}

static void activate(ScreenContext *c)
{
    if (!entry_selectable(selection)) return;
    BrowserEntry *e = &entries[selection];

    if (e->type == BROWSER_PARENT) {
        parent_directory();
    } else if (e->type == BROWSER_DIRECTORY) {
        enter_directory(c, e->path);
    } else if (e->type == BROWSER_BOOK ||
               e->type == BROWSER_PLAY_CURRENT) {
        open_book(c, e->path);
    }
}

void menu_handle_event(ScreenContext *c, const SDL_Event *e)
{
    rebuild(c);

    if (e->type == SDL_JOYBUTTONDOWN) {
        int b = e->jbutton.button;
        if (b == BUTTON_B) { *c->screen = SCREEN_PLAYER; return; }
        if (b == BUTTON_DPAD_LEFT) { if (current_path[0]) parent_directory(); else *c->screen = SCREEN_PLAYER; return; }
        if (b == BUTTON_DPAD_RIGHT) { activate(c); return; }
        if (b == BUTTON_DPAD_UP) { move_selection(-1, 1); return; }
        if (b == BUTTON_DPAD_DOWN) { move_selection(1, 1); return; }
        if (b == BUTTON_L1) { move_selection(-1, LIST_PAGE_SIZE); return; }
        if (b == BUTTON_L2) { go_to_start(); return; }
        if (b == BUTTON_R1) { move_selection(1, LIST_PAGE_SIZE); return; }
        if (b == BUTTON_R2) { go_to_end(); return; }
        if (b == BUTTON_A) { activate(c); return; }
    }

    if (e->type == SDL_JOYAXISMOTION && e->jaxis.axis == AXIS_Y) {
        if (!*c->axis_y_lock && e->jaxis.value < -AXIS_DEADZONE) {
            move_selection(-1, 1);
            *c->axis_y_lock = 1;
        } else if (!*c->axis_y_lock && e->jaxis.value > AXIS_DEADZONE) {
            move_selection(1, 1);
            *c->axis_y_lock = 1;
        }
        if (abs(e->jaxis.value) < AXIS_DEADZONE)
            *c->axis_y_lock = 0;
    }
}

static void draw_book_progress(ScreenContext *c, const BrowserEntry *e,
                               int x, int y)
{
    int bi = find_book_index(c, e->path);
    if (bi < 0 || c->book_track_counts[bi] <= 0) return;

    int pi = find_book_progress(e->path);
    if (pi < 0 ||
        (progress[pi].track <= 0 && progress[pi].position <= 0.0))
        return;

    int percent = (progress[pi].track * 100) / c->book_track_counts[bi];
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;

    char text[16];
    snprintf(text, sizeof(text), "%d%%", percent);
    int w = 0, h = 0;
    TTF_SizeUTF8(c->font, e->label, &w, &h);
    draw_text(c->renderer, c->font, text, x + w + 10, y, c->gray);
}

void menu_render(ScreenContext *c)
{
    menu_font_apply(c->font);
    rebuild(c);
    keep_selection_visible();

    int rows = visible_rows();
    int y = MENU_TOP_Y;

    if (current_path[0]) {
        draw_text(c->renderer, c->font, current_path, 20, y, c->gray);
        y += MENU_ROW_H;
    }

    int end = scroll_offset + rows;
    if (end > entry_count) end = entry_count;

    for (int i = scroll_offset; i < end; i++) {
        BrowserEntry *e = &entries[i];

        if (e->type == BROWSER_HEADING) {
            draw_text(c->renderer, c->font, e->label, 20, y, c->gray);
        } else {
            SDL_Color color = (i == selection) ? c->selected : c->white;
            int x = 45;
            draw_text(c->renderer, c->font, e->label, x, y, color);

            if (e->type == BROWSER_BOOK ||
                e->type == BROWSER_PLAY_CURRENT ||
                e->type == BROWSER_DIRECTORY)
                draw_book_progress(c, e, x, y);
        }

        y += MENU_ROW_H;
    }

    if (entry_count > rows) {
        int track_y = current_path[0] ? MENU_TOP_Y + MENU_ROW_H : MENU_TOP_Y;
        int track_h = MENU_BOTTOM_Y - track_y;
        int thumb_h = (track_h * rows) / entry_count;
        if (thumb_h < 18) thumb_h = 18;

        int max_scroll = entry_count - rows;
        if (max_scroll < 1) max_scroll = 1;

        int travel = track_h - thumb_h;
        int thumb_y = track_y + (travel * scroll_offset) / max_scroll;

        SDL_SetRenderDrawColor(c->renderer, 70,70,80,255);
        SDL_Rect rail = {SCREEN_W - 8, track_y, 2, track_h};
        SDL_RenderFillRect(c->renderer, &rail);

        SDL_SetRenderDrawColor(c->renderer, 230,210,70,255);
        SDL_Rect thumb = {SCREEN_W - 8, thumb_y, 2, thumb_h};
        SDL_RenderFillRect(c->renderer, &thumb);
    }

    draw_text(c->renderer,c->font,
              "Rechts/A: Auswaehlen   Links: Zurueck   B: Player   X: Menue",
              20,SCREEN_H-55,c->gray);
    draw_text(c->renderer,c->font,
              "L1: Seite hoch  L2: Anfang  R1: Seite runter  R2: Ende",
              20,SCREEN_H-35,c->gray);
    menu_font_restore(c->font);
}
