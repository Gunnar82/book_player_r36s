#ifndef STORAGE_H
#define STORAGE_H

#define MAX_STORAGE_PATHS 32
#define STORAGE_PATH_LEN 512
#define STORAGE_LABEL_LEN 128

typedef struct { char path[STORAGE_PATH_LEN]; char label[STORAGE_LABEL_LEN]; int available; } StoragePath;
int get_storage_paths(StoragePath paths[], int max_paths);
const char *get_audio_directory(void);
const char *get_storage_config_path(void);
int get_led_gpio_config(int *gpio, int *is_manual);
int set_led_gpio_config(int gpio, int is_manual);

#endif
