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

#endif
