# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.2.8**

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner
- Track- und Gesamtfortschritt mit Fortschrittsbalken
- Akku- und Lautstärkeanzeige auf dem Wiedergabebildschirm
- aktiver Sleeptimer mit verbleibender Zeit auf dem Wiedergabebildschirm
- einstellbarer, persistenter Display-Inaktivitätstimer: Aus, 15 s, 30 s, 60 s, 2 min, 5 min oder 10 min
- Sleep- und Idle-Timer
- Herunterfahren über `systemd-logind`/D-Bus
- Downloads aus nginx-XML-Listings mit HTTPS/mTLS, Mehrfachauswahl, rekursiven Ordnerdownloads, Datei-/Gesamtfortschritt und Restzeitschätzung
- USB-/Headset-Mediatasten für Play/Pause, Stop und Trackwechsel
- `KEY_PLAYPAUSE` und `KEY_PLAYCD` (Linux-Keycode 200) werden beide als Play/Pause behandelt
- Headset-Mediatasten bleiben auch bei aktiver Tastensperre nutzbar
- integrierter MPRIS2-Player über D-Bus
- integrierte BlueZ-Media-Registrierung für Bluetooth/AVRCP ohne separates `mpris-proxy`-Programm

## Projekt / Kontakt

- GitHub: `https://github.com/Gunnar82/book_player_r36s`
- E-Mail: `gunnar_82@hotmail.com`

## Steuerung

- `Y`: Hörspielauswahl
- `X`: Systemmenü
- `MID` (`EV_KEY 708`): Display an/aus
- Wiedergabe: Hoch/Runter = +15/-15 Sekunden, Links/Rechts = Track zurück/weiter
- `SELECT`: Tastensperre aktivieren

### Media-Keys

Unter Linux werden unter anderem folgende evdev-Tasten unterstützt:

- `KEY_PREVIOUSSONG`: vorheriger Track
- `KEY_NEXTSONG`: nächster Track
- `KEY_PLAYPAUSE`: Play/Pause
- `KEY_PLAYCD` / Keycode `200`: Play/Pause
- `KEY_PLAY`: Wiedergabe starten/fortsetzen
- `KEY_PAUSE` / `KEY_PAUSECD`: pausieren
- `KEY_STOP` / `KEY_STOPCD`: stoppen

## MPRIS / Bluetooth / Navi

Seit **0.2.8** ist die Media-Brücke direkt in `hoerspiel_player` integriert. Auf DarkOS muss dafür kein separates `mpris-proxy` installiert werden.

Der Player versucht beim Start zwei Wege parallel:

1. Auf einem vorhandenen Benutzer-D-Bus wird `org.mpris.MediaPlayer2.HoerspielPlayer` unter `/org/mpris/MediaPlayer2` bereitgestellt.
2. Auf dem System-D-Bus wird eine BlueZ-Media-Anwendung exportiert. Sobald ein Bluetooth-Adapter mit `org.bluez.Media1` verfügbar ist, registriert sich der Player automatisch über `RegisterApplication`.

Fehlt beim Programmstart ein Bluetooth-Adapter, ist das **kein Fehler**. Der Player läuft normal weiter und versucht die BlueZ-Registrierung regelmäßig erneut. Der Bluetooth-Stick kann deshalb auch erst später vorhanden sein; für den ersten Navi-Test ist es am einfachsten, den Stick einzustecken und anschließend den Player zu starten.

Über MPRIS/BlueZ werden aktuell folgende Steuerbefehle in dieselbe interne Media-Key-Logik eingespeist wie USB-/Headset-Tasten:

- Play
- Pause
- Play/Pause
- Stop
- nächster Track
- vorheriger Track

Zusätzlich werden Wiedergabestatus, aktueller Trackname, Hörspielname, Tracknummer, Tracklänge, Position und Lautstärke veröffentlicht. Seek über MPRIS ist derzeit bewusst nicht freigegeben.

Die BlueZ-Integration basiert auf `libsystemd`/`sd-bus`. Auf dem getesteten DarkOS ist `libsystemd.so` bereits als AArch64-Systembibliothek vorhanden. Nur der Docker-Build benötigt `libsystemd-dev`, damit die Header beim Kompilieren verfügbar sind. Es wird keine zusätzliche `libsystemd` mit dem Player ausgeliefert.

