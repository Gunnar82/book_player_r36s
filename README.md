# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.1.12**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner mit `↵ Zurueck`
- Wiedergabe-, Track- und Gesamtfortschritt mit Prozentanzeige im Fließtext
- standardmäßig Stop am Ende des Hörspiels, optional Wiederholung von vorn
- Sleep- und Idle-Timer
- optionales Herunterfahren nach N vollständig abgespielten Tracks
- optionales Herunterfahren am Ende des Hörspiels
- Display-Backlight und Helligkeit
- Akku-, CPU-, RAM- und Temperaturanzeige
- Lautstärke unter `Einstellungen` einstellbar
- Audio-/ALSA-Ausgabe unter `Einstellungen` sichtbar, sofern ermittelbar
- LED-GPIO automatisch ermitteln und unter `Einstellungen` manuell überschreiben
- LED-Test unter `Einstellungen` mit 0,5-Sekunden-Blinktakt
- USB-Mediatasten
- Button-Debug über Linux-Input-Events
- D-Pad, Analogstick und Shoulder-Button-Navigation

## Steuerung

- `Y`: Hörspielauswahl
- `X`: Systemmenü mit Einstellungen, Button Debug, Beenden und Herunterfahren
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

[playback]
repeat_book=0
shutdown_after_tracks=0
shutdown_at_book_end=0
```

`repeat_book=0` ist der Standard: Nach dem letzten Track endet das Hörspiel. Mit `repeat_book=1` beginnt es nach dem letzten Track wieder bei Track 1.

`shutdown_after_tracks=0` deaktiviert den trackbasierten Sleep-Modus. Ein Wert größer 0 fährt das Gerät nach dieser Anzahl vollständig abgespielter Tracks herunter.

`shutdown_at_book_end=1` fährt das Gerät nach dem letzten Track des Hörspiels herunter. Diese Option hat Vorrang vor `repeat_book`.

Die drei Werte unter `[playback]` werden beim Ändern in `Einstellungen` sofort in `config.ini` gespeichert. Ab Version 0.1.12 wird die Datei zusätzlich mit `fflush` und `fsync` dauerhaft geschrieben, damit `shutdown_after_tracks` und `shutdown_at_book_end` einen Programm- oder Systemneustart zuverlässig überleben. Der laufende Countdown für `shutdown_after_tracks` ist getrennt vom gespeicherten Einstellwert.

`led_gpio=-1` bedeutet: Beim ersten Start versucht der Player, unter den bereits exportierten Sysfs-GPIOs einen eindeutigen Ausgang zu erkennen. Wird genau einer gefunden, wird dessen Nummer in `config.ini` gespeichert. Unter `Einstellungen` kann `LED GPIO` anschließend mit Links/Rechts manuell geändert werden; die Änderung wird sofort als `manual` gespeichert. Mit `A` auf dem Eintrag wird die automatische Erkennung erneut ausgeführt.

## Systemberechtigungen

### GPIO-Zugriff für die LED

Die exportierten Sysfs-GPIO-Dateien gehören standardmäßig `root:root` und sind für den Benutzer `ark` nicht beschreibbar. Der Player verwendet deshalb eine eigene Gruppe `gpio` und eine udev-Regel, die die `value`-Dateien exportierter GPIOs für diese Gruppe schreibbar macht.

Gruppe anlegen und `ark` hinzufügen:

```sh
sudo groupadd -f gpio
sudo usermod -aG gpio ark
```

Danach die Datei `/etc/udev/rules.d/99-gpio-led.rules` mit folgendem Inhalt anlegen:

```udev
SUBSYSTEM=="gpio", KERNEL=="gpio[0-9]*", ACTION=="add", RUN+="/bin/chown root:gpio /sys%p/value", RUN+="/bin/chmod 0660 /sys%p/value"
```

Die Regeldatei selbst darf nicht ausführbar sein:

```sh
sudo chmod 0644 /etc/udev/rules.d/99-gpio-led.rules
```

Regeln anschließend neu laden und auf bereits vorhandene GPIOs anwenden:

```sh
sudo udevadm control --reload-rules
sudo udevadm trigger --action=add --subsystem-match=gpio
sudo udevadm settle
```

Nach dem Hinzufügen von `ark` zur Gruppe `gpio` ist eine neue Anmeldung oder ein Neustart erforderlich.

Prüfen:

```sh
id ark
getent group gpio
ls -l /sys/class/gpio/gpio77/value
```

Bei einem erkannten GPIO 77 sollte die `value`-Datei beispielsweise so aussehen:

```text
-rw-rw---- 1 root gpio ... /sys/class/gpio/gpio77/value
```

Die udev-Regel ist absichtlich nicht auf GPIO 77 festgelegt. Unterschiedliche DTBs bzw. Gerätevarianten können die LED unter einer anderen GPIO-Nummer bereitstellen.

### Herunterfahren ohne Passwortabfrage

Damit der Player das Gerät über den Menüpunkt `Herunterfahren` ohne interaktive Passwortabfrage ausschalten kann, benötigt der Benutzer `ark` eine gezielt eingeschränkte `sudo`-Freigabe für `poweroff`.

```sudoers
ark ALL=(root) NOPASSWD: /usr/sbin/poweroff
```

Anschließend:

```sh
sudo chmod 0440 /etc/sudoers.d/hoerspiel-player
sudo visudo -cf /etc/sudoers.d/hoerspiel-player
```

Falls `poweroff` auf dem verwendeten System an einem anderen Pfad liegt, muss in der sudoers-Regel exakt der von `command -v poweroff` ausgegebene Pfad verwendet werden.

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
