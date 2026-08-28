#include "output_volume.h"
#include "app_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define AMIXER "/usr/bin/amixer"
#define OUTPUT_VOLUME_CACHE_MS 3000ULL

static int cached_percent = -1;
static unsigned long long cached_at_ms = 0;

static unsigned long long monotonic_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (unsigned long long)ts.tv_sec * 1000ULL + (unsigned long long)ts.tv_nsec / 1000000ULL;
}

static int clamp_percent(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

int output_volume_get(int *percent)
{
    if (!percent) {
        return -1;
    }

    unsigned long long now = monotonic_ms();
    if (cached_percent >= 0 && cached_at_ms != 0 && now >= cached_at_ms && now - cached_at_ms < OUTPUT_VOLUME_CACHE_MS) {
        *percent = cached_percent;
        return 0;
    }
    if (access(AMIXER, X_OK) != 0) {
        return -1;
    }

    FILE *fp = popen(AMIXER " get Master 2>/dev/null", "r");
    if (!fp) {
        return -1;
    }

    char line[512];
    int found = -1;
    while (fgets(line, sizeof(line), fp)) {
        char *pct = strchr(line, '%');
        if (!pct) {
            continue;
        }
        char *p = pct;
        while (p > line && isdigit((unsigned char)p[-1])) {
            p--;
        }
        if (p < pct) {
            found = atoi(p);
            break;
        }
    }
    pclose(fp);
    if (found < 0) {
        return -1;
    }

    cached_percent = clamp_percent(found);
    cached_at_ms = now;
    *percent = cached_percent;
    return 0;
}

int output_volume_set(int percent)
{
    if (access(AMIXER, X_OK) != 0) {
        return -1;
    }
    percent = clamp_percent(percent);
    char value[16];
    snprintf(value, sizeof(value), "%d%%", percent);

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        execl(AMIXER, AMIXER, "set", "Master", value, (char *)NULL);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        cached_percent = percent;
        cached_at_ms = monotonic_ms();
        app_logf("Ausgangslautstaerke: Master -> %d%%", percent);
        return 0;
    }
    app_logf("Ausgangslautstaerke: Master konnte nicht gesetzt werden");
    return -1;
}

int output_volume_change(int delta, int *percent)
{
    int current;
    if (output_volume_get(&current) != 0) {
        return -1;
    }
    current = clamp_percent(current + delta);
    if (output_volume_set(current) != 0) {
        return -1;
    }
    if (percent) {
        *percent = current;
    }
    return 0;
}
