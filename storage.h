#ifndef STORAGE_H
#define STORAGE_H

#define MAX_STORAGE_PATHS 32
#define STORAGE_PATH_LEN 512
#define STORAGE_LABEL_LEN 128

typedef struct { char path[STORAGE_PATH_LEN]; char label[STORAGE_LABEL_LEN]; int available; } StoragePath;
int get_storage_paths(StoragePath paths[], int max_paths);
const char *get_audio_directory(void);
const char *get_storage_config_path(void);

/* Hardware-Konfiguration in config.ini. gpio=-1 bedeutet Auto. */
int get_led_gpio_config(int *gpio, int *is_manual);
int set_led_gpio_config(int gpio, int is_manual);

/* Wiedergabe-/Sleep-Einstellungen aus config.ini */
extern int repeat_book;
extern int shutdown_after_tracks;
extern int shutdown_at_book_end;
void load_playback_config(void);
int save_playback_config(void);

/* Download-Konfiguration. Nur enabled wird im Menue veraendert;
   URL/Pfade/TLS-Parameter werden aus config.ini gelesen. */
#define DOWNLOAD_URL_LEN 1024
extern int downloads_enabled;
extern int download_verify_peer;
extern int download_verify_host;
extern char download_base_url[DOWNLOAD_URL_LEN];
extern char download_target_path[STORAGE_PATH_LEN];
extern char download_ca_cert[STORAGE_PATH_LEN];
extern char download_client_cert[STORAGE_PATH_LEN];
extern char download_client_key[STORAGE_PATH_LEN];
extern char download_client_key_password[256];
void load_download_config(void);
int save_download_enabled(void);

#endif
