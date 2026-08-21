#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "battery.h"

#define POWER_SUPPLY_DIR "/sys/class/power_supply"

static char capacity_path[512];
static char status_path[512];
static char energy_now_path[512];
static char energy_full_path[512];
static char power_now_path[512];
static char charge_now_path[512];
static char charge_full_path[512];
static char current_now_path[512];
static int last_percent = -1;
static time_t last_percent_time = 0;
static double discharge_percent_per_hour = 0.0;
static double charge_percent_per_hour = 0.0;
static double smoothed_discharge_current = 0.0;
static double smoothed_charge_current = 0.0;
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

static long long read_number(const char *path)
{
    char value[64];
    if (!path || !path[0] || read_line(path, value, sizeof(value)) != 0)
        return -1;
    char *end = NULL;
    long long v = strtoll(value, &end, 10);
    if (end == value)
        return -1;
    return v;
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
        snprintf(energy_now_path, sizeof(energy_now_path), "%s/%s/energy_now",
                  POWER_SUPPLY_DIR, e->d_name);
        snprintf(energy_full_path, sizeof(energy_full_path), "%s/%s/energy_full",
                  POWER_SUPPLY_DIR, e->d_name);
        snprintf(power_now_path, sizeof(power_now_path), "%s/%s/power_now",
                  POWER_SUPPLY_DIR, e->d_name);
        snprintf(charge_now_path, sizeof(charge_now_path), "%s/%s/charge_now",
                  POWER_SUPPLY_DIR, e->d_name);
        snprintf(charge_full_path, sizeof(charge_full_path), "%s/%s/charge_full",
                  POWER_SUPPLY_DIR, e->d_name);
        snprintf(current_now_path, sizeof(current_now_path), "%s/%s/current_now",
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

    time_t now = time(NULL);
    if (last_percent >= 0 && last_percent_time > 0 && percent != last_percent) {
        double hours = difftime(now, last_percent_time) / 3600.0;
        int delta = percent - last_percent;
        if (hours >= (1.0 / 60.0)) {
            double rate = (delta < 0 ? -delta : delta) / hours;
            if (rate > 0.1 && rate < 100.0) {
                double *target = delta > 0
                               ? &charge_percent_per_hour
                               : &discharge_percent_per_hour;
                if (*target <= 0.0)
                    *target = rate;
                else
                    *target = *target * 0.7 + rate * 0.3;
            }
        }
        last_percent = percent;
        last_percent_time = now;
    } else if (last_percent < 0) {
        last_percent = percent;
        last_percent_time = now;
    }

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


int get_battery_remaining_minutes(void)
{
    if (!battery_found || is_battery_charging() == 1)
        return -1;

    long long energy = read_number(energy_now_path);
    long long power = read_number(power_now_path);
    if (energy > 0 && power > 0) {
        double minutes = ((double)energy / (double)power) * 60.0;
        if (minutes >= 1.0 && minutes < 24.0 * 60.0)
            return (int)(minutes + 0.5);
    }

    long long current_raw = read_number(current_now_path);
    if (current_raw != -1 && current_raw != 0) {
        double current = current_raw < 0
                       ? -(double)current_raw
                       :  (double)current_raw;

        /* current_now schwankt mit CPU-, Display- und Bluetooth-Last.
           Glaetten verhindert eine nervoes springende Restzeitanzeige. */
        if (smoothed_discharge_current <= 0.0)
            smoothed_discharge_current = current;
        else
            smoothed_discharge_current =
                smoothed_discharge_current * 0.85 + current * 0.15;

        long long charge = read_number(charge_now_path);
        if (charge > 0 && smoothed_discharge_current > 0.0) {
            double minutes = ((double)charge / smoothed_discharge_current) * 60.0;
            if (minutes >= 1.0 && minutes < 24.0 * 60.0)
                return (int)(minutes + 0.5);
        }

        /* RK817 auf dem R36S hat kein charge_now, aber charge_full,
           capacity und current_now. Restladung daher naeherungsweise:
               charge_full * capacity / 100 */
        long long charge_full = read_number(charge_full_path);
        int percent = get_battery_percent();
        if (charge_full > 0 && percent > 0 &&
            smoothed_discharge_current > 0.0) {
            double remaining_charge =
                (double)charge_full * (double)percent / 100.0;
            double minutes =
                (remaining_charge / smoothed_discharge_current) * 60.0;
            if (minutes >= 1.0 && minutes < 24.0 * 60.0)
                return (int)(minutes + 0.5);
        }
    }

    if (discharge_percent_per_hour > 0.1) {
        int percent = get_battery_percent();
        if (percent > 0) {
            double minutes = ((double)percent / discharge_percent_per_hour) * 60.0;
            if (minutes >= 1.0 && minutes < 24.0 * 60.0)
                return (int)(minutes + 0.5);
        }
    }

    return -1;
}

int get_battery_charge_remaining_minutes(void)
{
    if (!battery_found || is_battery_charging() != 1)
        return -1;

    int percent = get_battery_percent();
    if (percent >= 100)
        return 0;

    /* Falls der Kernel Energie und Leistung liefert, ist das die direkteste
       Schaetzung: fehlende Energie / momentane Ladeleistung. */
    long long energy_now = read_number(energy_now_path);
    long long energy_full = read_number(energy_full_path);
    long long power = read_number(power_now_path);
    if (energy_now >= 0 && energy_full > energy_now && power > 0) {
        double minutes =
            ((double)(energy_full - energy_now) / (double)power) * 60.0;
        if (minutes >= 1.0 && minutes < 24.0 * 60.0)
            return (int)(minutes + 0.5);
    }

    long long current_raw = read_number(current_now_path);
    if (current_raw != -1 && current_raw != 0) {
        double current = current_raw < 0
                       ? -(double)current_raw
                       :  (double)current_raw;

        if (smoothed_charge_current <= 0.0)
            smoothed_charge_current = current;
        else
            smoothed_charge_current =
                smoothed_charge_current * 0.85 + current * 0.15;

        long long charge_now = read_number(charge_now_path);
        long long charge_full = read_number(charge_full_path);
        if (charge_full > 0 && smoothed_charge_current > 0.0) {
            double missing_charge = -1.0;

            if (charge_now >= 0 && charge_now < charge_full)
                missing_charge = (double)(charge_full - charge_now);
            else if (percent >= 0 && percent < 100)
                missing_charge =
                    (double)charge_full * (double)(100 - percent) / 100.0;

            if (missing_charge >= 0.0) {
                double minutes =
                    (missing_charge / smoothed_charge_current) * 60.0;
                if (minutes >= 1.0 && minutes < 24.0 * 60.0)
                    return (int)(minutes + 0.5);
            }
        }
    }

    /* Fallback fuer Treiber ohne brauchbare Strom-/Kapazitaetswerte.
       Erst nach mindestens einem echten Prozentanstieg verfuegbar. */
    if (charge_percent_per_hour > 0.1 && percent >= 0 && percent < 100) {
        double minutes =
            ((double)(100 - percent) / charge_percent_per_hour) * 60.0;
        if (minutes >= 1.0 && minutes < 24.0 * 60.0)
            return (int)(minutes + 0.5);
    }

    return -1;
}
