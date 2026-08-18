#ifndef MEDIA_KEYS_H
#define MEDIA_KEYS_H

#include <stdint.h>

#define MEDIA_KEYS_MAX_DEVICES 32

typedef enum {
    MEDIA_KEY_NONE = 0,
    MEDIA_KEY_PREVIOUS,
    MEDIA_KEY_NEXT,
    MEDIA_KEY_PLAY_PAUSE,
    MEDIA_KEY_PLAY,
    MEDIA_KEY_PAUSE,
    MEDIA_KEY_STOP,
    MEDIA_KEY_DISPLAY_TOGGLE
} MediaKeyAction;

typedef struct {
    int fds[MEDIA_KEYS_MAX_DEVICES];
    uint64_t last_scan_ms;
} MediaKeys;

void media_keys_init(MediaKeys *mk);
void media_keys_close(MediaKeys *mk);
int media_keys_poll(MediaKeys *mk, MediaKeyAction *actions, int max_actions);

#endif
