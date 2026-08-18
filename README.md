# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.2.7**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner mit `↵ Zurueck`
- Wiedergabe-, Track- und Gesamtfortschritt mit Prozentanzeige im Fließtext
- Akku- und Lautstärkeanzeige direkt auf dem Wiedergabebildschirm
- aktiver Sleeptimer wird mit verbleibender Zeit auf dem Wiedergabebildschirm angezeigt
- einstellbarer, persistenter Display-Inaktivitätstimer: Aus, 15 s, 30 s, 60 s, 2 min, 5 min oder 10 min
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
- Mehrfachauswahl im Downloadbrowser; ausgewählte Ordner werden rekursiv mit allen Unterordnern und Dateien geladen
- Downloadanzeige mit Datei- und Gesamtfortschrittsbalken sowie geschätzter Restzeit für aktuelle Datei und gesamte Auswahl
- bei aktivierten Downloads werden URL, Zielpfad und TLS-/Client-Zertifikatsparameter unter `Einstellungen` nur lesend angezeigt
- unter `Einstellungen` wird der absolute Pfad der tatsächlich verwendeten `config.ini` angezeigt
- GitHub- und Kontakt-Hinweis unter `Einstellungen`
- USB-/Headset-Mediatasten für Play/Pause, Stop und Trackwechsel
- `KEY_PLAYPAUSE` und `KEY_PLAYCD` (Linux-Keycode 200) werden beide als Play/Pause behandelt
- Headset-Mediatasten bleiben auch bei aktiver Tastensperre nutzbar
- Button-Debug, D-Pad, Analogstick und Shoulder-Button-Navigation

## Projekt / Kontakt

- GitHub: `https://github.com/Gunnar82/book_player_r36s`
- E-Mail: `gunnar_82@hotmail.com`

## Steuerung

- `Y`: Hörspielauswahl
- `X`: Systemmenü mit Einstellungen, Downloads, Button Debug, Beenden und Herunterfahren
- `MID` (`EV_KEY 708`): Display an/aus
- Wiedergabe: Hoch/Runter = +15/-15 Sekunden, Links/Rechts = Track zurück/weiter
- `SELECT`: Tastensperre aktivieren; zum Entsperren wird die angezeigte zufällige Tastenfolge verwendet
- Bei aktiver Tastensperre bleiben die über `media_keys` erkannten Headset-/USB-Mediatasten aktiv. Die Sperre betrifft weiterhin die Bedienelemente des Geräts.

### Media-Keys

Unter Linux werden unter anderem folgende evdev-Tasten unterstützt:

- `KEY_PREVIOUSSONG`: vorheriger Track
- `KEY_NEXTSONG`: nächster Track
- `KEY_PLAYPAUSE`: Play/Pause
- `KEY_PLAYCD` / Keycode `200`: Play/Pause
- `KEY_PLAY`: Wiedergabe starten/fortsetzen
- `KEY_PAUSE` / `KEY_PAUSECD`: pausieren
- `KEY_STOP` / `KEY_STOPCD`: stoppen

Damit funktionieren auch Geräte, die ihre zentrale Wiedergabetaste als `KEY_PLAYCD` statt `KEY_PLAYPAUSE` melden.

### Downloadbrowser

Im Downloadbrowser haben `X` und `Y` bewusst eine lokale Funktion. Die globale Belegung für Systemmenü bzw. Hörspielauswahl wird dort nicht ausgeführt.

- `A`: ausgewählten Ordner öffnen
- `B`: einen Ordner höher; im Basisverzeichnis direkt zurück zum Wiedergabe-/Hauptbildschirm
- `Y`: aktuellen Ordner oder aktuelle Datei markieren bzw. Markierung entfernen
- `X`: alle markierten Einträge herunterladen
- D-Pad/Analogstick: Auswahl bewegen
- `L1`/`R1`: seitenweise navigieren
- `L2`/`R2`: zum Anfang/Ende springen

Markierungen bleiben beim Navigieren durch andere Remote-Ordner erhalten. Dadurch können beispielsweise mehrere Hörspielordner eines Autors ausgewählt und anschließend gemeinsam mit `X` geladen werden. Wird ein übergeordneter Ordner markiert, wird er rekursiv traversiert und vollständig heruntergeladen.

Vor dem eigentlichen rekursiven Download ermittelt der Player die Anzahl und die bekannte Gesamtgröße der ausgewählten Dateien. Dadurch zeigt der Downloadbildschirm den Fortschritt über die **gesamte Auswahl** und nicht nur über den gerade bearbeiteten Ordner. Angezeigt werden die aktuelle Datei und die komplette Auswahl jeweils mit Fortschrittsbalken, Prozentwert und geschätzter Restzeit. Direkt zu Beginn eines Downloads wird zunächst `--:--` angezeigt, bis genügend Messdaten für eine Schätzung vorliegen.

