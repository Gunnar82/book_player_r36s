#ifndef BLUETOOTH_DISCOVERY_H
#define BLUETOOTH_DISCOVERY_H

#include "bluetooth.h"

int bluetooth_discovery_start(void);
int bluetooth_discovery_stop(void);
int bluetooth_discovery_active(void);
int bluetooth_scan_all(BluetoothDevice *devices,int max_devices);

#endif
