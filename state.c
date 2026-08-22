#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "state.h"
#include "config.h"

#ifndef MIX_MAX_VOLUME
#define MIX_MAX_VOLUME 128
#endif

BookProgress progress[MAX_BOOKS];
int progress_count = 0;

int volume = MIX_MAX_VOLUME;
int idle_timer_minutes = 0;
int display_timeout_seconds = 60;
int menu_font_size = 1;
unsigned long long usage_app_starts = 0;
unsigned long long usage_runtime_seconds = 0;
unsigned long long usage_playback_seconds = 0;

static char progress_file[512];

void setup_state_path(void)
{
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    snprintf(progress_file, sizeof(progress_file), "%s/.hoerspiel_player_state", home);
}

void load_state(void)
{
    progress_count = 0;
    FILE *fp = fopen(progress_file, "r");
    if (!fp) return;
    char line[800];
    while (fgets(line, sizeof(line), fp) && progress_count < MAX_BOOKS) {
        if (!strncmp(line, "@usage\t", 7)) {
            unsigned long long starts=0, runtime=0, playback=0;
            if (sscanf(line + 7, "%llu\t%llu\t%llu", &starts, &runtime, &playback) >= 1) {
                usage_app_starts = starts;
                usage_runtime_seconds = runtime;
                usage_playback_seconds = playback;
            }
            continue;
        }
        if (!strncmp(line, "@settings\t", 10)) {
            int saved_volume = volume;
            int saved_idle = idle_timer_minutes;
            int saved_display_timeout = display_timeout_seconds;
            int saved_menu_font_size = menu_font_size;
            int fields = sscanf(line + 10, "%d\t%d\t%d\t%d", &saved_volume, &saved_idle, &saved_display_timeout, &saved_menu_font_size);
            if (fields >= 1) {
                volume = saved_volume;
                if (volume < 0) volume = 0;
                if (volume > MIX_MAX_VOLUME) volume = MIX_MAX_VOLUME;
            }
            if (fields >= 2) {
                idle_timer_minutes = saved_idle;
                if (idle_timer_minutes < 0) idle_timer_minutes = 0;
                if (idle_timer_minutes > IDLE_TIMER_MAX_MINUTES) idle_timer_minutes = IDLE_TIMER_MAX_MINUTES;
            }
            if (fields >= 3) {
                display_timeout_seconds = saved_display_timeout;
                if (display_timeout_seconds < 0) display_timeout_seconds = 0;
                if (display_timeout_seconds > 3600) display_timeout_seconds = 3600;
            }
            if (fields >= 4) {
                menu_font_size = saved_menu_font_size;
                if (menu_font_size < 0) menu_font_size = 0;
                if (menu_font_size > 3) menu_font_size = 3;
            }
            continue;
        }
        char *a = strchr(line, '\t'); if (!a) continue; *a++ = '\0';
        char *b = strchr(a, '\t'); if (!b) continue; *b++ = '\0';
        char *c = strchr(b, '\t'); if (!c) continue; *c++ = '\0';
        char *d = strchr(c, '\t'); if (d) *d++ = '\0';
        BookProgress *p = &progress[progress_count++];
        memset(p, 0, sizeof(*p));
        snprintf(p->path, sizeof(p->path), "%s", line);
        p->track = atoi(a);
        p->position = atof(b);
        p->last_played = d ? atoll(d) : 0;
        if (d) { char *e = strchr(d, '\t'); if (e) p->dial_id = (unsigned int)strtoul(e + 1, NULL, 10); }
        volume = atoi(c);
        if (volume < 0) volume = 0;
        if (volume > MIX_MAX_VOLUME) volume = MIX_MAX_VOLUME;
    }
    fclose(fp);
}

void save_state(void)
{
    FILE *fp = fopen(progress_file, "w");
    if (!fp) return;
    fprintf(fp, "@settings\t%d\t%d\t%d\t%d\n", volume, idle_timer_minutes, display_timeout_seconds, menu_font_size);
    fprintf(fp, "@usage\t%llu\t%llu\t%llu\n", usage_app_starts, usage_runtime_seconds, usage_playback_seconds);
    for (int i = 0; i < progress_count; i++) {
        fprintf(fp, "%s\t%d\t%.3f\t%d\t%lld\t%u\n", progress[i].path, progress[i].track,
                progress[i].position, volume, progress[i].last_played, progress[i].dial_id);
    }
    fclose(fp);
}

int find_book_progress(const char *book_path)
{
    for (int i = 0; i < progress_count; i++) if (!strcmp(progress[i].path, book_path)) return i;
    return -1;
}

int ensure_book_progress(const char *book_path)
{
    int i = find_book_progress(book_path);
    if (i >= 0) return i;
    if (progress_count >= MAX_BOOKS) return -1;
    i = progress_count++;
    memset(&progress[i], 0, sizeof(progress[i]));
    snprintf(progress[i].path, sizeof(progress[i].path), "%s", book_path);
    return i;
}

void touch_book_progress(int index)
{
    if (index < 0 || index >= progress_count) return;
    progress[index].last_played = (long long)time(NULL);
}

static int dial_id_in_use(unsigned int id)
{
    for (int i = 0; i < progress_count; i++) if (progress[i].dial_id == id) return 1;
    return 0;
}

unsigned int ensure_book_dial_id(const char *book_path)
{
    int i = ensure_book_progress(book_path);
    if (i < 0) return 0;
    if (progress[i].dial_id >= 1001) return progress[i].dial_id;
    unsigned int id = 1001;
    while (dial_id_in_use(id) && id < 999999999U) id++;
    progress[i].dial_id = id;
    return id;
}

int find_book_progress_by_dial_id(unsigned int dial_id)
{
    if (dial_id < 1001) return -1;
    for (int i = 0; i < progress_count; i++) if (progress[i].dial_id == dial_id) return i;
    return -1;
}
