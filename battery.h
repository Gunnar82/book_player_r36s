#ifndef BATTERY_H
#define BATTERY_H

/* Sucht automatisch nach einer Batterie unter /sys/class/power_supply/.
   Muss einmal zu Programmstart aufgerufen werden. */
void init_battery(void);

/* Ladestand in Prozent (0..100), -1 falls keine Batterie gefunden
   oder Wert nicht lesbar. */
int get_battery_percent(void);

/* 1 = laedt gerade, 0 = laedt nicht, -1 = unbekannt. */
int is_battery_charging(void);

/* Geschaetzte Restlaufzeit in Minuten, -1 wenn noch keine belastbare
   Schaetzung moeglich ist oder der Akku gerade geladen wird. */
int get_battery_remaining_minutes(void);

/* Geschaetzte Restzeit bis 100 % in Minuten. Nur waehrend des Ladens;
   -1 wenn noch keine belastbare Schaetzung moeglich ist. */
int get_battery_charge_remaining_minutes(void);

#endif /* BATTERY_H */
