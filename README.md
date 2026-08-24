### Änderung 0.2.28-bt-id-suffix

- Hörspiel-ID wird in der Hörspielauswahl nur angezeigt, wenn ein Bluetooth-Adapter vorhanden ist.
- Die ID steht als Suffix hinter dem Hörspielnamen, z. B. `Hörspielname [1001]`.
- Ohne Bluetooth-Adapter wird nur `Hörspielname` angezeigt; die persistente ID bleibt intern für PBAP/HFP erhalten.

### Änderung 0.2.27-ui-bt-hotplug

- Analoge Joystick-Achsen werden vollständig ignoriert.
- Navigation und Wiedergabesteuerung erfolgen ausschließlich über D-Pad und Buttons.
- Dadurch können USB-/Ladezustandswechsel oder fehlerhafte Analogachsen keine Trackwechsel bzw. ±15-Sekunden-Sprünge mehr auslösen.
- HFP/PBAP und Akku-Ladezeit-Schätzung aus 0.2.25 bleiben unverändert.


### Änderung 0.2.25-battery-charge-eta

- Beim Laden zeigt die Akkuzeile im Wiedergabebildschirm nun eine geschätzte Restzeit bis 100 % an, z. B. `Akku: 72 %  ~48 min bis voll`.
- Der Hörspielname bleibt unverändert in der zweiten Zeile.
- Die Schätzung nutzt bevorzugt Kernelwerte aus `/sys/class/power_supply` (`energy_now`/`energy_full`/`power_now` bzw. `charge_now`/`charge_full`/`current_now`) und glättet den Ladestrom.
- Falls der Treiber diese Werte nicht vollständig liefert, dient der gemessene Prozentanstieg als Fallback.
- Im Systemmenü wechselt `Restlaufzeit` während des Ladens automatisch zu `Bis voll`.
- HFP/PBAP-Funktionalität aus 0.2.24 bleibt unverändert.


### Änderung 0.2.24-pbap-vcards

Beim Bibliotheksscan erzeugt der Player nun automatisch ein PBAP-kompatibles vCard-Telefonbuch unter `$HOME/phonebook/telecom/pb/`. Für jedes Hörspiel wird eine Datei `<ID>.vcf` erzeugt. Der Kontaktname (`FN`) entspricht dem Hörspielnamen, die Telefonnummer (`TEL`) der persistenten HFP-Wähl-ID. Beispiel: `1001.vcf` enthält den Hörspielnamen und `TEL:1001`.

Vor dem Neuaufbau entfernt der Player vorhandene `.vcf`-Dateien in diesem Verzeichnis, damit gelöschte Hörspiele nicht im Navi-Telefonbuch verbleiben. BlueZ `obexd` muss mit dem `dummy`-Phonebook-Backend laufen, das `$HOME/phonebook` als PBAP-Datenquelle verwendet.

# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2/SDL2_ttf.

## Version

**0.3**

### Änderung 0.2.21

- Akku-Restlaufzeit auf dem R36S verbessert.
- Fehlt `charge_now`, wird die Restladung aus `charge_full × capacity / 100` geschätzt.
- `current_now` wird geglättet, damit kurze Lastspitzen die Restzeitanzeige nicht ständig springen lassen.
- Die bisherige Prozent-Verlaufsschätzung bleibt als Fallback erhalten.

### Änderung 0.2.30

- Während eines aktiven Downloads wird der Idle-Timer pausiert. Die Downloadzeit wird nach Ende oder Abbruch nicht nachträglich vom Idle-Timer abgezogen.

### Änderungen 0.2.29

- Hörspiel-IDs werden bei Bluetooth-Hotplug sofort ein- bzw. ausgeblendet; ein Neustart des Players ist nicht mehr erforderlich.
- Sichtbare Hörspiel-IDs stehen als Suffix (`Hörspielname [1001]`) und werden ohne Bluetooth-Adapter nicht angezeigt.

### Änderungen 0.3

- Versionssprung auf `0.3`.
- Funktionsstand entspricht dem zuletzt getesteten 0.2.36-Stand mit Streams, Favoriten, Zertifikatsoptionen, Menü-Keyrepeat und den aktuellen R36S-/Batocera-Buildanpassungen.

### Änderungen 0.2.36 (Streaming-Test)

- Neuer Menüpunkt `Online`.
- Die Streamliste wird über `[streams] xml_url=...` aus einer XML-Datei geladen.
- XML-Format angepasst auf `<station ...></station>` mit Attributen wie `name`, `url`, `url_resolved`, `codec`, `bitrate`, `hls`, `favicon` und `tags`.
- Der Parser nutzt bevorzugt `url_resolved` und fällt auf `url` zurück. `codec`, `tags` und `favicon` werden ebenfalls übernommen.
- Wiedergabe erfolgt über `mpv` als Subprozess.
- Steuerung und Metadaten laufen über `/tmp/hoerspiel-mpv.sock`.
- `icy-name` und `icy-title` erscheinen auf dem Wiedergabebildschirm.
- A/START pausiert bzw. setzt fort, B beendet den Online-Stream.
- XML-Entities wie `&amp;`, `&quot;`, `&apos;`, `&lt;` und `&gt;` werden beim Einlesen dekodiert.



#### Batocera-Starter (GPM2804)

