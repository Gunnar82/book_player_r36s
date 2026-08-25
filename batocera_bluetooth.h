#ifndef BATOCERA_BLUETOOTH_H
#define BATOCERA_BLUETOOTH_H

#include "bluetooth.h"
#include <SDL2/SDL.h>

int batocera_bluetooth_available(void);
int batocera_bluetooth_enable(void);
int batocera_bluetooth_disable(void);
int batocera_bluetooth_list(BluetoothDevice *devices,int max_devices);
int batocera_bluetooth_connect(const char *mac);
int batocera_bluetooth_disconnect(const char *mac);
int batocera_bluetooth_remove(const char *mac);
int batocera_bluetooth_pair(const char *mac);
int batocera_bluetooth_start_live_devices(void);
void batocera_bluetooth_stop_live_devices(void);
int batocera_bluetooth_live_devices(BluetoothDevice *devices,int max_devices);

/* Der globale Event-Handler reserviert X normalerweise fuer das Systemmenue.
   Im Batocera-Bluetooth-Screen wird X deshalb vor diesem Handler intern auf
   die lokale Entfernen-Aktion umgesetzt. */
int batocera_bluetooth_remap_event(SDL_Event *e);

#endif
