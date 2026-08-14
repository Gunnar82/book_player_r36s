#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "battery.h"

#define POWER_SUPPLY_DIR "/sys/class/power_supply"

static char capacity_path[512];
static char status_path[512];
static int battery_found = 0;

static int read_line(const char *path, char *out, size_t outlen)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    if (!fgets(out, outlen, fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    /* Zeilenumbruch entfernen. */
    size_t len = strlen(out);
    while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r')) {
        out[--len] = '\0';
    }

    return 0;
}

void init_battery(void)
{
    battery_found = 0;

    DIR *dir = opendir(POWER_SUPPLY_DIR);
    if (!dir) {
        fprintf(stderr, "Kein %s vorhanden - keine Akkuanzeige.\n",
                POWER_SUPPLY_DIR);
        return;
    }

    struct dirent *e;
    while ((e = readdir(dir))) {
        if (e->d_name[0] == '.')
            continue;

        char type_path[600];
        snprintf(type_path, sizeof(type_path), "%s/%s/type",
                  POWER_SUPPLY_DIR, e->d_name);

        char type_value[64] = "";
        if (read_line(type_path, type_value, sizeof(type_value)) != 0)
            continue;

        if (strcasecmp(type_value, "Battery") != 0)
            continue;

        snprintf(capacity_path, sizeof(capacity_path), "%s/%s/capacity",
                  POWER_SUPPLY_DIR, e->d_name);
        snprintf(status_path, sizeof(status_path), "%s/%s/status",
                  POWER_SUPPLY_DIR, e->d_name);

        battery_found = 1;
        fprintf(stderr, "Akku erkannt: %s\n", e->d_name);
        break;
    }

    closedir(dir);

    if (!battery_found)
        fprintf(stderr, "Keine Batterie unter %s gefunden.\n", POWER_SUPPLY_DIR);
}

int get_battery_percent(void)
{
    if (!battery_found)
        return -1;

    char value[16];
    if (read_line(capacity_path, value, sizeof(value)) != 0)
        return -1;

    int percent = atoi(value);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    return percent;
}

int is_battery_charging(void)
{
    if (!battery_found)
        return -1;

    char value[32];
    if (read_line(status_path, value, sizeof(value)) != 0)
        return -1;

    if (strcasecmp(value, "Charging") == 0)
        return 1;

    if (strcasecmp(value, "Discharging") == 0 ||
        strcasecmp(value, "Not charging") == 0 ||
        strcasecmp(value, "Full") == 0) {
        return 0;
    }

    return -1;
}
