#include "tracks.h"
#include "menu.h"
#include "../scanner.h"
#include "../state.h"
#include "../audio.h"
#include "../ui.h"
#include "../bluetooth.h"
#include "../local_delete.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int delete_stage = 0;
static int action_selection = 0;
static int l1_down = 0;
static int r1_down = 0;
static char delete_message[256] = "";

static void stop_playback(ScreenContext *c)
{
    if (*c->music) {
        Mix_HaltMusic();
        Mix_FreeMusic(*c->music);
        *c->music = NULL;
    }
    *c->base_position = 0.0;
    *c->duration = 0.0;
    *c->paused = 0;
}

static void refresh_tracks(ScreenContext *c)
{
    if (*c->book_index < 0 || *c->book_index >= *c->book_count) {
        *c->track_count = 0;
        *c->track_index = 0;
        return;
    }

    *c->track_count = scan_tracks(c->book_paths[*c->book_index], c->tracks);
    if (*c->track_count <= 0) {
        *c->track_index = 0;
        *c->book_duration = 0.0;
        c->book_track_counts[*c->book_index] = 0;
        return;
    }

    if (*c->track_index >= *c->track_count) {
        *c->track_index = *c->track_count - 1;
    }
    if (*c->track_index < 0) {
        *c->track_index = 0;
    }
    *c->book_duration = get_track_durations(c->tracks, *c->track_count, c->track_durations);
    c->book_track_counts[*c->book_index] = *c->track_count;
}

static void remove_book_from_library(ScreenContext *c, int index)
{
    if (index < 0 || index >= *c->book_count) {
        return;
    }

    for (int i = index; i + 1 < *c->book_count; i++) {
        memmove(c->book_names[i], c->book_names[i + 1], sizeof(c->book_names[i]));
        memmove(c->book_paths[i], c->book_paths[i + 1], sizeof(c->book_paths[i]));
        memmove(c->book_roots[i], c->book_roots[i + 1], sizeof(c->book_roots[i]));
        c->book_track_counts[i] = c->book_track_counts[i + 1];
    }

    (*c->book_count)--;
    if (*c->book_count < 0) {
        *c->book_count = 0;
    }
    if (*c->book_count == 0) {
        *c->book_index = 0;
    } else if (*c->book_index >= *c->book_count) {
        *c->book_index = *c->book_count - 1;
    }
    *c->track_count = 0;
    *c->track_index = 0;
    *c->book_duration = 0.0;
}

static void open_delete_action(ScreenContext *c)
{
    if (*c->track_count <= 0 || *c->track_index < 0 || *c->track_index >= *c->track_count) {
        return;
    }
    action_selection = 0;
    delete_stage = 1;
    delete_message[0] = '\0';
}

static void confirm_delete(ScreenContext *c)
{
    if (*c->book_index < 0 || *c->book_index >= *c->book_count) {
        delete_stage = 0;
        return;
    }

    stop_playback(c);

    if (action_selection == 0) {
        if (*c->track_count <= 0 || *c->track_index < 0 || *c->track_index >= *c->track_count) {
            delete_stage = 0;
            return;
        }
        char path[512];
        snprintf(path, sizeof(path), "%s", c->tracks[*c->track_index].path);
        if (local_delete_file(path, c->storage_paths, *c->storage_path_count) == 0) {
            snprintf(delete_message, sizeof(delete_message), "Datei geloescht");
            refresh_tracks(c);
            menu_invalidate();
        } else {
            snprintf(delete_message, sizeof(delete_message), "Loeschen fehlgeschlagen");
        }
        delete_stage = 3;
        return;
    }

    int deleted_index = *c->book_index;
    char path[512];
    snprintf(path, sizeof(path), "%s", c->book_paths[deleted_index]);
    if (local_delete_directory(path, c->storage_paths, *c->storage_path_count) == 0) {
        remove_book_from_library(c, deleted_index);
        menu_invalidate();
        delete_stage = 0;
        *c->screen = SCREEN_MENU;
    } else {
        snprintf(delete_message, sizeof(delete_message), "Ordner konnte nicht geloescht werden");
        delete_stage = 3;
    }
}

