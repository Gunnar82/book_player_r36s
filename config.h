#ifndef CONFIG_H
#define CONFIG_H
#define APP_NAME "Hoerspiel Player"
#define APP_VERSION "0.1.4"
#define SCREEN_W 640
#define SCREEN_H 480
#define MAX_BOOKS 200
#define MAX_TRACKS 500
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
/*
 * Controller-Belegung
 *
 * A     = 1
 * B     = 0
 * X     = 2  -> Systemmenue
 * Y     = 3  -> Hoerspielauswahl
 * START = 13
 * SELECT= 12  -> Tastensperre aktivieren
 *
 * Steuerkreuz: Hoch=8, Runter=9, Links=10, Rechts=11
 *
 * Schultertasten:
 * L1 = 4   R1 = 5
 * L2 = 6   R2 = 7
 */
#define BUTTON_A           1
#define BUTTON_B           0
#define BUTTON_X           2
#define BUTTON_Y           3
#define BUTTON_START      13
#define BUTTON_DPAD_UP     8
#define BUTTON_DPAD_DOWN   9
#define BUTTON_DPAD_LEFT  10
#define BUTTON_DPAD_RIGHT 11
#define BUTTON_L1   4
#define BUTTON_R1   5
#define BUTTON_L2   6
#define BUTTON_R2   7
#define BUTTON_SELECT 12
#define LIST_PAGE_SIZE 10
#define UNLOCK_SEQUENCE_LEN 4
#define KEY_VOLUME_UP    128
#define KEY_VOLUME_DOWN  129
/* Linux evdev: MID-Taste (Display an/aus) */
#define EV_KEY_MID 708
#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_DEADZONE 8000
#define SEEK_STEP 15.0
#define SAVE_INTERVAL_MS 5000
#define VOLUME_STEP 8
#define DISPLAY_TIMEOUT_MS 60000
#define SLEEP_DEFAULT_MINUTES 30
#define SLEEP_STEP_MINUTES     2
#define SLEEP_MIN_MINUTES      0
#define SLEEP_MAX_MINUTES    180
#define IDLE_TIMER_STEP_MINUTES 1
#define IDLE_TIMER_MAX_MINUTES 360
#define LED_VALUE_PATH "/sys/class/gpio/gpio0/value"
#define LED_BLINK_THRESHOLD_SEC 60
#define LED_BLINK_PERIOD_MS 500
#define BACKLIGHT_PATH \
    "/sys/class/backlight/backlight/brightness"
#define BACKLIGHT_MAX_PATH \
    "/sys/class/backlight/backlight/max_brightness"
#endif /* CONFIG_H */
