#include <stdio.h>

#include "led.h"
#include "config.h"

static int led_last_state = -1; /* -1 = noch nie gesetzt */
static int warned_once = 0;

void led_set(int on)
{
    on = on ? 1 : 0;

    if (on == led_last_state)
        return; /* keine Änderung -> nichts zu tun */

    FILE *fp = fopen(LED_VALUE_PATH, "w");
    if (!fp) {
        if (!warned_once) {
            fprintf(stderr,
                    "LED: Kann %s nicht schreiben (Rechte?). "
                    "Blink-Funktion bleibt ohne Effekt.\n",
                    LED_VALUE_PATH);
            warned_once = 1;
        }
        return;
    }

    fprintf(fp, "%d", on);
    fclose(fp);

    led_last_state = on;
}
