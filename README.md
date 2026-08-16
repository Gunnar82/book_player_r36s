# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.1.9**

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
- LED-Test im System-Menü
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

Diese einfache Variante wurde gewählt, weil die vorherige zusätzliche Prüfung auf `direction=out` beim normalen Boot auf dem getesteten System nicht zuverlässig griff, obwohl sie bei einem manuellen `udevadm trigger` funktionierte.

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

Nach dem Hinzufügen von `ark` zur Gruppe `gpio` ist eine neue Anmeldung oder ein Neustart erforderlich, damit die Gruppenmitgliedschaft in der Benutzersitzung aktiv wird.

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

Ein Schreibtest kann anschließend ohne `sudo` erfolgen:

```sh
echo 0 | tee /sys/class/gpio/gpio77/value
sleep 1
echo 1 | tee /sys/class/gpio/gpio77/value
```

Die udev-Regel ist absichtlich nicht auf GPIO 77 festgelegt. Unterschiedliche DTBs bzw. Gerätevarianten können die LED unter einer anderen GPIO-Nummer bereitstellen. Die Regel gibt nur die jeweilige `value`-Datei für die Gruppe `gpio` frei; `direction`, `export` und `unexport` werden nicht freigegeben.

Zum Debuggen einer Regel kann beispielsweise verwendet werden:

```sh
sudo udevadm test --action=add /sys/class/gpio/gpio77 2>&1 | grep -Ei 'gpio|RUN|chmod|chown'
```

Hinweis: `udevadm test` zeigt die passenden Regeln und `RUN`-Kommandos an, führt die `RUN`-Kommandos selbst aber nicht aus.

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