Der GPM2804/Batocera-Port verwendet wieder den bewaehrten Starter unter `/userdata/roms/ports/hoerspiel.sh`. Der Player selbst liegt in `/userdata/roms/ports/Hoerspiel Player`. Der Starter legt den DejaVu-Fontpfad fuer Batocera an, kopiert nur `libsystemd.so.0` nach `/tmp/hoerspiel-libs` und startet das Binary direkt mit passendem `LD_LIBRARY_PATH`.


#### Menünavigation: Taste halten

In Menüs und Listen kann D-Pad **Hoch/Runter** jetzt gehalten werden. Nach ca. 400 ms beginnt die automatische Wiederholung. Nach 5 Sekunden Halten wird die Wiederholung deutlich schneller. Der Wiedergabebildschirm und Button-Debug sind davon ausgenommen.

#### Streams: Quellen, Zertifikate und Favoriten

- Im Streams-Menü sind X und Y von den globalen Shortcuts ausgenommen: Y setzt/entfernt Favoriten, X wechselt zwischen Alle/Favoriten.

`[streams] xml_url` akzeptiert einen lokalen Dateipfad sowie `http://` oder `https://`. Für HTTPS lässt sich unter **Einstellungen > Streams Zertifikat** zwischen `Keins`, `Downloads` und `Separat` umschalten. `Downloads` verwendet die TLS-/Client-Zertifikatswerte aus `[download]`; `Separat` verwendet die eigenen Werte aus `[streams]`.

Im Streams-Menü werden Favoriten anhand der XML-`stationuuid` gespeichert. **Y** setzt oder entfernt den Favoriten der markierten Station. **X** schaltet zwischen **Alle** und **Favoriten** um. Favoriten werden lokal in `stream_favorites.txt` gespeichert. Die maximale Stationszahl des Teststands wurde auf 4096 erhöht.

### Änderungen 0.2.35

- Controllerbelegung kann optional über `[input]` in `config.ini` angepasst werden.
- Ohne `[input]` bzw. mit `profile=r36s` bleibt die bisherige R36S-Belegung unverändert.
- `profile=custom` erlaubt frei belegbare SDL-Buttons sowie ein D-Pad über Joystick-Achsen.
- Im R36S-Profil bleiben analoge Achsen weiterhin vollständig ignoriert, damit die bekannten Phantom-Eingaben bei USB-/Ladezustandswechseln nicht zurückkehren.
- Nicht vorhandene Tasten können im Custom-Profil mit `-1` deaktiviert werden.

Beispiel für Waveshare GPM2804 unter Batocera:

```ini
[input]
profile=custom
dpad_mode=axis
dpad_x_axis=0
dpad_y_axis=1
dpad_deadzone=16000

a=2
b=1
x=3
y=0
l1=4
r1=-1
l2=-1
r2=-1
start=9
select=8
```

### Änderungen 0.2.34

- Neue persistente Einstellung `Schriftgroesse` unter `Einstellungen -> AUDIO / DISPLAY`.
- Stufen: `Klein`, `Normal`, `Gross`, `Sehr gross`.
- Die Skalierung gilt insbesondere für Hörspiel-, Track-, Download-, System- und Einstellungslisten.
- Zeilenhöhe und sichtbare Zeilenanzahl passen sich an die gewählte Schriftgröße an.
- Der Wiedergabebildschirm bleibt zunächst unverändert, damit Fortschritts-, Akku- und Statuslayout stabil bleiben.
- `Normal` entspricht dem bisherigen R36S-Layout; `Gross`/`Sehr gross` sind für kleinere 640x480-Panels wie den Waveshare GPM2804 gedacht.

### Änderungen 0.2.33

- In Listenmenüs entspricht D-Pad Links einer Zurück-Aktion und D-Pad Rechts der A-/Auswahl-Aktion. Das gilt für Hörspiel-Browser, Trackauswahl, Systemmenü und Download-Browser. Im Wiedergabebildschirm bleiben Links/Rechts weiterhin Track zurück/weiter; in den Einstellungen bleiben Links/Rechts für Wertänderungen reserviert.
- Beim Aktivieren der Tastensperre bleibt der Wiedergabebildschirm sichtbar und zeigt `[ LOCKED ]`. Erst ein Tastendruck blendet die Entsperrsequenz ein. Nach 5 Sekunden ohne weitere Eingabe wird wieder der Wiedergabebildschirm angezeigt; die Tastensperre bleibt aktiv.
- Unter Einstellungen gibt es eine persistente Nutzungsstatistik mit App-Starts, gesamter Player-Laufzeit und tatsächlicher Hörzeit.

### Änderung 0.2.31

- Beim Download werden lokal bereits vorhandene Dateien übersprungen, wenn ihre Dateigröße exakt der im Server-Listing angegebenen Größe entspricht.
- Ist die Servergröße unbekannt oder weicht die lokale Größe ab, wird die Datei wie bisher über eine `.part`-Datei neu geladen und erst nach erfolgreichem Abschluss ersetzt.
- Beim Zurückgehen aus einem Unterordner im Download-Browser bleibt der zuvor geöffnete Ordner markiert.

### Änderungen 0.2.32

- Tastendrücke setzen den aktiven Idle-Timer weiterhin auf den vollständigen konfigurierten Ausgangswert zurück.
- Nach jedem Downloadversuch wird der aktive Idle-Timer ebenfalls auf den vollständigen Ausgangswert zurückgesetzt. Das gilt für erfolgreiche Downloads, abgebrochene Downloads und Auswahlen, bei denen vorhandene größenidentische Dateien übersprungen wurden.