void tracks_handle_event(ScreenContext *c, const SDL_Event *e)
{
    if (*c->track_count > 0) {
        if (*c->track_index < 0) {
            *c->track_index = *c->track_count - 1;
        }
        if (*c->track_index >= *c->track_count) {
            *c->track_index = 0;
        }
    }

    if (e->type == SDL_JOYBUTTONUP) {
        if (e->jbutton.button == BUTTON_L1) {
            l1_down = 0;
        }
        if (e->jbutton.button == BUTTON_R1) {
            r1_down = 0;
        }
    }

    if (e->type == SDL_JOYBUTTONDOWN) {
        int b = e->jbutton.button;

        if (delete_stage != 0) {
            if (b == BUTTON_B || b == BUTTON_DPAD_LEFT) {
                delete_stage = 0;
                return;
            }
            if (delete_stage == 1 && (b == BUTTON_DPAD_UP || b == BUTTON_DPAD_DOWN)) {
                action_selection = 1 - action_selection;
                return;
            }
            if (b == BUTTON_A || b == BUTTON_DPAD_RIGHT) {
                if (delete_stage == 1) {
                    delete_stage = 2;
                } else if (delete_stage == 2) {
                    confirm_delete(c);
                } else {
                    delete_stage = 0;
                }
                return;
            }
            return;
        }

        if (b == BUTTON_L1) {
            l1_down = 1;
            if (r1_down) {
                open_delete_action(c);
                return;
            }
        }
        if (b == BUTTON_R1) {
            r1_down = 1;
            if (l1_down) {
                open_delete_action(c);
                return;
            }
        }

        if (b == BUTTON_B || b == BUTTON_DPAD_LEFT) {
            *c->screen = SCREEN_MENU;
            return;
        }
        if (b == BUTTON_DPAD_RIGHT && *c->track_count > 0) {
            b = BUTTON_A;
        }
        if (b == BUTTON_DPAD_UP) {
            (*c->track_index)--;
            return;
        }
        if (b == BUTTON_DPAD_DOWN) {
            (*c->track_index)++;
            return;
        }
        if (b == BUTTON_L1) {
            *c->track_index -= LIST_PAGE_SIZE;
            if (*c->track_index < 0) {
                *c->track_index = 0;
            }
            return;
        }
        if (b == BUTTON_L2) {
            *c->track_index = 0;
            return;
        }
        if (b == BUTTON_R1) {
            *c->track_index += LIST_PAGE_SIZE;
            if (*c->track_index >= *c->track_count) {
                *c->track_index = *c->track_count - 1;
            }
            return;
        }
        if (b == BUTTON_R2) {
            *c->track_index = *c->track_count - 1;
            return;
        }
        if (b == BUTTON_A && *c->track_count > 0) {
            int pi = ensure_book_progress(c->book_paths[*c->book_index]);
            double resume = 0;
            if (pi >= 0 && progress[pi].track == *c->track_index) {
                resume = progress[pi].position;
            }
            if (*c->music) {
                Mix_HaltMusic();
                Mix_FreeMusic(*c->music);
                *c->music = NULL;
            }
            *c->music = play_track(c->tracks, *c->track_index, resume, c->base_position, c->started_ticks, c->paused);
            if (*c->music) {
                touch_book_progress(pi);
                save_state();
                *c->duration = get_duration(*c->music);
                *c->last_save = SDL_GetTicks();
                *c->screen = SCREEN_PLAYER;
            }
            return;
        }
    }

    if (delete_stage == 0 && e->type == SDL_JOYAXISMOTION && e->jaxis.axis == AXIS_Y) {
        if (!*c->axis_y_lock && e->jaxis.value < -AXIS_DEADZONE) {
            (*c->track_index)--;
            *c->axis_y_lock = 1;
        } else if (!*c->axis_y_lock && e->jaxis.value > AXIS_DEADZONE) {
            (*c->track_index)++;
            *c->axis_y_lock = 1;
        }
        if (abs(e->jaxis.value) < AXIS_DEADZONE) {
            *c->axis_y_lock = 0;
        }
    }
}

