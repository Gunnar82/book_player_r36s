#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "types.h"
#include "state.h"
#include "backlight.h"
#include "battery.h"
#include "led.h"
#include "scanner.h"
#include "audio.h"
#include "ui.h"
#include "screens.h"
#include "storage.h"
#include "systemstats.h"
#include "media_keys.h"
#include "screens/menu.h"
#include "screens/tracks.h"
#include "screens/player.h"
#include "screens/sleeptimer.h"
#include "screens/systeminfo.h"
#include "screens/buttondebug.h"
#include "screens/systemmenu.h"
#include "screens/downloadbrowser.h"

static const int LOCK_CANDIDATE_BUTTONS[] = {
    BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y,
    BUTTON_L1, BUTTON_R1, BUTTON_L2, BUTTON_R2
};
static const char *LOCK_CANDIDATE_NAMES[] = {
    "A", "B", "X", "Y", "L1", "R1", "L2", "R2"
};
#define LOCK_CANDIDATE_COUNT \
    (int)(sizeof(LOCK_CANDIDATE_BUTTONS) / sizeof(LOCK_CANDIDATE_BUTTONS[0]))

static const char *button_name(int button)
{
    for (int i = 0; i < LOCK_CANDIDATE_COUNT; i++) {
        if (LOCK_CANDIDATE_BUTTONS[i] == button)
            return LOCK_CANDIDATE_NAMES[i];
    }
    return "?";
}

static void generate_unlock_sequence(int *sequence, int length)
{
    for (int i = 0; i < length; i++) {
        int idx = rand() % LOCK_CANDIDATE_COUNT;
        sequence[i] = LOCK_CANDIDATE_BUTTONS[idx];
    }
}

static void do_shutdown(Mix_Music **music)
{
    save_state();
    if (*music) {
        Mix_HaltMusic();
        Mix_FreeMusic(*music);
        *music = NULL;
    }
    set_display_off(0);
    led_set(0);
    sync();
    system("/usr/bin/busctl call org.freedesktop.login1 /org/freedesktop/login1 org.freedesktop.login1.Manager PowerOff b false");
}

