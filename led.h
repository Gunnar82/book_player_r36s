#ifndef LED_H
#define LED_H

/* Schaltet die LED an (1) oder aus (0). Schreibt nur bei tatsächlicher
   Zustandsänderung (spart unnötige Dateizugriffe). Bei fehlender
   Schreibberechtigung wird EINMALIG eine Warnung ausgegeben. */
void led_set(int on);

#endif /* LED_H */