static void render_delete_overlay(ScreenContext *c)
{
    if (delete_stage == 0) {
        return;
    }

    SDL_Rect box = {45, 130, SCREEN_W - 90, 195};
    SDL_SetRenderDrawColor(c->renderer, 25, 25, 30, 245);
    SDL_RenderFillRect(c->renderer, &box);
    SDL_SetRenderDrawColor(c->renderer, 180, 180, 190, 255);
    SDL_RenderDrawRect(c->renderer, &box);

    if (delete_stage == 1) {
        draw_text(c->renderer, c->font, "Aktion", 70, 150, c->gray);
        draw_text(c->renderer, c->font, "Datei loeschen", 85, 190, action_selection == 0 ? c->selected : c->white);
        draw_text(c->renderer, c->font, "Hoerspielordner loeschen", 85, 225, action_selection == 1 ? c->selected : c->white);
        draw_text(c->renderer, c->font, "Hoch/Runter   A: Auswaehlen   B: Abbrechen", 70, 280, c->gray);
    } else if (delete_stage == 2) {
        const char *question = action_selection == 0 ? "Datei wirklich loeschen?" : "Ordner samt Inhalt wirklich loeschen?";
        draw_text(c->renderer, c->font, question, 70, 175, c->selected);
        if (action_selection == 1) {
            draw_text(c->renderer, c->font, "Alle Dateien im Hoerspielordner werden entfernt.", 70, 215, c->white);
        }
        draw_text(c->renderer, c->font, "A: Ja, loeschen   B: Nein", 70, 270, c->gray);
    } else {
        draw_text(c->renderer, c->font, delete_message, 70, 190, c->selected);
        draw_text(c->renderer, c->font, "A/B: Schliessen", 70, 255, c->gray);
    }
}

void tracks_render(ScreenContext *c)
{
    menu_font_apply(c->font);
    char book_label[320];
    unsigned int dial_id = ensure_book_dial_id(c->book_paths[*c->book_index]);
    if (bluetooth_adapter_present() && dial_id >= 1001) {
        snprintf(book_label, sizeof(book_label), "%s [%u]", c->book_names[*c->book_index], dial_id);
    } else {
        snprintf(book_label, sizeof(book_label), "%s", c->book_names[*c->book_index]);
    }
    draw_text(c->renderer, c->font, book_label, 20, 20, c->selected);
    const int top = 60;
    const int bottom = SCREEN_H - 60;
    const int row = menu_line_height();
    const int visible = (bottom - top) / row;
    int start = 0;
    if (*c->track_count > visible && *c->track_index >= visible) {
        start = *c->track_index - visible + 1;
    }
    int n = *c->track_count - start;
    if (n > visible) {
        n = visible;
    }
    int y = top;
    for (int i = 0; i < n; i++) {
        int t = start + i;
        draw_text(c->renderer, c->font, c->tracks[t].name, 40, y, t == *c->track_index ? c->selected : c->white);
        y += row;
    }
    if (*c->track_count > visible) {
        int h = bottom - top;
        int thumb = (h * visible) / (*c->track_count);
        if (thumb < 12) {
            thumb = 12;
        }
        int range = *c->track_count - visible;
        int travel = h - thumb;
        int ty = top;
        if (range > 0) {
            ty += (travel * start) / range;
        }
        SDL_Rect r = {SCREEN_W - 8, ty, 2, thumb};
        SDL_SetRenderDrawColor(c->renderer, 230, 210, 70, 255);
        SDL_RenderFillRect(c->renderer, &r);
    }
    draw_text(c->renderer, c->font, "Rechts/A: Start   Links/B: Zurueck   X: System   Y: Hoerspiele", 20, SCREEN_H - 55, c->gray);
    draw_text(c->renderer, c->font, "L1+R1: Aktion   L2: Anfang   R2: Ende", 20, SCREEN_H - 35, c->gray);
    render_delete_overlay(c);
    menu_font_restore(c->font);
}