## Funktionen

- Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade über `config.ini`
- verschachtelte Hörspielordner
- Track- und Gesamtfortschritt mit Fortschrittsbalken
- Akku- und Lautstärkeanzeige auf dem Wiedergabebildschirm
- Tracktitel auf dem Wiedergabebildschirm bevorzugt aus ID3, mit bisherigem Tracknamen als Fallback
- aktiver Sleeptimer mit verbleibender Zeit auf dem Wiedergabebildschirm
- einstellbarer, persistenter Display-Inaktivitätstimer: Aus, 15 s, 30 s, 60 s, 2 min, 5 min oder 10 min
- Sleep- und Idle-Timer
- persistente Nutzungsstatistik: App-Starts, gesamte Laufzeit und Hörzeit
- Tastensperre mit Wiedergabeansicht, `[ LOCKED ]` und bedarfsgesteuertem Entsperrbildschirm
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
- integriertes Programm-Log im Systemmenü für MPRIS/BlueZ- und Medienstatus
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

Zusätzlich werden Wiedergabestatus, Titel, Album, Artist, Tracknummer, Tracklänge, Position und Lautstärke veröffentlicht. Seit **0.2.12** werden Titel, Album und Artist bevorzugt aus den ID3-Metadaten des aktuell geladenen Tracks übernommen. Fehlt der Titel, wird der Dateiname ohne Erweiterung verwendet. Fehlt das Album, wird der Hörspielordner verwendet; fehlt der Artist, wird dessen übergeordneter Ordner verwendet. An den Albumtext wird immer der anhand der realen Tracklängen berechnete Gesamtfortschritt angehängt, zum Beispiel `Das Paket (42%)`. Die MP3-Dateien und ihre ID3-Tags werden dabei nicht verändert. Seek über MPRIS ist derzeit nicht freigegeben.

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


### Programm-Log / Bluetooth-Debugging

Seit **0.2.14** gibt es im Systemmenü den Eintrag `Programm-Log`. Dort werden die letzten Programmmeldungen direkt auf dem R36S angezeigt. Für die Bluetooth-/Navi-Fehlersuche werden insbesondere MPRIS-Kommandos, aktuell veröffentlichte Titel/Album/Artist-Metadaten, Wiedergabestatus sowie die BlueZ-Media-Registrierung protokolliert. Mit Hoch/Runter kann gescrollt werden, L1/R1 scrollen seitenweise, L2 leert das Log und B kehrt zum Systemmenü zurück.

0.2.14 prüft außerdem die BlueZ-Media-Registrierung regelmäßig erneut. Das ist wichtig, wenn ein externer Bluetooth-Manager `bluetoothd` bzw. den Bluetooth-Dienst neu startet: BlueZ verwirft dabei zuvor registrierte Media-Anwendungen, während der Hörspiel-Player selbst weiterläuft. Der Player erkennt diesen Zustand nun und registriert seine Media-Anwendung erneut, sodass Titel/Album/Artist am Navi nach einem Bluetooth-Neustart wieder erscheinen können.

### Bluetooth-Auswahl und Autoconnect (0.2.15)

Unter `Einstellungen -> Bluetooth` werden nur BlueZ-Geraete angeboten, die sowohl `Paired: yes` als auch `Trusted: yes` sind. Mit `A` wird das markierte Geraet als Ziel gespeichert und eine Verbindung ueber `bluetoothctl connect` versucht. Das gespeicherte Ziel ist mit `*` markiert; aktive Verbindungen werden als `[verbunden]` angezeigt.

`Autoconnect beim Start` kann auf `Ein` oder `Aus` gestellt werden. Einstellung und Ziel-MAC werden in `config.ini` gespeichert:

```ini
[bluetooth]
autoconnect=1
device=00:11:22:33:44:55
```

Beim Programmstart wird bei aktiviertem Autoconnect nach kurzer Verzoegerung ein normaler BlueZ-Verbindungsversuch gestartet. Der Player startet oder beendet dabei weder `bluetoothd` noch PulseAudio. Die PulseAudio-Module `module-bluetooth-policy` und `module-bluetooth-discover` sollten dauerhaft ueber `/etc/pulse/default.pa` geladen werden.


### Bluetooth-Verbindung per D-Bus und lesbareres Log (0.2.16)

Seit 0.2.16 ruft die Bluetooth-Geraeteauswahl BlueZ direkt ueber den System-D-Bus (`org.bluez.Device1.Connect`) auf. Das Programm startet dazu nicht mehr `bluetoothctl` als Unterprozess. Damit ist der Verbindungsweg im Programm derselbe BlueZ-D-Bus-Aufruf, den `bluetoothctl connect` letztlich ebenfalls verwendet, ohne von Terminal-/Prozessumgebung des gestarteten Players abzuhängen. Bei Fehlern werden D-Bus-Fehlername und Fehlermeldung einzeln in das Programm-Log geschrieben; ein fehlgeschlagener Connect wird bis zu drei Mal wiederholt.

