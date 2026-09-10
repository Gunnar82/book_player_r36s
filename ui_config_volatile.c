#include "storage.h"
#include <SDL2/SDL_mixer.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MIX_MAX_VOLUME
#define MIX_MAX_VOLUME 128
#endif

static void trim(char *s)
{
    if (!s) return;
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

int __wrap_load_ui_config(int *out_volume,int *out_idle,int *out_display,int *out_font)
{
    if (out_volume) *out_volume = MIX_MAX_VOLUME;

    FILE *fp = fopen(get_storage_config_path(), "r");
    if (!fp) return 0;

    char line[1200];
    int in_ui = 0, found = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (!line[0] || line[0] == '#' || line[0] == ';') continue;
        if (line[0] == '[') {
            in_ui = !strcmp(line, "[ui]");
            continue;
        }
        if (!in_ui) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq++ = '\0';
        trim(line);
        trim(eq);

        if (!strcmp(line, "idle_timer_minutes")) {
            if (out_idle) *out_idle = atoi(eq);
            found = 1;
        } else if (!strcmp(line, "display_timeout_seconds")) {
            if (out_display) *out_display = atoi(eq);
            found = 1;
        } else if (!strcmp(line, "menu_font_size")) {
            if (out_font) *out_font = atoi(eq);
            found = 1;
        }
    }

    fclose(fp);
    return found;
}

int __wrap_save_ui_config(int volume,int idle,int display,int font)
{
    (void)volume;

    const char *path = get_storage_config_path();
    FILE *fp = fopen(path, "r");
    char **lines = NULL;
    size_t count = 0, cap = 0;
    char line[1200];

    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (count == cap) {
                size_t nc = cap ? cap * 2 : 32;
                char **tmp = realloc(lines, nc * sizeof(*tmp));
                if (!tmp) { fclose(fp); goto fail; }
                lines = tmp;
                cap = nc;
            }
            lines[count] = strdup(line);
            if (!lines[count]) { fclose(fp); goto fail; }
            count++;
        }
        fclose(fp);
    }

    fp = fopen(path, "w");
    if (!fp) goto fail;

    int in_ui = 0, have_ui = 0;
    int wrote_idle = 0, wrote_display = 0, wrote_font = 0;

    for (size_t i = 0; i < count; i++) {
        char check[1200];
        snprintf(check, sizeof(check), "%s", lines[i]);
        trim(check);

        if (check[0] == '[') {
            if (in_ui) {
                if (!wrote_idle) fprintf(fp, "idle_timer_minutes=%d\n", idle);
                if (!wrote_display) fprintf(fp, "display_timeout_seconds=%d\n", display);
                if (!wrote_font) fprintf(fp, "menu_font_size=%d\n", font);
            }
            in_ui = !strcmp(check, "[ui]");
            if (in_ui) have_ui = 1;
            fputs(lines[i], fp);
            continue;
        }

        if (in_ui && !strncmp(check, "volume=", 7)) {
            continue;
        }
        if (in_ui && !strncmp(check, "idle_timer_minutes=", 19)) {
            fprintf(fp, "idle_timer_minutes=%d\n", idle);
            wrote_idle = 1;
            continue;
        }
        if (in_ui && !strncmp(check, "display_timeout_seconds=", 24)) {
            fprintf(fp, "display_timeout_seconds=%d\n", display);
            wrote_display = 1;
            continue;
        }
        if (in_ui && !strncmp(check, "menu_font_size=", 15)) {
            fprintf(fp, "menu_font_size=%d\n", font);
            wrote_font = 1;
            continue;
        }

        fputs(lines[i], fp);
    }

    if (in_ui) {
        if (!wrote_idle) fprintf(fp, "idle_timer_minutes=%d\n", idle);
        if (!wrote_display) fprintf(fp, "display_timeout_seconds=%d\n", display);
        if (!wrote_font) fprintf(fp, "menu_font_size=%d\n", font);
    } else if (!have_ui) {
        if (count > 0 && lines[count-1][0] && lines[count-1][strlen(lines[count-1]) - 1] != '\n') fputc('\n', fp);
        fprintf(fp, "\n[ui]\nidle_timer_minutes=%d\ndisplay_timeout_seconds=%d\nmenu_font_size=%d\n",
                idle, display, font);
    }

    if (fflush(fp) != 0 || fsync(fileno(fp)) != 0) { fclose(fp); goto fail; }
    if (fclose(fp) != 0) goto fail;

    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return 0;

fail:
    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return -1;
}
