#include "recent_history.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int recent_history_count = RECENT_HISTORY_DEFAULT;

static void trim(char *s)
{
    if (!s) return;
    while (*s == ' ' || *s == '\t') memmove(s, s + 1, strlen(s));
    size_t n = strlen(s);
    while (n && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = '\0';
}

static void clamp_count(void)
{
    if (recent_history_count < RECENT_HISTORY_MIN) recent_history_count = RECENT_HISTORY_MIN;
    if (recent_history_count > RECENT_HISTORY_MAX) recent_history_count = RECENT_HISTORY_MAX;
}

void recent_history_load(void)
{
    recent_history_count = RECENT_HISTORY_DEFAULT;
    FILE *fp = fopen(get_storage_config_path(), "r");
    if (!fp) return;
    char line[1200];
    int in_playback = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (!line[0] || line[0] == '#' || line[0] == ';') continue;
        if (line[0] == '[') {
            in_playback = !strcmp(line, "[playback]");
            continue;
        }
        if (in_playback && !strncmp(line, "recent_history_count=", 21)) {
            recent_history_count = atoi(line + 21);
            break;
        }
    }
    fclose(fp);
    clamp_count();
}

int recent_history_save(void)
{
    clamp_count();
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
                lines = tmp; cap = nc;
            }
            lines[count] = strdup(line);
            if (!lines[count]) { fclose(fp); goto fail; }
            count++;
        }
        fclose(fp);
    }

    fp = fopen(path, "w");
    if (!fp) goto fail;
    int in = 0, have = 0, wrote = 0;
    for (size_t i = 0; i < count; i++) {
        char check[1200];
        snprintf(check, sizeof(check), "%s", lines[i]);
        trim(check);
        if (check[0] == '[') {
            if (in && !wrote) fprintf(fp, "recent_history_count=%d\n", recent_history_count);
            in = !strcmp(check, "[playback]");
            if (in) have = 1;
            fputs(lines[i], fp);
            continue;
        }
        if (in && !strncmp(check, "recent_history_count=", 21)) {
            fprintf(fp, "recent_history_count=%d\n", recent_history_count);
            wrote = 1;
            continue;
        }
        fputs(lines[i], fp);
    }
    if (in && !wrote) fprintf(fp, "recent_history_count=%d\n", recent_history_count);
    else if (!have) {
        if (count && lines[count-1][0] && lines[count-1][strlen(lines[count-1])-1] != '\n') fputc('\n', fp);
        fprintf(fp, "\n[playback]\nrecent_history_count=%d\n", recent_history_count);
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
