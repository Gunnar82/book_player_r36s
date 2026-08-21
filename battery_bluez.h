#ifndef BATTERY_BLUEZ_H
#define BATTERY_BLUEZ_H

typedef struct BatteryBluez BatteryBluez;

/* Initialisiert den BlueZ Battery Provider.
   percent darf -1 sein, wenn noch kein Akkustand bekannt ist. */
int battery_bluez_init(BatteryBluez **provider, int percent);

/* Aktualisiert den exportierten Akkustand. */
void battery_bluez_set_percent(BatteryBluez *provider, int percent);

/* D-Bus bedienen und nach einem bluetoothd-Neustart automatisch
   erneut registrieren. Nicht blockierend, daher fuer die Hauptschleife. */
void battery_bluez_process(BatteryBluez *provider);

/* Provider abmelden und Ressourcen freigeben. */
void battery_bluez_close(BatteryBluez *provider);

#endif
