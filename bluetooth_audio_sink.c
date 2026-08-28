#include "bluetooth.h"
#include "app_log.h"

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PACTL "/usr/bin/pactl"
#define SINK_REFRESH_SECONDS 3

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t worker_once = PTHREAD_ONCE_INIT;
static char cached_sink[512] = "";
static int cached_volume = -1;

static int is_bluetooth_sink(const char *name)
{
    return name && (!strncmp(name, "bluez_sink.", 11) || !strncmp(name, "bluez_output.", 13));
}

static int read_volume(const char *sink, int *volume)
{
    if (!sink || !volume) {
        return -1;
    }
    char cmd[768];
    snprintf(cmd, sizeof(cmd), PACTL " get-sink-volume '%s' 2>/dev/null", sink);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }
    char line[1024];
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
    if (found > 100) {
        found = 100;
    }
    *volume = found;
    return 0;
}

static int read_sink(char *sink_name, size_t sink_name_size, int *volume_percent)
{
    if (!sink_name || sink_name_size == 0 || !volume_percent || access(PACTL, X_OK) != 0) {
        return -1;
    }

    sink_name[0] = '\0';
    FILE *fp = popen(PACTL " list short sinks 2>/dev/null", "r");
    if (!fp) {
        return -1;
    }

    char line[1024];
    char fallback[512] = "";
    while (fgets(line, sizeof(line), fp)) {
        char *name = strchr(line, '\t');
        if (!name) {
            continue;
        }
        name++;
        char *end = strchr(name, '\t');
        if (!end) {
            continue;
        }
        *end = '\0';
        if (!is_bluetooth_sink(name)) {
            continue;
        }
        if (!fallback[0]) {
            snprintf(fallback, sizeof(fallback), "%s", name);
        }
        if (strstr(end + 1, "RUNNING")) {
            snprintf(sink_name, sink_name_size, "%s", name);
            break;
        }
    }
    pclose(fp);

    if (!sink_name[0] && fallback[0]) {
        snprintf(sink_name, sink_name_size, "%s", fallback);
    }
    if (!sink_name[0]) {
        return -1;
    }
    if (read_volume(sink_name, volume_percent) != 0) {
        sink_name[0] = '\0';
        return -1;
    }
    return 0;
}

static void update_cache(const char *sink_name, int volume_percent)
{
    pthread_mutex_lock(&cache_mutex);
    if (sink_name && sink_name[0] && volume_percent >= 0) {
        snprintf(cached_sink, sizeof(cached_sink), "%s", sink_name);
        cached_volume = volume_percent;
    } else {
        cached_sink[0] = '\0';
        cached_volume = -1;
    }
    pthread_mutex_unlock(&cache_mutex);
}

static void *sink_worker(void *unused)
{
    (void)unused;
    for (;;) {
        char sink_name[512];
        int volume_percent;
        if (read_sink(sink_name, sizeof(sink_name), &volume_percent) == 0) {
            update_cache(sink_name, volume_percent);
        } else {
            update_cache(NULL, -1);
        }
        sleep(SINK_REFRESH_SECONDS);
    }
    return NULL;
}

static void start_worker(void)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, sink_worker, NULL) == 0) {
        pthread_detach(thread);
    }
}

static void ensure_worker(void)
{
    pthread_once(&worker_once, start_worker);
}

int bluetooth_audio_sink_get(char *sink_name, size_t sink_name_size, int *volume_percent)
{
    if (!sink_name || sink_name_size == 0 || !volume_percent) {
        return -1;
    }

    ensure_worker();
    pthread_mutex_lock(&cache_mutex);
    if (!cached_sink[0] || cached_volume < 0) {
        pthread_mutex_unlock(&cache_mutex);
        return -1;
    }
    snprintf(sink_name, sink_name_size, "%s", cached_sink);
    *volume_percent = cached_volume;
    pthread_mutex_unlock(&cache_mutex);
    return 0;
}

int bluetooth_audio_sink_set_volume(const char *sink_name, int volume_percent)
{
    if (!is_bluetooth_sink(sink_name) || access(PACTL, X_OK) != 0) {
        return -1;
    }
    if (volume_percent < 0) {
        volume_percent = 0;
    }
    if (volume_percent > 100) {
        volume_percent = 100;
    }
    char value[16];
    snprintf(value, sizeof(value), "%d%%", volume_percent);
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        execl(PACTL, PACTL, "set-sink-volume", sink_name, value, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        update_cache(sink_name, volume_percent);
        app_logf("Bluetooth Audio-Sink: %s -> %d%%", sink_name, volume_percent);
        return 0;
    }
    app_logf("Bluetooth Audio-Sink: Lautstaerke fuer %s fehlgeschlagen", sink_name);
    return -1;
}