Das Programm-Log blendet auf dem R36S den Zeitstempel aus und bricht lange Meldungen auf mehrere Displayzeilen um. Auf stderr bleibt der Zeitstempel erhalten. Dadurch sind insbesondere BlueZ-/MPRIS-Fehlermeldungen vollständig auf dem 640x480-Display lesbar.

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


### Alternatives Dockerfile für GPM2804 / Batocera / Batocera

Zusätzlich zum normalen `Dockerfile` liegt `Dockerfile.gpm2804-batocera` bei. Es baut ein portables ARM64-Runtime-Paket inklusive benötigter Shared Libraries und Starter-Script für ein nicht ausführbares SHARE-Dateisystem. Der Build enthält auch `input_config.c` und legt `config.gpm2804.ini` als Beispiel für die auf dem Waveshare GPM2804 unter Batocera ermittelte Controllerbelegung bei.

Beispiel:

```bash
docker buildx build \
  --platform linux/arm64 \
  -f Dockerfile.gpm2804-batocera \
  --output type=local,dest=./out-gpm2804 \
  .
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

**0.2.13** verwendet auch auf dem Wiedergabebildschirm für die Trackzeile bevorzugt den ID3-Titel des aktuell geladenen Tracks. Fehlt der ID3-Titel oder ist noch kein Track geladen, bleibt der bisherige Trackname/Dateiname als Fallback erhalten. Album-, Fortschritts-, Akku-, Lautstärke- und Timeranzeige bleiben dort unverändert.

**0.2.14** ergänzt ein direkt im Systemmenü sichtbares Programm-Log und macht die BlueZ-Media-Registrierung robust gegen Neustarts von `bluetoothd` durch externe Bluetooth-Manager.

Die weitere Entwicklung erfolgt direkt auf `main`.


**0.2.12** erweitert die MPRIS-/BlueZ-Metadaten: ID3-Titel, -Album und -Artist werden an das Navi übergeben, mit Dateiname/Hörspielordner/übergeordnetem Ordner als Fallback. Das Album enthält zusätzlich den dynamischen Gesamtfortschritt des Hörspiels in Prozent.

0.2.15 ergaenzt die Bluetooth-Geraeteauswahl in den Einstellungen sowie persistentes Autoconnect fuer ein ausgewaehltes paired + trusted Geraet.


0.2.16 ersetzt den Bluetooth-Connect-Unterprozess durch einen direkten BlueZ-D-Bus-Aufruf und macht lange Programm-Logmeldungen auf dem R36S vollständig lesbar.


### Download-Einstellungen als Untermenue (0.2.18)

Der Punkt `Downloads` in den Einstellungen verhaelt sich nun wie `Bluetooth`: Er zeigt `< Oeffnen >` und fuehrt mit A in ein eigenes Download-Untermenue. Dort befinden sich der persistente An/Aus-Schalter sowie die aktuelle Download-/TLS-Konfiguration. Dadurch wird die Hauptseite der Einstellungen deutlich kuerzer und uebersichtlicher.


### Timer-Einstellungen oben (0.2.18)

Der komplette Bereich `TIMER` steht nun ganz oben in den Einstellungen, vor Downloads, Audio/Display und den weiteren Bereichen.


Build-Fix fuer 0.2.18: Die lokalen Variablen `n` und `buf` in `screens/systeminfo.c` werden vor dem nach oben verschobenen TIMER-Block deklariert. Keine Funktionsaenderung.


Build-Fix 2 fuer 0.2.18: doppelte lokale Deklaration von `n` und `buf` in `screens/systeminfo.c` entfernt. Keine Funktionsaenderung.


### Timer direkt aktivieren und Akku-Restlaufzeit (0.2.19)

Ein gespeicherter Sleeptimer kann jetzt mit `A` direkt mit seinem aktuell angezeigten Wert aktiviert werden. Eine vorherige Aenderung um eine Minute ist nicht mehr noetig. Beim Idle-Timer setzt `A` die Restzeit direkt wieder auf den aktuell eingestellten Wert.

Die Akkuanzeige kann zusaetzlich eine geschaetzte Restlaufzeit darstellen. Wenn der Kernel `energy_now`/`power_now` oder `charge_now`/`current_now` unter `/sys/class/power_supply/` bereitstellt, wird daraus direkt die Restzeit berechnet. Fehlen diese Werte, wird nach beobachteten Prozentabfaellen vorsichtig eine geglaettete Schaetzung aufgebaut. Solange keine belastbare Schaetzung vorhanden ist, wird `--` angezeigt.


### Bluetooth-Akkustand am Navi (0.2.20)

Der Player registriert nun zusätzlich einen BlueZ Battery Provider über
`org.bluez.BatteryProviderManager1`. Die Property
`org.bluez.BatteryProvider1.Percentage` wird aus demselben Akkustand gespeist,
den der Player über `/sys/class/power_supply/battery/capacity` verwendet.

Bei einer Änderung des Akkustands sendet der Player ein D-Bus
`PropertiesChanged`-Signal. Gegenstellen, die den Bluetooth-Akkustand anzeigen,
können dadurch den echten R36S-Akkustand statt des bisherigen Werts `0 %`
anzeigen. Die Registrierung wird regelmäßig geprüft und nach einem Neustart von
`bluetoothd` automatisch erneut durchgeführt.

Die Funktion beeinflusst MPRIS/AVRCP, A2DP und die lokale Akkuanzeige nicht.

## Experiment: HFP-Wahlnummern als Player-Kommandos (0.2.21-hfp-test1)

Diese lokale Testfassung enthaelt `hfp_gateway.c/.h`. Der Player versucht beim Start, bei BlueZ das HFP-Audio-Gateway-Profil zu registrieren und empfangene `ATD...;`-Wahlbefehle auszuwerten.

Testbelegung:

- `1001` = Play/Pause
- `1002` = naechster Track
- `1003` = vorheriger Track
- `1004` = Play
- `1005` = Pause
- `1006` = Stop

Jede empfangene Nummer wird zusaetzlich im Programmlog als `HFP Dial: ...` und `HFP Playerkommando: ...` ausgegeben.

### Wichtiger Hinweis zu PulseAudio

PulseAudio 17 kann mit seinem nativen Headset/HFP-Backend selbst das HFP-Profil bei BlueZ registrieren. In diesem Fall kann der Player dasselbe Standardprofil nicht gleichzeitig registrieren. Die Testfassung beendet sich deswegen nicht, sondern schreibt unter anderem `HFP: Profil belegt. PulseAudio-native-HFP ist wahrscheinlich aktiv.` ins Log und laeuft ansonsten normal weiter.

Fuer den eigentlichen HFP-Dial-Test muss spaeter sichergestellt werden, dass PulseAudio weiterhin A2DP bereitstellt, aber das HFP-AG-Profil nicht selbst beansprucht. Diese Umschaltung ist absichtlich noch nicht automatisiert, damit die funktionierende Audio-Konfiguration auf dem R36S beim Experimentieren nicht ungefragt veraendert wird.

## HFP-Hörspielwahl (0.2.22-hfp-books)

Eingehende HFP-Wahlnummern werden jetzt als dauerhafte Hörspiel-IDs behandelt. Beim Bibliotheksscan erhält jedes gefundene Hörspiel eine numerische ID ab `1001`. Die ID wird zusammen mit dem Wiedergabestand in `~/.hoerspiel_player_state` gespeichert und bleibt damit auch nach Neustarts und erneuten Scans erhalten.

Beispiel: Wenn im Log `HFP Buch-ID: 1001 = <Titel>` steht, startet ein vom Navi kommendes `ATD1001;` dieses Hörspiel direkt ab Track 1. Die bisherigen Testkommandos `1001` bis `1006` für Play/Pause/Next usw. wurden entfernt.

Die Zuordnung wird beim Start im Player-Log als `HFP Buch-ID: <ID> = <Titel>` ausgegeben. Diese IDs können später unverändert als Telefonnummern für ein PBAP-Telefonbuch verwendet werden.


## HFP-ID in der Hörspielauswahl (0.2.23-hfp-id-prefix)

Die persistente HFP-Wähl-ID wird in der Hörspielauswahl als Präfix vor dem Titel angezeigt, z. B. `[1001] Die drei ??? - Der Super-Papagei`. Der Ordner- bzw. Hörspielname selbst wird nicht verändert. Auch in der Trackansicht wird die ID vor dem aktuellen Hörspieltitel angezeigt. Dieselbe ID wird weiterhin für `DIAL <ID>` und später für PBAP verwendet.


## 0.2.27 - D-Pad, Bluetooth-Hotplug und Menuekomfort

- Analoge Joystick-Achsen bleiben deaktiviert; Bedienung erfolgt ueber D-Pad und Buttons.
- Ohne eingesteckten Bluetooth-Adapter werden BlueZ-BatteryProvider, HFP-IPC und BlueZ/MPRIS nicht initialisiert oder periodisch abgefragt.
- Ein spaeter eingesteckter Bluetooth-Adapter wird erkannt und die Bluetooth-Funktionen werden wieder aktiviert; beim Abziehen werden sie sauber beendet.
- Beim Zurueckgehen im Hoerspiel-Browser bleibt der zuvor geoeffnete Ordner markiert.
- Button Debug und Programm-Log befinden sich jetzt ganz unten in den Einstellungen unter Diagnose.

- R36S-Dockerfile ergänzt: `streaming.c` und `screens/streams.c` werden jetzt beim normalen Build mitgelinkt.

- Update-Migration: Beim Start werden fehlende `[streams]`-Optionen direkt im vorhandenen `[streams]`-Abschnitt ergänzt. Vorhandene Werte bleiben erhalten; es wird kein zweiter `[streams]`-Block angelegt.

- Streams-Konfiguration korrigiert: Streaming verwendet jetzt denselben, über `get_storage_config_path()` ermittelten `config.ini`-Pfad wie der restliche Player. Migration und Speichern des Zertifikatsmodus arbeiten ebenfalls auf genau dieser Datei. Neue Standard-Configs enthalten `[streams]` direkt.

- Streams-Menü: Hintere Tasten wie im Downloads-Menü belegt: L1/R1 blättern seitenweise, L2/R2 springen an Anfang/Ende.

- `stations.xml` ist nicht mehr Bestandteil des Pakets. Unter `examples/` liegt nur noch `stations.xml.example` als Syntaxbeispiel.


### FFmpeg als Streams-Backend

Die Streams-Wiedergabe verwendet ab Version 0.3 `ffmpeg` statt `mpv`. Damit kann auf R36S und GPM2804/Batocera derselbe Backend-Ansatz verwendet werden, sofern FFmpeg mit ALSA-Ausgabe vorhanden ist.

Prüfen:

```bash
which ffmpeg
ffmpeg -version
ffmpeg -devices | grep -i alsa
```

Der Player startet FFmpeg als Audio-Prozess und gibt auf ALSA `default` aus. **Stop** beendet den FFmpeg-Prozess. **Pause/Weiter** wird über `SIGSTOP` und `SIGCONT` umgesetzt. ICY-Metadaten werden, soweit FFmpeg sie ausgibt, aus dem FFmpeg-Log gelesen und auf dem Wiedergabebildschirm dargestellt.



### Große Streams-Listen

Die Stationsliste hat keine feste Grenze von 4096 Einträgen mehr. Der Parser startet mit einem kleinen Puffer und vergrößert ihn bei Bedarf dynamisch per `realloc()`. Dadurch werden auch deutlich größere XML-Dateien vollständig eingelesen, solange genügend RAM verfügbar ist. Beim Neuladen bzw. Zurücksetzen der Streams-Liste wird der belegte Speicher wieder freigegeben.


- Compiler-Warnung in `screens/streams.c` behoben: versehentlich eingebettetes NUL-Zeichen durch korrektes `\\0`-Literal ersetzt.


### Streams: Navigation und Rückkehr

- Während einer laufenden Stream-Wiedergabe werden A/START/B/Links nicht mehr an die lokale Hörspielsteuerung durchgereicht.
- B bzw. Links beendet den Stream und kehrt direkt in die Streams-Liste zurück.
- Sind Favoriten vorhanden, öffnet die Streams-Liste standardmäßig die Favoritenansicht. Ohne Favoriten wird weiterhin `Alle` angezeigt.



### Streams-Backend: mpv mit FFmpeg-Fallback

Der Player verwendet für Streams automatisch das beste verfügbare Backend:

1. **mpv**, wenn vorhanden und startbar. mpv wird bevorzugt, weil ICY-Metadaten wie Sendername und aktueller Titel sauber über den IPC-Socket abgefragt werden können.
2. **FFmpeg**, wenn mpv fehlt oder nicht gestartet werden kann.

Im Streams-Menü wird das aktuell bevorzugte/verfügbare Backend in der Überschrift angezeigt. Während der Wiedergabe zeigt die Statuszeile ebenfalls `mpv` bzw. `ffmpeg`.



### Display-/DRM-Entlastung

Der Hauptloop ist auf ungefähr 30 FPS begrenzt. Zusätzlich werden Stream-Metadaten nur noch etwa alle 500 ms aktualisiert. Das reduziert unnötige SDL/DRM-Pageflips und IPC-/Logzugriffe, insbesondere auf dem R36S.



### Stream-Wiedergabemodus

Der Wiedergabebildschirm unterscheidet jetzt dauerhaft zwischen einer ausgewählten Stream-Sitzung und der lokalen Hörspielwiedergabe. Beendet sich mpv/FFmpeg unerwartet, fällt die Oberfläche nicht mehr auf den Hörspielplayer zurück. Stattdessen wird ein Stream-Fehler angezeigt; B bzw. Links führt zurück zur Streams-Liste.



- Streams-Menü: L1/R1 unterstützen jetzt ebenfalls Halte-Wiederholung. Nach kurzer Verzögerung wird seitenweise weitergeblättert; nach ca. 5 Sekunden beschleunigt die Wiederholung.


### Docker-Builds per Makefile

Für die beiden Gerätevarianten gibt es jetzt kurze Make-Ziele. Docker darf dabei seinen Build-Cache verwenden; `--no-cache` wird nicht gesetzt.

R36S:

```bash
make r36s
```

Das baut mit dem normalen `Dockerfile` das ARM64-Image und kopiert anschließend `hoerspiel_player` in das aktuelle Projektverzeichnis.

GPM2804 mit Batocera:

```bash
make gpm2804
```

Das verwendet `Dockerfile.gpm2804-batocera` und exportiert das Paket direkt nach:

```text
dist-batocera/
```




### Radio-Favoriten in `config.ini`

Radio-/Stream-Favoriten werden nicht mehr in `stream_favorites.txt` gespeichert, sondern direkt im `[streams]`-Abschnitt der `config.ini`. Gespeichert werden weiterhin die stabilen UUIDs der Stationen:

```ini
[streams]
favorites=uuid-1,uuid-2,uuid-3
```

Fehlt `favorites`, wird der Eintrag beim Ergänzen der Streams-Konfiguration automatisch als leerer Wert angelegt. Das separate `stream_favorites.txt` wird nicht mehr verwendet.


### FFmpeg-ICY-Metadaten

Der FFmpeg-Fallback wertet ICY-Metadaten jetzt tolerant als `key : value` aus. Dadurch werden auch die von FFmpeg üblichen, ausgerichteten Zeilen erkannt, zum Beispiel:

```text
icy-name        : 104.6 RTL Berlin Livestream
StreamTitle     : 104.6 RTL Berlin Livestream
```

Zusätzlich werden weiterhin Varianten wie `StreamTitle=...` akzeptiert.


### Streams und Idle-Timer

Während ein Stream tatsächlich wiedergegeben wird, wird der Idle-Timer wie bei einer laufenden Hörspielwiedergabe angehalten. Wird der Stream pausiert, läuft der Idle-Timer wieder herunter. Tastendrücke setzen den Idle-Timer weiterhin global zurück, also auch im Streams-Menü.

Der Stream-Wiedergabescreen zeigt zusätzlich, soweit vom Sender/Backend verfügbar, Backend, Bitrate, Samplerate, Kanalzahl und ICY-Beschreibung an.


### Streams-Favoriten: Performance

Die Favoriten-UUIDs werden beim ersten Zugriff einmal aus `[streams] favorites=` der `config.ini` geladen und danach im RAM gecacht. Die Favoritenansicht öffnet die INI daher nicht mehr für jede Station und jeden Renderdurchlauf. Das beseitigt das deutliche Nachhängen bei großen Stationslisten. Beim Setzen oder Entfernen eines Favoriten werden Cache und `config.ini` gemeinsam aktualisiert.



### Streams: Auswahl beibehalten

Die Streams-Auswahl wird über `stationuuid` gemerkt. Nach einer Wiedergabe und Rückkehr zur Liste bleibt der zuletzt gestartete Sender markiert. Beim Umschalten zwischen `Alle` und `Favoriten` bleibt derselbe Sender ausgewählt, sofern er in der Zielansicht vorhanden ist.


### QR-Code auf dem Stream-Wiedergabescreen

Rechts auf dem Stream-Wiedergabescreen wird ein echter QR-Code der aktuell verwendeten Stream-URL angezeigt. Der Code wird mit `libqrencode` erzeugt. Die benötigte `libqrencode.so.4` wird beim Build mit ausgeliefert: beim GPM2804/Batocera im Paket unter `lib/`, beim R36S kopiert `make r36s` sie ebenfalls nach `./lib/`.

Die ICY-Informationen bleiben links sichtbar. Dazu gehören, soweit vom Sender geliefert, Sendername, `StreamTitle`, Backend, Bitrate, Samplerate, Kanalzahl und `icy-description`. Lange Texte werden vor dem QR-Bereich gekürzt, damit sie den Code nicht überzeichnen.


- QR-Code-Linking: `libqrencode` wird über `pkg-config` dynamisch gelinkt. Die Runtime-Bibliothek wird für Batocera und R36S mit ausgeliefert.


### Plattformabhängige TTF-Pfade

Der Fontpfad wird jetzt bereits beim Build gewählt:

- `BUILD_R36S`: `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`
- `BUILD_BATOCERA`: `/usr/share/fonts/dejavu/DejaVuSans.ttf`

Das normale R36S-Dockerfile kompiliert mit `-DBUILD_R36S`, das GPM2804/Batocera-Dockerfile mit `-DBUILD_BATOCERA`. Zusätzlich wurden feste `--platform=linux/arm64`-Angaben aus den `FROM`-Zeilen entfernt; die Zielarchitektur kommt weiterhin über das Makefile bzw. `docker build --platform linux/arm64`.


### Build 0.3.1

Aktueller Entwicklungsstand mit Streams-Favoriten-Cache, beibehaltener Senderauswahl, erweitertem Stream-Wiedergabescreen inklusive ICY-Informationen und QR-Code sowie plattformabhängigen TTF-Pfaden für R36S/DarkOSRE und GPM2804/Batocera.


### Build 0.3.49

- Batocera-Starter ergänzt: `libqrencode.so.4` wird zusammen mit `libsystemd.so.0` gezielt nach `/tmp/hoerspiel-libs` kopiert.
- Der Starter setzt weiterhin nur diesen kontrollierten temporären Library-Pfad, statt das komplette Debian-Library-Bundle vor Batoceras eigene Bibliotheken zu schieben.

### Build 0.3.49

Weitere Bereinigung der vollständigen R36S-Warning-Liste:

- INI-Section-Namen in `streaming.c` werden längengeprüft übernommen.
- `-Wmisleading-indentation` in `mpris_bridge.c` und `bluetooth.c` behoben.
- PBAP-Verzeichnispfade werden vor dem Zusammensetzen auf ihre Länge geprüft.
- Veraltete `selection_progress()`-Deklaration aus `download.c` entfernt.
- Unbenutzte `dl`-Variable aus `screens/systeminfo.c` entfernt.
- Menülabels in `screens/menu.c` werden kontrolliert auf die sichtbare Puffergröße begrenzt.
- Das externe Makefile verwendet für Docker-Builds standardmäßig `--progress=plain`, damit Compilerwarnungen vollständig sichtbar bleiben.

### Build 0.3.49

- Batocera: Ein vorhandener, aber ausgeschalteter Bluetooth-Adapter wird jetzt von einem eingeschalteten Adapter unterschieden.
- Der Bluetooth-Bildschirm startet keine `bluetoothctl`-Abfrage mehr, wenn BlueZ den Adapter als `Powered=false` meldet.
- Im Systemmenü wird in diesem Fall `Bluetooth: ausgeschaltet` angezeigt und das Bluetooth-Untermenü nicht geöffnet.
- Scan, Connect und Autoconnect prüfen ebenfalls den tatsächlichen Powered-Zustand. Dadurch kann ein über Batocera deaktivierter Adapter das Programm nicht mehr beim Betreten des Bluetooth-Menüs blockieren.

### Build 0.3.49

- Tastensperre korrigiert: Der tatsächlich verwendete Kandidatenpool in `main.c` enthält jetzt nur **Hoch, Runter, Links, Rechts, A, B, X, Y**.
- Die Anzeigenamen der Entsperrsequenz wurden passend auf `Hoch`, `Runter`, `Links`, `Rechts`, `A`, `B`, `X`, `Y` geändert.
- Der in 0.3.25 irrtümlich zusätzlich eingefügte, aber unbenutzte `unlock_buttons`-Pool wurde entfernt.

### Build 0.3.49

- Batocera erhält einen Software-Fallback für den Display-Timeout.
- Wenn Batocera kein beschreibbares Backlight-/Display-Interface bereitstellt, setzt `Display aus nach` weiterhin den internen Display-Off-Zustand.
- Während dieses Zustands wird der SDL-Framebuffer unmittelbar vor der Ausgabe vollständig schwarz gerendert.
- Die Sonderbehandlung ist mit `#ifdef BUILD_BATOCERA` auf den Batocera-Build begrenzt; DarkOSRE/R36S verwendet weiterhin das vorhandene Hardware-Backlight.
- Ein Tastendruck beendet den schwarzen Zustand wie bisher über die vorhandene Display-Wakeup-Logik.
- Hinweis: Das schaltet die LCD-Hintergrundbeleuchtung ohne Kernel-/Backlight-Schnittstelle nicht elektrisch ab, verhindert aber dauerhaft sichtbare Bildinhalte und reduziert damit statische LCD-Belastung.

