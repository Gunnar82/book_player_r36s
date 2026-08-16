# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2_ttf.

## Version

**0.1.8**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner mit `↵ Zurueck`
- Wiedergabe-, Track- und Gesamtfortschritt
- Sleep- und Idle-Timer
- Display-Backlight und Helligkeit
- Akku-, CPU-, RAM- und Temperaturanzeige
- Lautstärke im System-Menü einstellbar
- Audio-/ALSA-Ausgabe im System-Menü sichtbar, sofern ermittelbar
- LED-GPIO automatisch ermitteln und im System-Menü manuell überschreiben
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

[hardware]
led_gpio=-1
led_gpio_mode=auto
```

`led_gpio=-1` bedeutet: Beim ersten Start versucht der Player, unter den bereits exportierten Sysfs-GPIOs einen eindeutigen Ausgang zu erkennen. Wird genau einer gefunden, wird dessen Nummer in `config.ini` gespeichert. Im System-Menü kann `LED GPIO` anschließend mit Links/Rechts manuell geändert werden; die Änderung wird sofort als `manual` gespeichert. Mit `A` auf dem Eintrag wird die automatische Erkennung erneut ausgeführt.

Die GPIO-Berechtigungen für den Benutzer `ark` werden separat eingerichtet und sind nicht Bestandteil der automatischen Erkennung.

## Systemberechtigungen

### Herunterfahren ohne Passwortabfrage

Damit der Player das Gerät über den Menüpunkt `Herunterfahren` ohne interaktive Passwortabfrage ausschalten kann, benötigt der Benutzer `ark` eine gezielt eingeschränkte `sudo`-Freigabe für `poweroff`.

Die Regel sollte mit `visudo` angelegt werden, zum Beispiel als `/etc/sudoers.d/hoerspiel-player`:

```sudoers
ark ALL=(root) NOPASSWD: /usr/sbin/poweroff
```

Anschließend:

```sh
sudo chmod 0440 /etc/sudoers.d/hoerspiel-player
sudo visudo -cf /etc/sudoers.d/hoerspiel-player
```

Falls `poweroff` auf dem verwendeten System an einem anderen Pfad liegt, muss in der sudoers-Regel exakt der von `command -v poweroff` ausgegebene Pfad verwendet werden. Es sollte ausdrücklich **nicht** `NOPASSWD: ALL` vergeben werden.

## USB-Netzwerk-Tools

Die USB-Netzwerk-Skripte aus `scripts/` sind für die Verwendung durch DarkOS auf dem R36S vorgesehen und müssen auf dem Gerät unter `/roms/tools/` liegen:

```text
USB-Network-Start.sh
USB-Network-Stop.sh
r36s-usb-ssh-dhcp-server.sh
```

`USB-Network-Start.sh` ruft die Hauptdatei mit `start`, `USB-Network-Stop.sh` mit `stop` auf. Die Hauptdatei unterstützt zusätzlich `restart`, `status` und ohne Parameter den Toggle-Modus.

## Build

Das Repository enthält ein Dockerfile für ARMv7. Die Screen-Dateien bleiben im Unterordner `src/screens/`; das Zusatzmenü wird aus `screens/systemmenu.c` gebaut.

Benötigt:

- SDL2
- SDL2_mixer
- SDL2_ttf
- gcc
