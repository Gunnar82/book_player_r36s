# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2_ttf.

## Version

**0.1.5**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner mit `↵ Zurueck`
- Wiedergabe-, Track- und Gesamtfortschritt
- Sleep- und Idle-Timer
- Display-Backlight und Helligkeit
- Akku-, CPU-, RAM- und Temperaturanzeige
- Lautstärke im System-Menü einstellbar
- Audio-Ausgabe im System-Menü sichtbar, sofern SDL sie ermitteln kann
- USB-Mediatasten
- Button-Debug über Linux-Input-Events
- D-Pad, Analogstick und Shoulder-Button-Navigation

## Steuerung

- `Y`: Hörspielauswahl
- `X`: Systemmenü mit System, Button Debug, Beenden und Herunterfahren
- `MID` (`EV_KEY 708`): Display an/aus
- Wiedergabe: Hoch/Runter = +15/-15 Sekunden, Links/Rechts = Track zurück/weiter

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

## USB-Netzwerk-Tools

Die USB-Netzwerk-Skripte aus `scripts/` sind für die Verwendung durch DarkOS auf dem R36S vorgesehen.

Auf dem Gerät müssen sie unter folgendem Pfad abgelegt werden:

```text
/roms/tools/
```

Die beiden DarkOS-/EmulationStation-Starter heißen:

```text
USB-Network-Start.sh
USB-Network-Stop.sh
```

Beide Starter erwarten die Hauptdatei

```text
r36s-usb-ssh-dhcp-server.sh
```

im selben Verzeichnis. Auf dem R36S sollten daher alle drei Dateien gemeinsam unter `/roms/tools/` liegen. `USB-Network-Start.sh` ruft die Hauptdatei mit `start`, `USB-Network-Stop.sh` mit `stop` auf.

## Build

Das Repository enthält ein Dockerfile für ARMv7. Die Screen-Dateien bleiben im Unterordner `src/screens/`; das Zusatzmenü wird aus `screens/systemmenu.c` gebaut.

Benötigt:

- SDL2
- SDL2_mixer
- SDL2_ttf
- gcc