## Einstellungen und Timer

Der **Sleeptimer** wird weiterhin im Einstellungsmenü konfiguriert. Ist er aktiv, erscheint seine verbleibende Zeit zusätzlich auf dem Wiedergabebildschirm. Ist er deaktiviert, wird dort keine Sleeptimer-Zeile angezeigt.

Unter `Einstellungen` steht außerdem **Display aus nach** zur Verfügung. Mit Links/Rechts kann zwischen `Aus`, `15 s`, `30 s`, `60 s`, `2 min`, `5 min` und `10 min` gewechselt werden. Nach Ablauf der gewählten Zeit ohne Bedienung wird nur das Backlight abgeschaltet; die Wiedergabe läuft weiter. Die nächste Geräteingabe schaltet das Display wieder ein und wird dabei nur zum Aufwecken verwendet. Der Wert wird zusammen mit den lokalen Player-Einstellungen in `~/.hoerspiel_player_state` gespeichert.

Der **Idle-Timer** ist davon unabhängig: Er kann den Player bei ausbleibender Wiedergabe bzw. Bedienung vollständig herunterfahren. Display-Timeout und Idle-Timer sind daher bewusst zwei verschiedene Funktionen.

## Konfiguration

`config.ini` liegt im selben Verzeichnis wie `hoerspiel_player`. Der Player ermittelt diesen Pfad über `/proc/self/exe`; unter `Einstellungen` wird der daraus resultierende absolute Pfad angezeigt.

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

Mit `repeat_book=1` beginnt das Hörspiel nach dem letzten Track wieder bei Track 1. `repeat_book` wird in `config.ini` gespeichert. Lautstärke, Idle-Timer und Display-Inaktivitätstimer werden im lokalen Player-State gespeichert. `Shutdown nach Tracks` und `Shutdown am Ende` sind dagegen nicht persistent und gelten nur für die aktuelle Sitzung.

Downloads sind standardmäßig deaktiviert. Unter `Einstellungen` kann `Downloads` persistent auf `An` gestellt werden. Nur dann ist der Punkt `Downloads` im Systemmenü aktiv. URL, Zielpfad und TLS-Parameter werden direkt aus `config.ini` gelesen.

Bei aktivierten Downloads werden zusätzlich `base_url`, `target_path`, `verify_peer`, `verify_host`, `ca_cert`, `client_cert` und `client_key` im Einstellungsmenü angezeigt. Diese Werte sind dort reine Information und werden weiterhin ausschließlich über `config.ini` geändert. Bei `client_key_password` wird aus Sicherheitsgründen nur angezeigt, ob ein Passwort gesetzt ist.

`base_url` darf HTTP oder HTTPS verwenden. Bei HTTPS sind Zertifikats- und Hostprüfung standardmäßig aktiv. `ca_cert` kann auf eine eigene CA-Datei zeigen. Für mTLS können optional `client_cert` und `client_key` gesetzt werden; `client_key_password` ist nur erforderlich, wenn der private Key verschlüsselt ist. Bei öffentlichen Serverzertifikaten kann `ca_cert` leer bleiben, wenn die System-CA von DarkOS das Zertifikat bereits vertraut.

Die lokale Ordnerstruktur wird relativ zu `base_url` gespiegelt. Der in `base_url` enthaltene Pfad selbst wird nicht lokal angelegt. Das gilt auch für rekursiv ausgewählte Ordner.

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

### Herunterfahren über systemd-logind und Polkit

Seit **0.2.2** verwendet der Player kein `sudo poweroff` mehr. Stattdessen ruft er über `busctl` die D-Bus-Methode `org.freedesktop.login1.Manager.PowerOff` von `systemd-logind` auf.

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

Mit passender Polkit-Regel sollte `s "yes"` zurückgegeben werden.

## ARM64-Build

DarkOS läuft auf AArch64. Die 0.2-Serie wird für `linux/arm64` gebaut und verwendet auf dem Gerät die installierten AArch64-Systembibliotheken. Es wird kein eigener `lib/`-Ordner aus Docker mit ausgeliefert.

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

## Entwicklung

**0.2.7** stellt den automatischen Display-Timeout wieder her und macht ihn im Einstellungsmenü persistent konfigurierbar. Ein aktiver Sleeptimer wird nun zusätzlich auf dem Wiedergabebildschirm angezeigt. Außerdem werden sowohl `KEY_PLAYPAUSE` als auch `KEY_PLAYCD` (Keycode 200) als Play/Pause akzeptiert.

Die weitere Entwicklung erfolgt direkt auf `main`.