### Build 0.3.49

- BlueZ-Akkuabfragen werden nicht mehr ausgeführt, solange `org.bluez` nicht verfügbar ist. Dadurch laufen Batocera und DarkOSRE bei deaktiviertem Bluetooth nicht mehr mit wiederholten `ServiceUnknown`-Akku-Fehlern voll.
- Die Änderung ist absichtlich plattformunabhängig und nicht per `#ifdef` getrennt.
- Die bestehende MPRIS/BlueZ-Fehlermeldung wurde um den erkannten BlueZ-Verfügbarkeitszustand ergänzt, damit der Fehler `-22` auf Batocera und R36S/DarkOSRE vergleichbar diagnostiziert werden kann.
- Netzwerk-/Bluetooth-Zustandslogging bleibt zustandsbasiert und erzeugt keine identischen Meldungen im Sekundentakt.

### Build 0.3.49

- Batocera verwendet für die BlueZ-Medienregistrierung jetzt `org.bluez.Media1.RegisterPlayer` mit dem exportierten Player-Pfad statt `RegisterApplication`.
- Der funktionierende R36S/DarkOSRE-Pfad mit `RegisterApplication` bleibt per `#ifdef BUILD_BATOCERA` unverändert erhalten.
- Batocera exportiert auf dem System-Bus nur den für `RegisterPlayer` benötigten Player statt zusätzlich einen `ObjectManager`-Application-Root anzulegen.
- Fehler von `RegisterPlayer` werden mit Adapter, Rückgabecode sowie D-Bus-Fehlername und -text protokolliert.

