#ifndef UPDATE_CONFIG_H
#define UPDATE_CONFIG_H

#include "storage.h"

#define UPDATE_URL_LEN 1024

extern int updates_enabled;
extern int update_use_download_tls;
extern int update_verify_peer;
extern int update_verify_host;
extern char update_base_url[UPDATE_URL_LEN];
extern char update_ca_cert[STORAGE_PATH_LEN];
extern char update_client_cert[STORAGE_PATH_LEN];
extern char update_client_key[STORAGE_PATH_LEN];
extern char update_client_key_password[256];

int update_config_ensure_section(void);
void update_config_load(void);
const char *update_config_ca_cert(void);
const char *update_config_client_cert(void);
const char *update_config_client_key(void);
const char *update_config_client_key_password(void);
int update_config_verify_peer(void);
int update_config_verify_host(void);

#endif
