# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.1.13**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner mit `↵ Zurueck`
- Wiedergabe-, Track- und Gesamtfortschritt mit Prozentanzeige im Fließtext
- standardmäßig Stop am Ende des Hörspiels, optional Wiederholung von vorn
- Sleep- und Idle-Timer
- Herunterfahren nach N vollständig abgespielten Tracks
- Herunterfahren am Ende des Hörspiels
- Display-Backlight und Helligkeit
- Akku-, CPU-, RAM- und Temperaturanzeige
- Lautstärke, Audio-/ALSA-Ausgabe, LED-GPIO und LED-Test unter `Einstellungen`
- GitHub- und Kontakt-Hinweis unter `Einstellungen`
- USB-Mediatasten, Button-Debug, D-Pad, Analogstick und Shoulder-Button-Navigation

## Projekt / Kontakt

- GitHub: `https://github.com/Gunnar82/book_player_r36s`
- E-Mail: `gunnar_82@hotmail.com`

## Steuerung

- `Y`: Hörspielauswahl
- `X`: Systemmenü mit Einstellungen, Button Debug, Beenden und Herunterfahren
- `MID` (`EV_KEY 708`): Display an/aus
- Wiedergabe: Hoch/Runter = +15/-15 Sekunden, Links/Rechts = Track zurück/weiter

## Konfiguration

`config.ini` liegt im selben Verzeichnis wie `hoerspiel_player`.

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
```

`repeat_book=0` ist der Standard. Mit `repeat_book=1` beginnt das Hörspiel nach dem letzten Track wieder bei Track 1. **Nur diese Wiedergabeoption ist persistent.**

`Shutdown nach Tracks` und `Shutdown am Ende` sind **nicht persistent**. Beide starten bei jedem Programmstart mit `Aus` und gelten nur für die aktuelle Sitzung. `Shutdown am Ende` hat während der Sitzung Vorrang vor `repeat_book`.

## Systemberechtigungen

### GPIO-Zugriff für die LED

```sh
sudo groupadd -f gpio
sudo usermod -aG gpio ark
```

`/etc/udev/rules.d/99-gpio-led.rules`:

```udev
SUBSYSTEM=="gpio", KERNEL=="gpio[0-9]*", ACTION=="add", RUN+="/bin/chown root:gpio /sys%p/value", RUN+="/bin/chmod 0660 /sys%p/value"
```

```sh
sudo chmod 0644 /etc/udev/rules.d/99-gpio-led.rules
sudo udevadm control --reload-rules
sudo udevadm trigger --action=add --subsystem-match=gpio
sudo udevadm settle
```

Nach dem Hinzufügen von `ark` zur Gruppe `gpio` ist eine neue Anmeldung oder ein Neustart erforderlich.

### Herunterfahren ohne Passwortabfrage

```sudoers
ark ALL=(root) NOPASSWD: /usr/sbin/poweroff
```

```sh
sudo chmod 0440 /etc/sudoers.d/hoerspiel-player
sudo visudo -cf /etc/sudoers.d/hoerspiel-player
```

Falls `poweroff` an einem anderen Pfad liegt, muss die sudoers-Regel entsprechend angepasst werden.

## USB-Netzwerk-Tools

Die Skripte `USB-Network-Start.sh`, `USB-Network-Stop.sh` und `r36s-usb-ssh-dhcp-server.sh` müssen unter `/roms/tools/` liegen.

## Build

Das Repository enthält ein Dockerfile für ARMv7. Benötigt werden SDL2, SDL2_mixer, SDL2_ttf und gcc.