### Build 0.3.49

- Die in 0.3.34 eingeführte rekursive Batocera-Library-Sammlung wurde zurückgenommen.
- Das Batocera-Dockerfile verwendet wieder die vorherige direkte `ldd`-Ermittlung der Laufzeitbibliotheken wie in 0.3.33.
- Hintergrund: Die vermisste Bibliothek war bereits eine direkte Abhängigkeit und lediglich beim Kopieren/Prüfen des Pakets übersehen worden.

### Build 0.3.49

- Die Änderung aus 0.3.37 für `KEY_PAUSECD` / EV_KEY 201 wurde vollständig zurückgenommen.
- Media-Key-Verhalten entspricht wieder dem Stand 0.3.36.
- Alle übrigen Änderungen bleiben erhalten.

### Build 0.3.49

- Nur bei `BUILD_BATOCERA` erscheinen unter `FUNK` die Systemschalter `WLAN` und `Bluetooth System`.
- Die Schalter werden auf R36S/DarkOSRE nicht kompiliert und nicht angezeigt.
- WLAN und Bluetooth werden über Batoceras Systemkonfiguration bzw. Service-Helfer geschaltet; der Player speichert dafür keinen eigenen Funkzustand.
- Änderungen werden im Programm-Log protokolliert.

### Build 0.3.49

- Dauerhafte Benutzereinstellungen aus `.hoerspiel_player_state` nach `config.ini` verschoben.
- Neuer Abschnitt `[ui]`: `volume`, `idle_timer_minutes`, `display_timeout_seconds`, `menu_font_size`.
- `[playback]` speichert jetzt auch `shutdown_after_tracks` und `shutdown_at_book_end`.
- `.hoerspiel_player_state` enthält weiterhin Wiedergabefortschritt, letzte Wiedergabe/Buch-ID und Nutzungsstatistik.
- Bestehende `@settings` aus einer alten State-Datei werden einmalig als Migrationsquelle verwendet, falls `[ui]` noch nicht in `config.ini` vorhanden ist.

### Build 0.3.49

- Der R36S-Docker-Build verwendet jetzt wie der GPM2804/Batocera-Build eine eigene Export-Stage.
- `make r36s` erzeugt den Ausgabeordner `dist-r36s/`.
- Das R36S-Binary liegt danach unter `dist-r36s/hoerspiel_player`.
- Der bisherige Umweg über `docker create` und `docker cp` ist für den R36S-Build nicht mehr nötig.
