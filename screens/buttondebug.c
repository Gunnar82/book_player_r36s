#include "buttondebug.h"
#include "../ui.h"

#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEBUG_LINES 12
#define MAX_INPUT_DEVICES 32

typedef struct {
    int fd;
    char path[64];
    char name[128];
} InputDevice;

static char debug_lines[DEBUG_LINES][160];
static int debug_line_count = 0;
static InputDevice devices[MAX_INPUT_DEVICES];
static int device_count = 0;
static int devices_initialized = 0;

static void add_debug_line(const char *text)
{
    if (!text || !*text)
        return;

    if (debug_line_count < DEBUG_LINES) {
        snprintf(debug_lines[debug_line_count], sizeof(debug_lines[0]), "%s", text);
        debug_line_count++;
        return;
    }

    for (int i = 1; i < DEBUG_LINES; i++)
        snprintf(debug_lines[i - 1], sizeof(debug_lines[0]), "%s", debug_lines[i]);

    snprintf(debug_lines[DEBUG_LINES - 1], sizeof(debug_lines[0]), "%s", text);
}

static const char *event_type_name(unsigned short type)
{
    switch (type) {
        case EV_SYN: return "EV_SYN";
        case EV_KEY: return "EV_KEY";
        case EV_REL: return "EV_REL";
        case EV_ABS: return "EV_ABS";
        case EV_MSC: return "EV_MSC";
        case EV_SW:  return "EV_SW";
        case EV_LED: return "EV_LED";
        case EV_SND: return "EV_SND";
        case EV_REP: return "EV_REP";
        case EV_FF:  return "EV_FF";
        case EV_PWR: return "EV_PWR";
        default:     return NULL;
    }
}

static const char *key_code_name(unsigned short code)
{
    switch (code) {
        case KEY_ESC: return "KEY_ESC";
        case KEY_1: return "KEY_1";
        case KEY_2: return "KEY_2";
        case KEY_3: return "KEY_3";
        case KEY_4: return "KEY_4";
        case KEY_5: return "KEY_5";
        case KEY_6: return "KEY_6";
        case KEY_7: return "KEY_7";
        case KEY_8: return "KEY_8";
        case KEY_9: return "KEY_9";
        case KEY_0: return "KEY_0";
        case KEY_ENTER: return "KEY_ENTER";
        case KEY_SPACE: return "KEY_SPACE";
        case KEY_UP: return "KEY_UP";
        case KEY_DOWN: return "KEY_DOWN";
        case KEY_LEFT: return "KEY_LEFT";
        case KEY_RIGHT: return "KEY_RIGHT";
        case KEY_HOME: return "KEY_HOME";
        case KEY_END: return "KEY_END";
        case KEY_PAGEUP: return "KEY_PAGEUP";
        case KEY_PAGEDOWN: return "KEY_PAGEDOWN";
        case KEY_POWER: return "KEY_POWER";
        case KEY_MUTE: return "KEY_MUTE";
        case KEY_VOLUMEDOWN: return "KEY_VOLUMEDOWN";
        case KEY_VOLUMEUP: return "KEY_VOLUMEUP";
        case KEY_PLAYPAUSE: return "KEY_PLAYPAUSE";
        case KEY_STOPCD: return "KEY_STOPCD";
        case KEY_PREVIOUSSONG: return "KEY_PREVIOUSSONG";
        case KEY_NEXTSONG: return "KEY_NEXTSONG";
        case BTN_0: return "BTN_0";
        case BTN_1: return "BTN_1";
        case BTN_2: return "BTN_2";
        case BTN_3: return "BTN_3";
        case BTN_4: return "BTN_4";
        case BTN_5: return "BTN_5";
        case BTN_6: return "BTN_6";
        case BTN_7: return "BTN_7";
        case BTN_8: return "BTN_8";
        case BTN_9: return "BTN_9";
        case BTN_LEFT: return "BTN_LEFT";
        case BTN_RIGHT: return "BTN_RIGHT";
        case BTN_MIDDLE: return "BTN_MIDDLE";
        case BTN_SOUTH: return "BTN_SOUTH";
        case BTN_EAST: return "BTN_EAST";
        case BTN_NORTH: return "BTN_NORTH";
        case BTN_WEST: return "BTN_WEST";
        case BTN_TL: return "BTN_TL";
        case BTN_TR: return "BTN_TR";
        case BTN_TL2: return "BTN_TL2";
        case BTN_TR2: return "BTN_TR2";
        case BTN_SELECT: return "BTN_SELECT";
        case BTN_START: return "BTN_START";
        case BTN_MODE: return "BTN_MODE";
        case BTN_THUMBL: return "BTN_THUMBL";
        case BTN_THUMBR: return "BTN_THUMBR";
        default: return NULL;
    }
}