int main(int argc, char **argv)
{
    const char *audio_dir = NULL;
    if (argc > 1 && argv[1][0])
        audio_dir = argv[1];

    srand((unsigned int)time(NULL));
    setup_state_path();
    load_state();
    load_playback_config();
    load_download_config();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_VolumeMusic(volume);
    SDL_Joystick *joy = NULL;
    if (SDL_NumJoysticks() > 0)
        joy = SDL_JoystickOpen(0);

    SDL_Window *window = SDL_CreateWindow(
        "Hoerspiel Player",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_FULLSCREEN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        if (joy) SDL_JoystickClose(joy);
        Mix_CloseAudio(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        if (joy) SDL_JoystickClose(joy);
        Mix_CloseAudio(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont(FONT_PATH, 20);
    if (!font) fprintf(stderr, "Font: %s\n", TTF_GetError());

    init_backlight();
    init_battery();
    int battery_percent = get_battery_percent();
    int battery_charging = is_battery_charging();
    Uint32 last_battery_check = SDL_GetTicks();

    double cpu_usage = get_cpu_usage();
    double ram_usage = get_ram_usage();
    double cpu_temperature = get_cpu_temperature();
    Uint32 last_systemstats_check = SDL_GetTicks();

    SDL_Color white = {255,255,255,255};
    SDL_Color selected = {255,220,80,255};
    SDL_Color gray = {160,160,160,255};

    char book_names[MAX_BOOKS][256];
    char book_paths[MAX_BOOKS][512];
    char book_roots[MAX_BOOKS][512];
    int book_track_counts[MAX_BOOKS];
    memset(book_track_counts, 0, sizeof(book_track_counts));
    int book_count = 0;

    StoragePath storage_paths[MAX_STORAGE_PATHS];
    int storage_path_count = get_storage_paths(storage_paths, MAX_STORAGE_PATHS);

    for (int si = 0; si < storage_path_count && book_count < MAX_BOOKS; si++) {
        if (!storage_paths[si].available) continue;
        char names[MAX_BOOKS][256];
        char paths[MAX_BOOKS][512];
        int count = scan_books_recursive(storage_paths[si].path, names, paths);
        for (int i = 0; i < count && book_count < MAX_BOOKS; i++) {
            snprintf(book_names[book_count], sizeof(book_names[book_count]), "%s", names[i]);
            snprintf(book_paths[book_count], sizeof(book_paths[book_count]), "%s", paths[i]);
            snprintf(book_roots[book_count], sizeof(book_roots[book_count]), "%s", storage_paths[si].path);
            book_count++;
        }
    }

    for (int i = 0; i < book_count; i++) {
        Track tmp_tracks[MAX_TRACKS];
        book_track_counts[i] = scan_tracks(book_paths[i], tmp_tracks);
    }

    Track tracks[MAX_TRACKS];
    int track_count = 0;
    ScreenId screen = SCREEN_PLAYER;
    int book_index = 0;
    int track_index = 0;
    Mix_Music *music = NULL;
    double base_position = 0.0;
    Uint32 started_ticks = 0;
    Uint32 last_save = SDL_GetTicks();
    int paused = 0;
    double duration = 0.0;
    double track_durations[MAX_TRACKS] = {0};
    double book_duration = 0.0;
    int running = 1;
    int axis_y_lock = 0;
    int axis_x_lock = 0;
    Uint32 last_activity = SDL_GetTicks();

    int idle_setting_seen = idle_timer_minutes;
    Uint32 idle_timer_remaining_ms = idle_timer_minutes > 0 ? (Uint32)idle_timer_minutes * 60000U : 0U;
    Uint32 idle_timer_last_tick = SDL_GetTicks();

    int resume_book = -1;
    int resume_progress = -1;
    long long newest_played = -1;
    for (int i = 0; i < book_count; i++) {
        int pi = find_book_progress(book_paths[i]);
        if (pi < 0) continue;
        if (progress[pi].last_played > newest_played) {
            newest_played = progress[pi].last_played;
            resume_book = i;
            resume_progress = pi;
        }
    }

    if (resume_book >= 0 && resume_progress >= 0) {
        track_count = scan_tracks(book_paths[resume_book], tracks);
        book_duration = get_track_durations(tracks, track_count, track_durations);
        if (track_count > 0) {
            book_index = resume_book;
            track_index = progress[resume_progress].track;
            if (track_index < 0 || track_index >= track_count) track_index = 0;
            base_position = progress[resume_progress].position;
            started_ticks = SDL_GetTicks();
            paused = 1;
            last_save = SDL_GetTicks();
        }
    }

    int sleep_timer_active = 0;
    int sleep_timer_minutes = SLEEP_DEFAULT_MINUTES;
    Uint32 sleep_timer_end_ticks = 0;
    int shutdown_tracks_remaining = shutdown_after_tracks;
    int shutdown_tracks_setting_seen = shutdown_after_tracks;

    int locked = 0;
    int unlock_sequence[UNLOCK_SEQUENCE_LEN];
    int unlock_progress = 0;
    Uint32 unlock_wrong_flash_until = 0;

    MediaKeys media_keys;
    media_keys_init(&media_keys);

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = 0; continue; }

            if (e.type == SDL_JOYBUTTONDOWN || e.type == SDL_KEYDOWN ||
                (e.type == SDL_JOYAXISMOTION && abs(e.jaxis.value) > AXIS_DEADZONE)) {
                last_activity = SDL_GetTicks();
                if (e.type == SDL_JOYBUTTONDOWN || e.type == SDL_KEYDOWN) {
                    idle_timer_remaining_ms = idle_timer_minutes > 0 ? (Uint32)idle_timer_minutes * 60000U : 0U;
                    idle_timer_last_tick = SDL_GetTicks();
                }
                if (is_display_off()) { set_display_off(0); continue; }
            }

            if (screen == SCREEN_BUTTON_DEBUG) {
                if (e.type == SDL_JOYBUTTONDOWN && e.jbutton.button == BUTTON_A) {
                    ScreenContext debug_ctx = {0};
                    debug_ctx.screen = &screen;
                    buttondebug_handle_event(&debug_ctx, &e);
                }
                continue;
            }

            if (e.type == SDL_JOYBUTTONDOWN && e.jbutton.button == BUTTON_SELECT) {
                if (!locked) {
                    locked = 1;
                    generate_unlock_sequence(unlock_sequence, UNLOCK_SEQUENCE_LEN);
                    unlock_progress = 0;
                }
                continue;
            }

            if (locked) {
                if (e.type == SDL_JOYBUTTONDOWN) {
                    if (e.jbutton.button == unlock_sequence[unlock_progress]) {
                        unlock_progress++;
                        if (unlock_progress >= UNLOCK_SEQUENCE_LEN) { locked = 0; unlock_progress = 0; }
                    } else {
                        unlock_progress = 0;
                        unlock_wrong_flash_until = SDL_GetTicks() + 400;
                    }
                }
                continue;
            }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.scancode == (SDL_Scancode)KEY_VOLUME_UP) {
                    volume += VOLUME_STEP; if (volume > MIX_MAX_VOLUME) volume = MIX_MAX_VOLUME;
                    Mix_VolumeMusic(volume); save_state(); continue;
                }
                if (e.key.keysym.scancode == (SDL_Scancode)KEY_VOLUME_DOWN) {
                    volume -= VOLUME_STEP; if (volume < 0) volume = 0;
                    Mix_VolumeMusic(volume); save_state(); continue;
                }
            }

            if (e.type == SDL_JOYBUTTONDOWN) {
                if (e.jbutton.button == BUTTON_X) { screen = SCREEN_SYSTEM_MENU; continue; }
                if (e.jbutton.button == BUTTON_START) {
                    if (!music && track_count > 0) {
                        music = play_track(tracks, track_index, base_position, &base_position, &started_ticks, &paused);
                        if (music) { duration = get_duration(music); last_save = SDL_GetTicks(); }
                    } else if (music) {
                        if (paused) { Mix_ResumeMusic(); started_ticks = SDL_GetTicks(); paused = 0; }
                        else if (Mix_PlayingMusic()) {
                            base_position = get_position(base_position, started_ticks, 0);
                            Mix_PauseMusic(); paused = 1;
                        }
                    }
                    continue;
                }
                if (e.jbutton.button == BUTTON_Y) {
                    screen = (screen == SCREEN_PLAYER) ? SCREEN_MENU : SCREEN_PLAYER;
                    continue;
                }
            }

            ScreenContext screen_ctx = {
                renderer, font, white, selected, gray,
                &screen, &running, &book_index, &track_index,
                &book_count, &track_count, book_names, book_paths, tracks,
                &music, &base_position, &started_ticks, &duration,
                track_durations, &book_duration, &paused,
                &last_save, &axis_y_lock, &axis_x_lock,
                &sleep_timer_active, &sleep_timer_minutes, &sleep_timer_end_ticks,
                storage_paths, &storage_path_count, book_roots,
                book_track_counts, &cpu_usage, &ram_usage, &cpu_temperature,
                &battery_percent, &battery_charging,
                &idle_timer_remaining_ms, do_shutdown
            };

            switch (screen) {
                case SCREEN_MENU: menu_handle_event(&screen_ctx, &e); break;
                case SCREEN_TRACKS: tracks_handle_event(&screen_ctx, &e); break;
                case SCREEN_PLAYER: player_handle_event(&screen_ctx, &e); break;
                case SCREEN_SLEEP_TIMER: sleeptimer_handle_event(&screen_ctx, &e); break;
                case SCREEN_SYSTEM_INFO: systeminfo_handle_event(&screen_ctx, &e); break;
                case SCREEN_BUTTON_DEBUG: buttondebug_handle_event(&screen_ctx, &e); break;
                case SCREEN_SYSTEM_MENU: systemmenu_handle_event(&screen_ctx, &e); break;
                case SCREEN_DOWNLOADS: downloadbrowser_handle_event(&screen_ctx, &e); break;
            }
        }

        MediaKeyAction media_actions[16];
        int media_action_count = media_keys_poll(&media_keys, media_actions, 16);
        if (media_action_count > 0) {
            idle_timer_remaining_ms = idle_timer_minutes > 0 ? (Uint32)idle_timer_minutes * 60000U : 0U;
            idle_timer_last_tick = SDL_GetTicks();
        }

        if (!locked && screen != SCREEN_BUTTON_DEBUG) {
            for (int mi = 0; mi < media_action_count; mi++) {
                MediaKeyAction action = media_actions[mi];
                if (action == MEDIA_KEY_PREVIOUS || action == MEDIA_KEY_NEXT) {
                    if (track_count <= 0) continue;
                    if (music) {
                        int pi = ensure_book_progress(book_paths[book_index]);
                        if (pi >= 0) {
                            progress[pi].track = track_index;
                            progress[pi].position = get_position(base_position, started_ticks, paused);
                            touch_book_progress(pi);
                        }
                        Mix_HaltMusic(); Mix_FreeMusic(music); music = NULL;
                    }
                    if (action == MEDIA_KEY_PREVIOUS) { track_index--; if (track_index < 0) track_index = track_count - 1; }
                    else { track_index++; if (track_index >= track_count) track_index = 0; }
                    base_position = 0.0;
                    music = play_track(tracks, track_index, 0.0, &base_position, &started_ticks, &paused);
                    if (music) {
                        duration = get_duration(music); last_save = SDL_GetTicks();
                        int pi = ensure_book_progress(book_paths[book_index]);
                        if (pi >= 0) { progress[pi].track = track_index; progress[pi].position = 0.0; touch_book_progress(pi); }
                        save_state();
                    }
                    continue;
                }
                if (action == MEDIA_KEY_PLAY_PAUSE) {
                    if (!music && track_count > 0) {
                        music = play_track(tracks, track_index, base_position, &base_position, &started_ticks, &paused);
                        if (music) { duration = get_duration(music); last_save = SDL_GetTicks(); }
                    } else if (music) {
                        if (paused) { Mix_ResumeMusic(); started_ticks = SDL_GetTicks(); paused = 0; }
                        else if (Mix_PlayingMusic()) { base_position = get_position(base_position, started_ticks, 0); Mix_PauseMusic(); paused = 1; }
                    }
                    continue;
                }
                if (action == MEDIA_KEY_PLAY) {
                    if (!music && track_count > 0) {
                        music = play_track(tracks, track_index, base_position, &base_position, &started_ticks, &paused);
                        if (music) { duration = get_duration(music); last_save = SDL_GetTicks(); }
                    } else if (music && paused) {
                        Mix_ResumeMusic(); started_ticks = SDL_GetTicks(); paused = 0;
                    }
                    continue;
                }
                if (action == MEDIA_KEY_PAUSE) {
                    if (music && !paused && Mix_PlayingMusic()) {
                        base_position = get_position(base_position, started_ticks, 0);
                        Mix_PauseMusic(); paused = 1;
                    }
                    continue;
                }
                if(action == MEDIA_KEY_STOP) {
                    if(music) {
                        base_position = get_position(base_position, started_ticks, paused);
                        int pi = ensure_book_progress(book_paths[book_index]);
                        if(pi >= 0) {
                            progress[pi].track = track_index;
                            progress[pi].position = base_position;
                            touch_book_progress(pi);
                        }
                        save_state();
                        Mix_HaltMusic();
                        Mix_FreeMusic(music);
                        music = NULL;
                        paused = 1;
                        started_ticks = SDL_GetTicks();
                    }
                    continue;
                }
            }
        }

        if (music && !paused && !Mix_PlayingMusic()) {
            int pi = ensure_book_progress(book_paths[book_index]);
            int was_last_track = (track_index >= track_count - 1);

            if (pi >= 0) {
                progress[pi].track = track_index;
                progress[pi].position = 0.0;
                touch_book_progress(pi);
            }

            if (shutdown_after_tracks != shutdown_tracks_setting_seen) {
                shutdown_tracks_setting_seen = shutdown_after_tracks;
                shutdown_tracks_remaining = shutdown_after_tracks;
            }
            if (shutdown_tracks_remaining > 0) {
                shutdown_tracks_remaining--;
                if (shutdown_tracks_remaining == 0) {
                    Mix_FreeMusic(music);
                    music = NULL;
                    do_shutdown(&music);
                    running = 0;
                    continue;
                }
            }

            if (was_last_track) {
                if (shutdown_at_book_end) {
                    Mix_FreeMusic(music);
                    music = NULL;
                    do_shutdown(&music);
                    running = 0;
                    continue;
                }

                if (!repeat_book) {
                    Mix_FreeMusic(music);
                    music = NULL;
                    base_position = 0.0;
                    paused = 1;
                    if (pi >= 0) {
                        progress[pi].track = track_count - 1;
                        progress[pi].position = track_durations[track_count - 1];
                        touch_book_progress(pi);
                        save_state();
                    }
                    continue;
                }

                track_index = 0;
            } else {
                track_index++;
            }

            Mix_FreeMusic(music);
            music = play_track(tracks, track_index, 0.0,
                                &base_position, &started_ticks, &paused);
            if (music) duration = get_duration(music);
        }

        if (music && SDL_GetTicks() - last_save >= SAVE_INTERVAL_MS) {
            int pi = ensure_book_progress(book_paths[book_index]);
            if (pi >= 0) {
                progress[pi].track = track_index;
                progress[pi].position = get_position(base_position, started_ticks, paused);
                touch_book_progress(pi);
            }
            save_state();
            last_save = SDL_GetTicks();
        }

        if (idle_timer_minutes != idle_setting_seen) {
            idle_setting_seen = idle_timer_minutes;
            idle_timer_remaining_ms = idle_timer_minutes > 0 ? (Uint32)idle_timer_minutes * 60000U : 0U;
            idle_timer_last_tick = SDL_GetTicks();
        }

        {
            Uint32 now = SDL_GetTicks();
            if (idle_timer_minutes <= 0) {
                idle_timer_remaining_ms = 0;
                idle_timer_last_tick = now;
            } else if (music && !paused && Mix_PlayingMusic()) {
                idle_timer_last_tick = now;
            } else {
                Uint32 elapsed = now - idle_timer_last_tick;
                idle_timer_last_tick = now;
                if (elapsed >= idle_timer_remaining_ms) {
                    idle_timer_remaining_ms = 0;
                    do_shutdown(&music);
                    running = 0;
                } else {
                    idle_timer_remaining_ms -= elapsed;
                }
            }
        }

        if (sleep_timer_active && SDL_GetTicks() >= sleep_timer_end_ticks) {
            do_shutdown(&music);
            running = 0;
        }

        if (sleep_timer_active) {
            Uint32 now = SDL_GetTicks();
            Uint32 rem = sleep_timer_end_ticks > now ? sleep_timer_end_ticks - now : 0;
            if (rem <= LED_BLINK_THRESHOLD_SEC * 1000U) {
                int blink_on = ((now / (LED_BLINK_PERIOD_MS / 2)) % 2) == 0;
                led_set(blink_on);
            } else {
                led_set(0);
            }
        } else {
            led_set(0);
        }

        if (SDL_GetTicks() - last_battery_check >= 5000) {
            battery_percent = get_battery_percent();
            battery_charging = is_battery_charging();
            last_battery_check = SDL_GetTicks();
        }
        if (SDL_GetTicks() - last_systemstats_check >= 2000) {
            cpu_usage = get_cpu_usage();
            ram_usage = get_ram_usage();
            cpu_temperature = get_cpu_temperature();
            last_systemstats_check = SDL_GetTicks();
        }

        SDL_SetRenderDrawColor(renderer, 15,15,20,255);
        SDL_RenderClear(renderer);

        ScreenContext screen_ctx = {
            renderer, font, white, selected, gray,
            &screen, &running, &book_index, &track_index,
            &book_count, &track_count, book_names, book_paths, tracks,
            &music, &base_position, &started_ticks, &duration,
            track_durations, &book_duration, &paused,
            &last_save, &axis_y_lock, &axis_x_lock,
            &sleep_timer_active, &sleep_timer_minutes, &sleep_timer_end_ticks,
            storage_paths, &storage_path_count, book_roots,
            book_track_counts, &cpu_usage, &ram_usage, &cpu_temperature,
            &battery_percent, &battery_charging,
            &idle_timer_remaining_ms, do_shutdown
        };

        if (locked) {
            draw_text(renderer,font,"Tastensperre",20,40,selected);
            draw_text(renderer,font,"Zum Entsperren:",20,100,white);
            int x=20;
            for(int i=0;i<UNLOCK_SEQUENCE_LEN;i++) {
                const char *name=button_name(unlock_sequence[i]);
                SDL_Color col=i<unlock_progress?selected:white;
                draw_text(renderer,font,name,x,150,col);
                x+=70;
            }
            if(SDL_GetTicks()<unlock_wrong_flash_until)
                draw_text(renderer,font,"Falsche Taste",20,210,gray);
        } else {
            switch(screen) {
                case SCREEN_MENU: menu_render(&screen_ctx); break;
                case SCREEN_TRACKS: tracks_render(&screen_ctx); break;
                case SCREEN_PLAYER: player_render(&screen_ctx); break;
                case SCREEN_SLEEP_TIMER: sleeptimer_render(&screen_ctx); break;
                case SCREEN_SYSTEM_INFO: systeminfo_render(&screen_ctx); break;
                case SCREEN_BUTTON_DEBUG: buttondebug_render(&screen_ctx); break;
                case SCREEN_SYSTEM_MENU: systemmenu_render(&screen_ctx); break;
                case SCREEN_DOWNLOADS: downloadbrowser_render(&screen_ctx); break;
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    save_state();
    media_keys_close(&media_keys);
    if (music) { Mix_HaltMusic(); Mix_FreeMusic(music); }
    if (joy) SDL_JoystickClose(joy);
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
    return 0;
}