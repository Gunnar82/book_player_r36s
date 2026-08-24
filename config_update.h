#ifndef CONFIG_UPDATE_H
#define CONFIG_UPDATE_H

#include <stddef.h>

typedef struct {
    const char *key;
    const char *value;
} ConfigUpdate;

int config_update_section(const char *path,
                          const char *section,
                          const ConfigUpdate *updates,
                          size_t update_count);

#endif