static const char *abs_code_name(unsigned short code)
{
    switch (code) {
        case ABS_X: return "ABS_X";
        case ABS_Y: return "ABS_Y";
        case ABS_Z: return "ABS_Z";
        case ABS_RX: return "ABS_RX";
        case ABS_RY: return "ABS_RY";
        case ABS_RZ: return "ABS_RZ";
        case ABS_THROTTLE: return "ABS_THROTTLE";
        case ABS_RUDDER: return "ABS_RUDDER";
        case ABS_WHEEL: return "ABS_WHEEL";
        case ABS_GAS: return "ABS_GAS";
        case ABS_BRAKE: return "ABS_BRAKE";
        case ABS_HAT0X: return "ABS_HAT0X";
        case ABS_HAT0Y: return "ABS_HAT0Y";
        case ABS_HAT1X: return "ABS_HAT1X";
        case ABS_HAT1Y: return "ABS_HAT1Y";
        default: return NULL;
    }
}

static const char *rel_code_name(unsigned short code)
{
    switch (code) {
        case REL_X: return "REL_X";
        case REL_Y: return "REL_Y";
        case REL_Z: return "REL_Z";
        case REL_RX: return "REL_RX";
        case REL_RY: return "REL_RY";
        case REL_RZ: return "REL_RZ";
        case REL_HWHEEL: return "REL_HWHEEL";
        case REL_WHEEL: return "REL_WHEEL";
        default: return NULL;
    }
}

static const char *msc_code_name(unsigned short code)
{
    switch (code) {
        case MSC_SERIAL: return "MSC_SERIAL";
        case MSC_PULSELED: return "MSC_PULSELED";
        case MSC_GESTURE: return "MSC_GESTURE";
        case MSC_RAW: return "MSC_RAW";
        case MSC_SCAN: return "MSC_SCAN";
        case MSC_TIMESTAMP: return "MSC_TIMESTAMP";
        default: return NULL;
    }
}

static void format_event_line(char *out, size_t out_size,
                              const char *path, const struct input_event *ev)
{
    const char *type_name = event_type_name(ev->type);
    const char *code_name = NULL;
    char type_buf[24];
    char code_buf[32];

    if (!type_name) {
        snprintf(type_buf, sizeof(type_buf), "EV_%u", ev->type);
        type_name = type_buf;
    }

    switch (ev->type) {
        case EV_KEY: code_name = key_code_name(ev->code); break;
        case EV_ABS: code_name = abs_code_name(ev->code); break;
        case EV_REL: code_name = rel_code_name(ev->code); break;
        case EV_MSC: code_name = msc_code_name(ev->code); break;
        default: break;
    }

    if (!code_name) {
        snprintf(code_buf, sizeof(code_buf), "CODE_%u", ev->code);
        code_name = code_buf;
    }

    snprintf(out, out_size, "%s %s %d %s",
             type_name, code_name, ev->value, path);
}

static void close_input_devices(void)
{
    for (int i = 0; i < device_count; i++) {
        if (devices[i].fd >= 0)
            close(devices[i].fd);
        devices[i].fd = -1;
    }
    device_count = 0;
    devices_initialized = 0;
}

static void init_input_devices(void)
{
    if (devices_initialized)
        return;

    devices_initialized = 1;
    device_count = 0;
    debug_line_count = 0;

    glob_t matches;
    memset(&matches, 0, sizeof(matches));

    if (glob("/dev/input/event*", 0, NULL, &matches) != 0) {
        add_debug_line("Keine /dev/input/event* Geraete gefunden");
        globfree(&matches);
        return;
    }

    int permission_errors = 0;

    for (size_t i = 0; i < matches.gl_pathc && device_count < MAX_INPUT_DEVICES; i++) {
        const char *path = matches.gl_pathv[i];
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            if (errno == EACCES || errno == EPERM)
                permission_errors++;
            continue;
        }

        InputDevice *d = &devices[device_count++];
        memset(d, 0, sizeof(*d));
        d->fd = fd;
        snprintf(d->path, sizeof(d->path), "%s", path);

        if (ioctl(fd, EVIOCGNAME(sizeof(d->name)), d->name) < 0)
            snprintf(d->name, sizeof(d->name), "unbekannt");
    }

    globfree(&matches);

    if (device_count == 0) {
        if (permission_errors > 0)
            add_debug_line("Keine Rechte auf /dev/input/event* (input-Gruppe?)");
        else
            add_debug_line("Keine lesbaren /dev/input/event* Geraete");
    }
}

