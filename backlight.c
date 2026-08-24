#include <stdio.h>

#include "backlight.h"
#include "config.h"

static int backlight_max = 0;
static int backlight_saved = 0;
static int display_off = 0;

static int read_backlight_value(const char *path)
{
    FILE *fp = fopen(path, "r");

    if (!fp)
        return -1;

    int value = -1;

    if (fscanf(fp, "%d", &value) != 1)
        value = -1;

    fclose(fp);

    return value;
}

static int write_backlight_value(const char *path, int value)
{
    FILE *fp = fopen(path, "w");

    if (!fp) {
        fprintf(stderr,
                "Backlight kann nicht geschrieben werden: %s\n",
                path);
        return -1;
    }

    fprintf(fp, "%d\n", value);
    fclose(fp);

    return 0;
}

void init_backlight(void)
{
    backlight_max = read_backlight_value(BACKLIGHT_MAX_PATH);

    if (backlight_max <= 0) {
        fprintf(stderr,
                "Backlight nicht verfuegbar: %s\n",
                BACKLIGHT_MAX_PATH);
        backlight_max = 0;
        return;
    }

    backlight_saved = read_backlight_value(BACKLIGHT_PATH);

    if (backlight_saved < 0 || backlight_saved > backlight_max)
        backlight_saved = backlight_max;

    fprintf(stderr,
            "Backlight erkannt: max=%d aktuell=%d\n",
            backlight_max, backlight_saved);
}

void set_display_off(int off)
{
#ifdef BUILD_BATOCERA
    if (backlight_max <= 0) {
        display_off = off ? 1 : 0;
        return;
    }
#else
    if (backlight_max <= 0)
        return;
#endif

    if (off) {
        if (!display_off) {
            backlight_saved = read_backlight_value(BACKLIGHT_PATH);

            if (backlight_saved <= 0)
                backlight_saved = backlight_max;

            if (write_backlight_value(BACKLIGHT_PATH, 0) == 0)
                display_off = 1;
        }
    } else {
        if (display_off) {
            if (write_backlight_value(BACKLIGHT_PATH, backlight_saved) == 0)
                display_off = 0;
        }
    }
}

void toggle_display_hw(void)
{
    set_display_off(!display_off);
}

int is_display_off(void)
{
    return display_off;
}

int get_brightness_percent(void)
{
    if (backlight_max <= 0) return -1;
    int value = display_off ? backlight_saved : read_backlight_value(BACKLIGHT_PATH);
    if (value < 0) return -1;
    int percent = (value * 100 + backlight_max / 2) / backlight_max;
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;
    return percent;
}

int set_brightness_percent(int percent)
{
    if (backlight_max <= 0) return -1;
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;
    int value = (backlight_max * percent + 50) / 100;
    if (value < 1) value = 1;
    if (value > backlight_max) value = backlight_max;
    backlight_saved = value;
    if (display_off) return 0;
    return write_backlight_value(BACKLIGHT_PATH, value);
}

int display_needs_software_blank(void)
{
#ifdef BUILD_BATOCERA
    return display_off && backlight_max <= 0;
#else
    return 0;
#endif
}
