#ifndef MPRIS_BRIDGE_H
#define MPRIS_BRIDGE_H

#include <stdint.h>
#include <systemd/sd-bus.h>
#include "media_keys.h"

typedef struct {
    sd_bus *session_bus;
    sd_bus *system_bus;
    sd_bus_slot *session_root_slot;
    sd_bus_slot *session_player_slot;
    sd_bus_slot *bluez_manager_slot;
    sd_bus_slot *bluez_player_slot;
    int bluez_registered;
    uint64_t last_bluez_try_ms;
    MediaKeyAction actions[16];
    int action_count;
    char title[256];
    char album[256];
    char playback_status[16];
    int track_number;
    int track_count;
    int64_t length_us;
    int64_t position_us;
    double volume;
} MprisBridge;

void mpris_bridge_init(MprisBridge *bridge);
void mpris_bridge_close(MprisBridge *bridge);
void mpris_bridge_update(MprisBridge *bridge,
                         const char *album,
                         const char *title,
                         int track_number,
                         int track_count,
                         double duration_seconds,
                         double position_seconds,
                         int has_music,
                         int paused,
                         int playing,
                         double volume);
int mpris_bridge_poll(MprisBridge *bridge, MediaKeyAction *actions, int max_actions);
int mpris_bridge_bluez_registered(const MprisBridge *bridge);

#endif
