#ifndef BACKLIGHT_H
#define BACKLIGHT_H

/* Liest max./aktuellen Wert vom Backlight-Interface ein. */
void init_backlight(void);

/* Schaltet das Display aus (off=1) oder wieder ein (off=0). */
void set_display_off(int off);

/* Kehrt den aktuellen Display-Zustand um. */
void toggle_display(void);

/* 1 = Display ist gerade ausgeschaltet, 0 = an. */
int is_display_off(void);

/* Aktuelle Helligkeit in Prozent (10..100), -1 falls nicht verfuegbar. */
int get_brightness_percent(void);

/* Setzt die Helligkeit in Prozent. Werte werden auf 10..100 begrenzt. */
int set_brightness_percent(int percent);

#endif /* BACKLIGHT_H */