static void poll_input_devices(void)
{
    init_input_devices();

    for (int i = 0; i < device_count; i++) {
        InputDevice *d = &devices[i];
        if (d->fd < 0)
            continue;

        /* Pro Frame begrenzen, damit ein noisy Device die UI nicht blockiert. */
        for (int n = 0; n < 64; n++) {
            struct input_event ev;
            ssize_t got = read(d->fd, &ev, sizeof(ev));

            if (got == (ssize_t)sizeof(ev)) {
                /* SYN_REPORT ist evdev-Protokollrahmen und flutet sonst die Anzeige. */
                if (ev.type != EV_SYN) {
                    char line[160];
                    format_event_line(line, sizeof(line), d->path, &ev);
                    add_debug_line(line);
                }
                continue;
            }

            if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;

            if (got <= 0) {
                if (got < 0 && errno != EINTR) {
                    char line[160];
                    snprintf(line, sizeof(line), "READ ERROR %s: %s", d->path, strerror(errno));
                    add_debug_line(line);
                }
                break;
            }
        }
    }
}

void buttondebug_handle_event(ScreenContext *ctx, const SDL_Event *e)
{
    char line[160];

    /* Zusaetzlich zu evdev auch die SDL-Sicht protokollieren. So kann
     * die physische BTN_TL/BTN_TR-Zuordnung direkt mit der SDL-Nummer
     * abgeglichen werden. */
    switch (e->type) {
        case SDL_JOYBUTTONDOWN:
            snprintf(line, sizeof(line), "SDL JOYBUTTON DOWN code=%d", e->jbutton.button);
            add_debug_line(line);
            if (e->jbutton.button == BUTTON_A) {
                close_input_devices();
                *ctx->screen = SCREEN_MENU;
            }
            break;

        case SDL_JOYBUTTONUP:
            snprintf(line, sizeof(line), "SDL JOYBUTTON UP   code=%d", e->jbutton.button);
            add_debug_line(line);
            break;

        case SDL_JOYAXISMOTION:
            snprintf(line, sizeof(line), "SDL JOYAXIS axis=%d value=%d",
                     e->jaxis.axis, e->jaxis.value);
            add_debug_line(line);
            break;

        case SDL_JOYHATMOTION:
            snprintf(line, sizeof(line), "SDL JOYHAT hat=%d value=%d",
                     e->jhat.hat, e->jhat.value);
            add_debug_line(line);
            break;

        case SDL_KEYDOWN:
            snprintf(line, sizeof(line), "SDL KEY DOWN key=%d scan=%d",
                     e->key.keysym.sym, e->key.keysym.scancode);
            add_debug_line(line);
            break;

        case SDL_KEYUP:
            snprintf(line, sizeof(line), "SDL KEY UP   key=%d scan=%d",
                     e->key.keysym.sym, e->key.keysym.scancode);
            add_debug_line(line);
            break;

        default:
            break;
    }
}

void buttondebug_render(ScreenContext *ctx)
{
    poll_input_devices();

    draw_text(ctx->renderer, ctx->font,
              "Input Debug (evdev / thd-Stil)", 20, 20, ctx->selected);

    char status[96];
    snprintf(status, sizeof(status), "%d Input-Geraet(e)  |  A: Zurueck", device_count);
    draw_text(ctx->renderer, ctx->font, status, 20, 48, ctx->gray);

    int y = 82;
    if (debug_line_count == 0) {
        draw_text(ctx->renderer, ctx->font,
                  "Warte auf /dev/input/event* ...", 20, y, ctx->white);
    } else {
        for (int i = 0; i < debug_line_count; i++) {
            draw_text(ctx->renderer, ctx->font,
                      debug_lines[i], 20, y, ctx->white);
            y += 28;
        }
    }

    draw_text(ctx->renderer, ctx->font,
              "Format: EV_TYP CODE WERT /dev/input/eventX",
              20, SCREEN_H - 28, ctx->gray);
}
