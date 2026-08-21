#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#define BT_MAX_DEVICES 16

typedef struct {
    char mac[18];
    char name[128];
    int connected;
} BluetoothDevice;

extern int bluetooth_autoconnect;
extern char bluetooth_device_mac[18];

void bluetooth_load_config(void);
int bluetooth_save_config(void);
int bluetooth_scan_paired_trusted(BluetoothDevice *devices,int max_devices);
int bluetooth_connect_device(const char *mac);
void bluetooth_autoconnect_start(void);
int bluetooth_adapter_present(void);

#endif
