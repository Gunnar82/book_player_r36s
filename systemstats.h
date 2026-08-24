#ifndef SYSTEMSTATS_H
#define SYSTEMSTATS_H
#include <stddef.h>
double get_cpu_usage(void);
double get_ram_usage(void);
double get_cpu_temperature(void);
int network_connection_active(void);
int network_get_active_ipv4(char *out,size_t out_size);
void network_log_status(void);
void network_log_if_changed(void);
#ifdef BUILD_BATOCERA
int batocera_wifi_enabled(void);
int batocera_set_wifi_enabled(int enabled);
int batocera_set_bluetooth_enabled(int enabled);
#endif

#endif
