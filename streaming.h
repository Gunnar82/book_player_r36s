#ifndef STREAMING_H
#define STREAMING_H
#include <stddef.h>
#define STREAM_NAME_LEN 256
#define STREAM_URL_LEN 2048
#define STREAM_UUID_LEN 80
#define STREAM_CERT_PATH_LEN 1024

typedef struct {
    char uuid[STREAM_UUID_LEN];
    char name[STREAM_NAME_LEN];
    char url[STREAM_URL_LEN];
    char type[32];
    char group[128];
    char logo[STREAM_URL_LEN];
} StreamEntry;

enum {
    STREAM_CERT_NONE=0,
    STREAM_CERT_DOWNLOADS=1,
    STREAM_CERT_SEPARATE=2
};

extern char stream_xml_url[STREAM_URL_LEN];
extern int stream_cert_mode;
extern char stream_ca_cert[STREAM_CERT_PATH_LEN];
extern char stream_client_cert[STREAM_CERT_PATH_LEN];
extern char stream_client_key[STREAM_CERT_PATH_LEN];
extern char stream_client_key_password[256];

void streaming_load_config(void);
int streaming_save_cert_mode(void);
const char *streaming_cert_mode_name(void);

int streaming_fetch_xml(StreamEntry **entries,int *count,char *err,size_t err_size);
int streaming_favorite_is_set(const char *uuid);
int streaming_favorite_toggle(const char *uuid);

int streaming_start(const StreamEntry *entry,char *err,size_t err_size);
void streaming_stop(void);
int streaming_is_active(void);
int streaming_toggle_pause(void);
int streaming_set_volume(int percent);
int streaming_get_metadata(char *station,size_t station_size,char *title,size_t title_size,char *extra,size_t extra_size);
const char *streaming_current_name(void);
#endif