### Test mit eingestecktem Bluetooth-Adapter

Nach dem Start des Players sollten im Log je nach Umgebung Meldungen wie diese erscheinen:

```text
MPRIS: org.mpris.MediaPlayer2.HoerspielPlayer bereit.
MPRIS/BlueZ: Media-Anwendung auf hci0 registriert.
```

Falls kein Session-Bus existiert, kann die erste Meldung fehlen bzw. einen Hinweis auf den nicht nutzbaren Session-Bus liefern. Die BlueZ-Registrierung über den System-Bus ist davon unabhängig.

Mit Adapter kann geprüft werden:

```sh
busctl --system introspect org.bluez /org/bluez/hci0 org.bluez.Media1
```

und lokal, falls ein Benutzer-D-Bus vorhanden ist:

```sh
busctl --user tree org.mpris.MediaPlayer2.HoerspielPlayer
busctl --user get-property \
  org.mpris.MediaPlayer2.HoerspielPlayer \
  /org/mpris/MediaPlayer2 \
  org.mpris.MediaPlayer2.Player \
  PlaybackStatus
```

## Downloadbrowser

Im Downloadbrowser gilt:

- `A`: ausgewählten Ordner öffnen
- `B`: einen Ordner höher; im Basisverzeichnis zurück zum Wiedergabebildschirm
- `Y`: Datei oder Ordner markieren/entmarkieren
- `X`: gesamte Auswahl herunterladen
- `L1`/`R1`: seitenweise navigieren
- `L2`/`R2`: Anfang/Ende

Markierte Ordner werden rekursiv geladen. Die lokale Ordnerstruktur wird relativ zu `base_url` gespiegelt.

## Einstellungen und Timer

Der Sleeptimer wird im Einstellungsmenü konfiguriert. Ist er aktiv, erscheint die Restzeit zusätzlich auf dem Wiedergabebildschirm.

`Display aus nach` kann zwischen `Aus`, `15 s`, `30 s`, `60 s`, `2 min`, `5 min` und `10 min` eingestellt werden. Nach Ablauf wird nur das Backlight abgeschaltet; die Wiedergabe läuft weiter. Die nächste Geräteingabe weckt das Display. Der Wert wird in `~/.hoerspiel_player_state` gespeichert.

Der Idle-Timer ist davon unabhängig und kann das Gerät vollständig herunterfahren.

## Konfiguration

`config.ini` liegt im selben Verzeichnis wie `hoerspiel_player`. Unter `Einstellungen` wird der absolute verwendete Pfad angezeigt.

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

`repeat_book` wird in `config.ini` gespeichert. Lautstärke, Idle-Timer und Display-Inaktivitätstimer werden im lokalen Player-State gespeichert. `Shutdown nach Tracks` und `Shutdown am Ende` gelten nur für die aktuelle Sitzung.

## Systemberechtigungen

### GPIO

```sh
sudo groupadd -f gpio
sudo usermod -aG gpio ark
```

`/etc/udev/rules.d/99-gpio-led.rules`:

```udev
SUBSYSTEM=="gpio", KERNEL=="gpio[0-9]*", ACTION=="add", RUN+="/bin/chown root:gpio /sys%p/value", RUN+="/bin/chmod 0660 /sys%p/value"
```

### Herunterfahren über systemd-logind und Polkit

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

## ARM64-Build

DarkOS läuft auf AArch64. Die 0.2-Serie wird für `linux/arm64` gebaut und verwendet die installierten AArch64-Systembibliotheken. Seit 0.2.8 gehört `libsystemd` für die integrierte D-Bus-/MPRIS-/BlueZ-Anbindung zu den Laufzeitabhängigkeiten; sie ist auf dem getesteten DarkOS bereits vorhanden.

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

Für normale Builds kein `--no-cache` verwenden.

## Entwicklung

**0.2.8** integriert MPRIS2 und eine direkte BlueZ-Media-Anwendung in den Player. Ein separates `mpris-proxy` ist damit für den vorgesehenen Navi-Test nicht mehr nötig. Die BlueZ-Registrierung wird dynamisch versucht und verhindert den Programmstart nicht, wenn kein Bluetooth-Adapter vorhanden ist.

Die weitere Entwicklung erfolgt direkt auf `main`.
