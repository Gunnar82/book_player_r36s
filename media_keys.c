#include "media_keys.h"
#include "media_feedback.h"
#include "config.h"

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MEDIA_RESCAN_MS 2000

static uint64_t monotonic_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static void scan_devices(MediaKeys *mk)
{
    char path[64];
    for (int i = 0; i < MEDIA_KEYS_MAX_DEVICES; i++) {
        if (mk->fds[i] >= 0) continue;
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd >= 0) mk->fds[i] = fd;
    }
    mk->last_scan_ms = monotonic_ms();
}

void media_keys_init(MediaKeys *mk)
{
    if (!mk) return;
    for (int i = 0; i < MEDIA_KEYS_MAX_DEVICES; i++) mk->fds[i] = -1;
    mk->last_scan_ms = 0;
    scan_devices(mk);
}

void media_keys_close(MediaKeys *mk)
{
    if (!mk) return;
    for (int i = 0; i < MEDIA_KEYS_MAX_DEVICES; i++) {
        if (mk->fds[i] >= 0) { close(mk->fds[i]); mk->fds[i] = -1; }
    }
}

static MediaKeyAction map_key(unsigned int code)
{
    switch (code) {
        case KEY_PREVIOUSSONG: return MEDIA_KEY_PREVIOUS;
        case KEY_NEXTSONG: return MEDIA_KEY_NEXT;
        case KEY_PLAYPAUSE: return MEDIA_KEY_PLAY_PAUSE;
        case KEY_PLAY:
        case KEY_PLAYCD: return MEDIA_KEY_PLAY;
        case KEY_PAUSE:
        case KEY_PAUSECD: return MEDIA_KEY_PAUSE;
        case KEY_STOP:
        case KEY_STOPCD: return MEDIA_KEY_STOP;
        case EV_KEY_MID: return MEDIA_KEY_DISPLAY_TOGGLE;
        default: return MEDIA_KEY_NONE;
    }
}

int media_keys_poll(MediaKeys *mk, MediaKeyAction *actions, int max_actions)
{
    if (!mk || !actions || max_actions <= 0) return 0;
    uint64_t now = monotonic_ms();
    if (mk->last_scan_ms == 0 || now - mk->last_scan_ms >= MEDIA_RESCAN_MS) scan_devices(mk);
    int count = 0;
    for (int i = 0; i < MEDIA_KEYS_MAX_DEVICES && count < max_actions; i++) {
        int fd = mk->fds[i];
        if (fd < 0) continue;
        for (;;) {
            struct input_event ev;
            ssize_t n = read(fd, &ev, sizeof(ev));
            if (n == (ssize_t)sizeof(ev)) {
                /* Nur den echten Tastendruck behandeln. Release (0) und
                   Auto-Repeat (2) duerfen keine Aktion doppelt ausloesen. */
                if (ev.type == EV_KEY && ev.value == 1) {
                    MediaKeyAction action = map_key(ev.code);
                    if (action != MEDIA_KEY_NONE) {
                        actions[count++] = action;
                        media_feedback_show(action,(int)ev.code,"evdev");
                        if (count >= max_actions) return count;
                    }
                }
                continue;
            }
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
            close(fd); mk->fds[i] = -1; break;
        }
    }
    return count;
}
