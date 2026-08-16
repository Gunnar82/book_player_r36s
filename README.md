# book_player_r36s

> Entwicklung von 0.2 erfolgt im Branch `develop-0.2`. Der stabile 0.1-Stand bleibt auf `main`.

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.2.0-dev1**

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
- Einstellungen sind sortiert: zuerst alle änderbaren Werte, danach System-/Statusinformationen, ganz unten GitHub und Kontakt
- GitHub- und Kontakt-Hinweis unter `Einstellungen`
- USB-Mediatasten, Button-Debug, D-Pad, Analogstick und Shoulder-Button-Navigation

## Projekt / Kontakt

- GitHub: `https://github.com/Gunnar82/book_player_r36s`
- E-Mail: `gunnar_82@hotmail.com`

## Steuerung

- `Y`: Hörspielauswahl
- `X`: Systemmenü mit Einstellungen, Downloads, Button Debug, Beenden und Herunterfahren
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

## 0.2: Downloads aus nginx XML-Listings

0.2 fügt einen lokalen Download-Browser hinzu. Online-Wiedergabe ist bewusst nicht vorgesehen: Dateien werden vollständig heruntergeladen und danach lokal verwendet.

Downloads sind standardmäßig deaktiviert. Unter `Einstellungen` kann `Downloads` persistent auf `An` gestellt werden. Nur dann ist der Punkt `Downloads` im Systemmenü aktiv. Das Aktivieren selbst führt noch keinen Netzwerkzugriff aus; erst das Öffnen des Download-Menüs fragt die konfigurierte URL ab.

Konfiguration:

```ini
[download]
enabled=0
base_url=https://example.org/hoerspiele/
target_path=/roms/hoerspiele
verify_peer=1
verify_host=1
ca_cert=
client_cert=
client_key=
client_key_password=
```

`base_url` darf HTTP oder HTTPS verwenden. Bei HTTPS sind Zertifikats- und Hostprüfung standardmäßig aktiv. `ca_cert` kann auf eine eigene CA-Datei zeigen. Für mTLS können optional `client_cert` und `client_key` gesetzt werden; `client_key_password` ist nur erforderlich, wenn der private Key verschlüsselt ist.

Der Browser erwartet das nginx-XML-Listing mit `<list>`, `<directory>` und `<file>` Einträgen. Verzeichnis- und Dateinamen werden für URLs percent-encoded, damit Leerzeichen und UTF-8-Namen funktionieren.

Ein Verzeichnis mit Dateien bietet `[Ordner herunterladen]`. Downloads werden zuerst als `.part` gespeichert und erst nach erfolgreichem Abschluss auf den eigentlichen Dateinamen umbenannt. `B` kann einen laufenden Download abbrechen. Nach einem erfolgreichen Download ist derzeit ein Neustart des Players nötig, damit die neu geladenen Hörspiele in die lokale Bibliothek eingelesen werden.

### Entwicklungszweig / Patch-Stack

`develop-0.2` basiert weiterhin auf dem stabilen 0.1.14-Quellstand. Die ersten 0.2-Änderungen liegen unter `patches/` als kleiner Patch-Stack. Das Dockerfile wendet diese Patches vor dem Build automatisch an. Im bereitgestellten 0.2-Download-ZIP sind die Änderungen bereits direkt in die Quelldateien eingearbeitet.

### Build-Abhängigkeit

0.2 benötigt zusätzlich `libcurl` mit TLS-Unterstützung. Das Dockerfile installiert `libcurl4-openssl-dev`, `ca-certificates` und `patch`.
