# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2_ttf.

## Version

**0.1.0**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner mit `↵ Zurueck`
- Wiedergabe-, Track- und Gesamtfortschritt
- Sleep- und Idle-Timer
- Display-Backlight und Helligkeit
- Akku-, CPU-, RAM- und Temperaturanzeige
- USB-Mediatasten
- Button-Debug über Linux-Input-Events
- D-Pad, Analogstick und Shoulder-Button-Navigation

## Konfiguration

`config.ini` liegt zur Laufzeit im selben Verzeichnis wie `hoerspiel_player`.

Beispiel:

```ini
[storage]
path=/roms/hoerspiele
path=/roms2/hoerspiele
path=/mnt/usbdrive/hoerspiele
```

Weitere `path=`-Einträge können ergänzt werden.

## Build

Benötigt:

- SDL2
- SDL2_mixer
- SDL2_ttf
- gcc
