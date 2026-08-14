#ifndef SCREEN_CONTEXT_H
#define SCREEN_CONTEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "../config.h"
#include "../types.h"
#include "../storage.h"
#include "../screens.h"

typedef struct ScreenContext {
    SDL_Renderer *renderer;
    TTF_Font *font;
    SDL_Color white, selected, gray;
    ScreenId *screen;
    int *running;
    int *book_index;
    int *track_index;
    int *book_count;
    int *track_count;
    char (*book_names)[256];
    char (*book_paths)[512];
    Track *tracks;
    Mix_Music **music;
    double *base_position;
    Uint32 *started_ticks;
    double *duration;
    double *track_durations;
    double *book_duration;
    int *paused;
    Uint32 *last_save;
    int *axis_y_lock;
    int *axis_x_lock;
    int *sleep_timer_active;
    int *sleep_timer_minutes;
    Uint32 *sleep_timer_end_ticks;
    StoragePath *storage_paths;
    int *storage_path_count;
    char (*book_roots)[512];
    int *book_track_counts;
    double *cpu_usage;
    double *ram_usage;
    double *cpu_temperature;
    int *battery_percent;
    int *battery_charging;
    Uint32 *idle_timer_remaining_ms;
    void (*shutdown_fn)(Mix_Music **music);
} ScreenContext;

#endif
