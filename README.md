# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.2.2**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner mit `↵ Zurueck`
- Wiedergabe-, Track- und Gesamtfortschritt mit Prozentanzeige im Fließtext
- Akku- und Lautstärkeanzeige direkt auf dem Wiedergabebildschirm
- standardmäßig Stop am Ende des Hörspiels, optional Wiederholung von vorn
- Sleep- und Idle-Timer
- Herunterfahren nach N vollständig abgespielten Tracks
- Herunterfahren am Ende des Hörspiels
- Herunterfahren über `systemd-logind`/D-Bus statt `sudo poweroff`
- Display-Backlight und Helligkeit
- Akku-, CPU-, RAM- und Temperaturanzeige
- Lautstärke, Audio-/ALSA-Ausgabe, LED-GPIO und LED-Test unter `Einstellungen`
- Downloads aus nginx-XML-Listings, standardmäßig deaktiviert
- HTTP/HTTPS mit optionaler eigener CA und optionalen Client-Zertifikaten
- bei aktivierten Downloads werden URL, Zielpfad und TLS-/Client-Zertifikatsparameter unter `Einstellungen` nur lesend angezeigt
- unter `Einstellungen` wird der absolute Pfad der tatsächlich verwendeten `config.ini` angezeigt
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

`config.ini` liegt im selben Verzeichnis wie `hoerspiel_player`. Der Player ermittelt diesen Pfad über `/proc/self/exe`; unter `Einstellungen` wird der daraus resultierende absolute Pfad angezeigt. Dadurch ist sofort sichtbar, welche Datei tatsächlich verwendet wird.

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

`repeat_book=0` ist der Standard. Mit `repeat_book=1` beginnt das Hörspiel nach dem letzten Track wieder bei Track 1. **Nur diese Wiedergabeoption ist persistent.**

`Shutdown nach Tracks` und `Shutdown am Ende` sind **nicht persistent**. Beide starten bei jedem Programmstart mit `Aus` und gelten nur für die aktuelle Sitzung. `Shutdown am Ende` hat während der Sitzung Vorrang vor `repeat_book`.

Downloads sind standardmäßig deaktiviert. Unter `Einstellungen` kann `Downloads` persistent auf `An` gestellt werden. Nur dann ist der Punkt `Downloads` im Systemmenü aktiv. URL, Zielpfad und TLS-Parameter werden direkt aus `config.ini` gelesen.

Bei aktivierten Downloads werden zusätzlich `base_url`, `target_path`, `verify_peer`, `verify_host`, `ca_cert`, `client_cert` und `client_key` im Einstellungsmenü angezeigt. Diese Werte sind dort reine Information und werden weiterhin ausschließlich über `config.ini` geändert. Bei `client_key_password` wird aus Sicherheitsgründen nur angezeigt, ob ein Passwort gesetzt ist.

`base_url` darf HTTP oder HTTPS verwenden. Bei HTTPS sind Zertifikats- und Hostprüfung standardmäßig aktiv. `ca_cert` kann auf eine eigene CA-Datei zeigen. Für mTLS können optional `client_cert` und `client_key` gesetzt werden; `client_key_password` ist nur erforderlich, wenn der private Key verschlüsselt ist. Bei öffentlichen Serverzertifikaten kann `ca_cert` leer bleiben, wenn die System-CA von DarkOS das Zertifikat bereits vertraut.

Der Browser erwartet nginx-XML-Listings mit `<list>`, `<directory>` und `<file>`. Downloads werden zunächst als `.part` gespeichert und nach erfolgreichem Abschluss umbenannt.

Die lokale Ordnerstruktur wird relativ zu `base_url` gespiegelt. Der in `base_url` enthaltene Pfad selbst wird nicht lokal angelegt. Beispiel:

```text
base_url=https://server.example/gunnar/
target_path=/roms/hoerspiele
remote: Hörspiele/Autor/Buch/01.mp3
lokal:  /roms/hoerspiele/Hörspiele/Autor/Buch/01.mp3
```

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

### Herunterfahren über systemd-logind und Polkit

Seit **0.2.2** verwendet der Player kein `sudo poweroff` mehr. Stattdessen ruft er über `busctl` die D-Bus-Methode `org.freedesktop.login1.Manager.PowerOff` von `systemd-logind` auf.

Damit Benutzer `ark` ohne Passwortabfrage herunterfahren darf, wird einmalig eine eng begrenzte Polkit-Regel angelegt:

`/etc/polkit-1/rules.d/50-hoerspiel-player.rules`:

```javascript
polkit.addRule(function(action, subject) {
    if (subject.user == "ark" &&
        (action.id == "org.freedesktop.login1.power-off" ||
         action.id == "org.freedesktop.login1.power-off-multiple-sessions" ||
         action.id == "org.freedesktop.login1.power-off-ignore-inhibit")) {
        return polkit.Result.YES;
    }
});
```

Danach:

```sh
sudo systemctl restart polkit
```

Ungefährlicher Funktionstest:

```sh
busctl call \
  org.freedesktop.login1 \
  /org/freedesktop/login1 \
  org.freedesktop.login1.Manager \
  CanPowerOff
```

Mit passender Polkit-Regel sollte `s "yes"` zurückgegeben werden. Der Player verwendet für das eigentliche Herunterfahren einen nicht-interaktiven D-Bus-Aufruf. Die frühere `sudoers`-Regel für `/usr/sbin/poweroff` wird nicht mehr benötigt und kann entfernt werden.

## ARM64-Build

DarkOS läuft auf AArch64. Die 0.2-Serie wird für `linux/arm64` gebaut und verwendet auf dem Gerät die installierten AArch64-Systembibliotheken. Es wird kein eigener `lib/`-Ordner aus Docker mit ausgeliefert.

Seit 0.2 liegen alle Änderungen vollständig in den Quelldateien; der frühere Patch-Stack wird nicht mehr beim Docker-Build angewendet.

```sh
docker buildx build \
  --platform linux/arm64 \
  --load \
  -t hoerspiel-build-arm64 \
  .

docker rm -f hoerspiel-build-temp 2>/dev/null || true
docker create --name hoerspiel-build-temp hoerspiel-build-arm64
docker cp hoerspiel-build-temp:/build/hoerspiel_player ./
docker rm hoerspiel-build-temp
```

Für normale Builds **kein `--no-cache`** verwenden. Dadurch bleibt insbesondere die Paketinstallation aus dem Dockerfile im Buildx-Cache.

0.2 benötigt zusätzlich `libcurl` mit TLS-Unterstützung. Das Dockerfile installiert zum Kompilieren `libcurl4-openssl-dev` und `ca-certificates`. Auf DarkOS wird die native AArch64-`libcurl.so.4` des Systems verwendet.

## Entwicklung

**0.2.2** ersetzt `sudo poweroff` durch `systemd-logind` über D-Bus/`busctl`. Die Berechtigung wird gezielt über Polkit auf die PowerOff-Aktionen beschränkt.

Die weitere Entwicklung erfolgt direkt auf `main`.
