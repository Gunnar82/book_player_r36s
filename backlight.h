#ifndef BACKLIGHT_H
#define BACKLIGHT_H

/* Liest max./aktuellen Wert vom Backlight-Interface ein. */
void init_backlight(void);

/* Schaltet das Display aus (off=1) oder wieder ein (off=0). */
void set_display_off(int off);

/* Physisches Display direkt an/aus schalten, z.B. EV_KEY 708. */
void toggle_display_hw(void);

/* 1 = Display ist gerade ausgeschaltet, 0 = an. */
int is_display_off(void);

/* Aktuelle Helligkeit in Prozent (10..100), -1 falls nicht verfuegbar. */
int get_brightness_percent(void);

/* Setzt die Helligkeit in Prozent. Werte werden auf 10..100 begrenzt. */
int set_brightness_percent(int percent);

/*
 * BUTTON_X ist der globale Einstieg ins Systemmenue. main.c verwendet
 * historisch toggle_display(); der Makro lenkt diesen Aufruf auf den
 * Systemmenue-Screen. Das echte Backlight-Toggle erfolgt separat ueber
 * toggle_display_hw(), insbesondere fuer EV_KEY 708.
 */
#define toggle_display() do { screen = SCREEN_SLEEP_TIMER; } while (0)

#endif /* BACKLIGHT_H */
