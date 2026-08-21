#ifndef CONFIG_H
#define CONFIG_H
#define APP_NAME "Hoerspiel Player"
#define APP_VERSION "0.2.33-lock-nav-usage"
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
 * X     = 2  -> Display an/aus
 * Y     = 3
 * START = 13
 * SELECT= 12  -> Tastensperre aktivieren
 *
 * Steuerkreuz: Hoch=8, Runter=9, Links=10, Rechts=11
 *
 * Schultertasten:
 * L1 = 4   R1 = 5
 * L2 = 6   R2 = 7
 *
 * In Listenansichten (Buecher/Tracks):
 * L1 = eine Seite zurueck     L2 = zum Anfang springen
 * R1 = eine Seite vor          R2 = zum Ende springen
 *
 * Lautstaerke:
 * Lauter = Tastaturcode 128
 * Leiser = Tastaturcode 129
 */
#define BUTTON_A           1
#define BUTTON_B           0
#define BUTTON_X           2
#define BUTTON_Y           3
#define BUTTON_START      13
/* Steuerkreuz */
#define BUTTON_DPAD_UP     8
#define BUTTON_DPAD_DOWN   9
#define BUTTON_DPAD_LEFT  10
#define BUTTON_DPAD_RIGHT 11
/* Schultertasten */
#define BUTTON_L1   4
#define BUTTON_R1   5
#define BUTTON_L2   6
#define BUTTON_R2   7
/* Select = Tastensperre aktivieren */
#define BUTTON_SELECT 12
/* Seitenweite fuer L1 (zurueck) / R1 (vor) in Listenansichten. */
#define LIST_PAGE_SIZE 10
/* Laenge der zufaelligen Tastenfolge zum Entsperren. */
#define UNLOCK_SEQUENCE_LEN 4
#define KEY_VOLUME_UP    128
#define KEY_VOLUME_DOWN  129
/* evdev: Display an/aus */
#define EV_KEY_MID 708
#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_DEADZONE 8000
#define SEEK_STEP 15.0
#define SAVE_INTERVAL_MS 5000
#define VOLUME_STEP 8
/* Sleeptimer: Standardwert und Schrittweite beim Einstellen (in Minuten). */
#define SLEEP_DEFAULT_MINUTES 30
#define SLEEP_STEP_MINUTES     2
#define SLEEP_MIN_MINUTES      0
#define SLEEP_MAX_MINUTES    180

/* Idle-Timer: 0 = aus. Einstellung bleibt ueber Neustarts erhalten. */
#define IDLE_TIMER_STEP_MINUTES 1
#define IDLE_TIMER_MAX_MINUTES 360
/*
 * Blaue LED an GPIO0 (sysfs-Interface). Falls dein System einen
 * anderen Namen verwendet, hier anpassen.
 */
/* Ab wie vielen verbleibenden Sekunden des Sleeptimers die LED blinkt. */
#define LED_BLINK_THRESHOLD_SEC 60
/* Blink-Geschwindigkeit (volle An/Aus-Periode) in Millisekunden. */
#define LED_BLINK_PERIOD_MS 500
/*
 * Linux Backlight
 *
 * Falls dein System einen anderen Namen verwendet,
 * diese beiden Pfade entsprechend anpassen.
 */
#define BACKLIGHT_PATH \
    "/sys/class/backlight/backlight/brightness"
#define BACKLIGHT_MAX_PATH \
    "/sys/class/backlight/backlight/max_brightness"
#endif /* CONFIG_H */
