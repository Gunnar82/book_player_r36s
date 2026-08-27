#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stddef.h>

#define BT_MAX_DEVICES 16

typedef struct {
    char mac[18];
    char name[128];
    int connected;
    char type[16];
} BluetoothDevice;

extern int bluetooth_autoconnect;
extern char bluetooth_device_mac[18];

void bluetooth_load_config(void);
int bluetooth_save_config(void);
int bluetooth_scan_paired_trusted(BluetoothDevice *devices,int max_devices);
int bluetooth_connect_device(const char *mac);
int bluetooth_remove_device(const char *mac);
void bluetooth_autoconnect_start(void);
int bluetooth_adapter_present(void);
int bluetooth_service_available(void);
int bluetooth_adapter_powered(void);
void bluetooth_log_status(void);
void bluetooth_log_if_changed(void);

/* Gemeinsame PulseAudio/PipeWire-Sink-Regelung fuer R36S und GPM2804.
   Erkennt sowohl bluez_sink.* als auch bluez_output.*. */
int bluetooth_audio_sink_get(char *sink_name,size_t sink_name_size,int *volume_percent);
int bluetooth_audio_sink_set_volume(const char *sink_name,int volume_percent);

#endif
