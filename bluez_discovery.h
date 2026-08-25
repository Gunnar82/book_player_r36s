#ifndef BLUEZ_DISCOVERY_H
#define BLUEZ_DISCOVERY_H

#include "bluetooth.h"

int bluez_discovery_start(void);
void bluez_discovery_stop(void);
int bluez_discovery_devices(BluetoothDevice *devices,int max_devices);

#endif
