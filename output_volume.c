#include "output_volume.h"
#include "app_log.h"

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define AMIXER "/usr/bin/amixer"
#define OUTPUT_VOLUME_REFRESH_SECONDS 3

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t worker_once = PTHREAD_ONCE_INIT;
static int cached_percent = -1;

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

static int read_output_volume(int *percent)
{
    if (!percent || access(AMIXER, X_OK) != 0) {
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

    *percent = clamp_percent(found);
    return 0;
}

static void update_cache(int percent)
{
    pthread_mutex_lock(&cache_mutex);
    cached_percent = clamp_percent(percent);
    pthread_mutex_unlock(&cache_mutex);
}

static void *volume_worker(void *unused)
{
    (void)unused;
    for (;;) {
        int percent;
        if (read_output_volume(&percent) == 0) {
            update_cache(percent);
        }
        sleep(OUTPUT_VOLUME_REFRESH_SECONDS);
    }
    return NULL;
}

static void start_worker(void)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, volume_worker, NULL) == 0) {
        pthread_detach(thread);
    }
}

static void ensure_worker(void)
{
    pthread_once(&worker_once, start_worker);
}

int output_volume_get(int *percent)
{
    if (!percent) {
        return -1;
    }

    ensure_worker();
    pthread_mutex_lock(&cache_mutex);
    int value = cached_percent;
    pthread_mutex_unlock(&cache_mutex);
    if (value < 0) {
        return -1;
    }

    *percent = value;
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
        update_cache(percent);
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
