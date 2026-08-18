# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.2.11**

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
- USB-/Headset-Mediatasten für Play/Pause, Play, Pause, Stop und Trackwechsel
- `KEY_PLAYPAUSE` wird als Play/Pause behandelt
- `KEY_PLAYCD` (Linux-Keycode 200) und `KEY_PLAY` werden ausschließlich als Play behandelt
- `KEY_PAUSECD` und `KEY_PAUSE` werden ausschließlich als Pause behandelt
- Headset-Mediatasten bleiben auch bei aktiver Tastensperre nutzbar
- drei Sekunden sichtbare Media-Key-Rückmeldung auf dem Wiedergabebildschirm
- auch unbekannte Tasten eines Media-fähigen evdev-Geräts werden mit ihrem numerischen Linux-Keycode angezeigt
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
- `KEY_PLAYCD` / Keycode `200`: Play
- `KEY_PLAY`: Play
- `KEY_PAUSE` / `KEY_PAUSECD`: Pause
- `KEY_STOP` / `KEY_STOPCD`: Stop

Damit werden getrennte Play- und Pause-Tasten nicht wie ein Toggle behandelt. `KEY_PLAYCD` ist seit 0.2.9 ausschließlich Play.

Seit **0.2.10** erscheint bei einer erkannten Media-Taste für drei Sekunden eine große Rückmeldung auf dem Wiedergabebildschirm. Angezeigt werden die Aktion (`NEXT`, `PREVIOUS`, `PLAY`, `PAUSE`, `PLAY / PAUSE` oder `STOP`), ein großes Symbol und darunter die Herkunft des Kommandos.

Seit **0.2.11** wird bei evdev-Ereignissen der numerische Linux-Keycode immer direkt unter dem Symbol angezeigt. Ist der Keycode noch keiner Player-Aktion zugeordnet, erscheint `?`, `UNBEKANNT` und trotzdem der echte Keycode. Damit dabei nicht jede normale R36S-Taste den Wiedergabebildschirm überflutet, werden unbekannte Codes nur von evdev-Geräten angezeigt, die sich über ihre Fähigkeiten als Media-Gerät erkennen lassen.

Bei Bluetooth-/MPRIS-Kommandos existiert kein Linux-evdev-Keycode. Dort zeigt das Overlay deshalb `Keycode: --` und zusätzlich die empfangene MPRIS-Methode wie `Play`, `Pause`, `Next` oder `Previous`.

## MPRIS / Bluetooth / Navi

Seit **0.2.8** ist die Media-Brücke direkt in `hoerspiel_player` integriert. Auf DarkOS muss dafür kein separates `mpris-proxy` installiert werden.

Der Player versucht beim Start zwei Wege parallel:

1. Auf einem vorhandenen Benutzer-D-Bus wird `org.mpris.MediaPlayer2.HoerspielPlayer` unter `/org/mpris/MediaPlayer2` bereitgestellt.
2. Auf dem System-D-Bus wird eine BlueZ-Media-Anwendung exportiert. Sobald ein Bluetooth-Adapter mit `org.bluez.Media1` verfügbar ist, registriert sich der Player automatisch über `RegisterApplication`.

Fehlt beim Programmstart ein Bluetooth-Adapter, ist das kein Fehler. Der Player läuft normal weiter und versucht die BlueZ-Registrierung regelmäßig erneut.

Über MPRIS/BlueZ werden folgende Steuerbefehle getrennt verarbeitet:

- Play
- Pause
- Play/Pause
- Stop
- nächster Track
- vorheriger Track

Der Bildschirm **Button Debug** zeigt evdev-/SDL-Ereignisse. Bluetooth-/MPRIS-Kommandos kommen dagegen über D-Bus/BlueZ und erscheinen dort nicht als `KEY_PLAYCD`, `KEY_PAUSECD` usw. Seit 0.2.10 werden diese Kommandos dafür direkt auf dem Wiedergabebildschirm als Media-Rückmeldung angezeigt.

Evdev-Erkennung und MPRIS/BlueZ laufen parallel. Normalerweise sind das getrennte Eingabewege. Ein Bluetooth-Stack kann theoretisch denselben physischen Tastendruck sowohl als evdev-Ereignis als auch als MPRIS/AVRCP-Kommando bereitstellen. Dann könnte dieselbe Aktion doppelt beim Player eintreffen. 0.2.11 unterdrückt solche Ereignisse bewusst noch nicht automatisch, weil eine pauschale Entprellung auch absichtliche schnelle Mehrfachdrücke verschlucken könnte. Das Overlay macht die Herkunft sichtbar: `Keycode: ... / Quelle: evdev` auf der einen Seite und `Keycode: -- / MPRIS: ...` auf der anderen. Damit lässt sich auf dem Zielgerät eindeutig feststellen, ob tatsächlich beide Wege gleichzeitig feuern, bevor eine gezielte Deduplizierung eingebaut wird.

Zusätzlich werden Wiedergabestatus, aktueller Trackname, Hörspielname, Tracknummer, Tracklänge, Position und Lautstärke veröffentlicht. Seek über MPRIS ist derzeit nicht freigegeben.

Die BlueZ-Integration basiert auf `libsystemd`/`sd-bus`. Auf dem getesteten DarkOS ist `libsystemd.so` bereits als AArch64-Systembibliothek vorhanden. Nur der Docker-Build benötigt `libsystemd-dev` für die Header.

### Test mit eingestecktem Bluetooth-Adapter

Nach dem Start des Players sollten im Log je nach Umgebung Meldungen wie diese erscheinen:

```text
MPRIS: org.mpris.MediaPlayer2.HoerspielPlayer bereit.
MPRIS/BlueZ: Media-Anwendung auf hci0 registriert.
```

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

**0.2.11** erweitert die Media-Rückmeldung um rohe evdev-Keycodes. Auch noch nicht zugeordnete Tasten eines Media-fähigen Eingabegeräts werden drei Sekunden lang als `UNBEKANNT` mit ihrem numerischen Keycode angezeigt. MPRIS-Kommandos zeigen weiterhin die D-Bus-Methode, da dort kein Linux-Keycode existiert. Die parallelen evdev- und MPRIS-Eingabewege werden damit zugleich sichtbar, um eventuelle Doppelereignisse gezielt diagnostizieren zu können.

Die weitere Entwicklung erfolgt direkt auf `main`.
