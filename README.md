# book_player_r36s

Hörspiel- und Audio-Player für Handhelds auf Basis von SDL2. Das Projekt ist primär für den **R36S** entwickelt und enthält zusätzlich einen **Waveshare GPM2804 / Batocera**-Build.

**Aktueller Stand: 0.3.49**

`main` ist die maßgebliche Quelle für den aktuellen Projektstand. Entwicklungs- und Sicherungsregeln stehen in [PROJECT_WORKFLOW.md](PROJECT_WORKFLOW.md).

## Funktionen

- lokale Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade
- verschachtelte Hörspielordner
- Track- und Gesamtfortschritt
- ID3-Tracktitel mit Dateinamen als Fallback
- Lautstärke-, Akku- und Ladezeitanzeige
- Sleep-, Idle- und Display-Timer
- Tastensperre
- persistente Nutzungsstatistik
- Downloads aus XML-/nginx-Listings
- HTTPS und optional mTLS für Downloads
- rekursive Ordnerdownloads, Mehrfachauswahl, Fortschritt und Restzeitschätzung
- Online-Streams aus XML-Quellen
- Stream-Favoriten über `stationuuid`
- Stream-Wiedergabe über `mpv`
- MPRIS2 über D-Bus
- BlueZ-/AVRCP-Integration
- USB-/Headset-Mediatasten
- Bluetooth-Hotplug-Erkennung
- HFP-Wählkommandos für Hörspielsteuerung
- PBAP-vCard-Telefonbuch für Hörspiele
- integriertes Programm-Log und Button-Debug
- konfigurierbare Controller-Belegung

## Unterstützte Plattformen

### R36S

Hauptziel des Projekts. Der R36S-Build verwendet `BUILD_R36S`, die Standard-Controllerbelegung aus `config.h` und den Debian-DejaVu-Fontpfad.

### Waveshare GPM2804 / Batocera

Zusätzlich existiert ein Batocera-Build mit `BUILD_BATOCERA` und eigener Beispielbelegung für den Controller. Der Export enthält Binary, benötigte Laufzeitbibliotheken und einen Starter.

## Projektstruktur

Wichtige Dateien und Bereiche:

- `main.c` – Hauptprogramm und Eventloop
- `state.c/.h` – globaler Laufzeit- und UI-Zustand
- `audio.c/.h` – lokale Audio-Wiedergabe
- `scanner.c/.h` – Bibliotheks-/Hörspielscan
- `storage.c/.h` – `config.ini`, Speicherpfade und persistente Einstellungen
- `download.c/.h` – Downloadlogik
- `streaming.c/.h` – Online-Streams und mpv-Steuerung
- `bluetooth.c/.h` – BlueZ-Erkennung und Bluetooth-Status
- `mpris_bridge.c/.h` – MPRIS2-/BlueZ-Media-Anbindung
- `media_keys.c/.h` – Linux-evdev-Mediatasten
- `hfp_gateway.c/.h` – HFP-Dial-IPC
- `pbap_phonebook.c/.h` – PBAP-vCards
- `battery.c/.h`, `battery_bluez.c/.h` – Geräte-/Bluetooth-Akku
- `backlight.c/.h` – Display-Hintergrundbeleuchtung
- `input_config.c/.h` – Controllerprofile und Eingaben
- `screens/` – Menü-, Player-, Download-, Stream-, Bluetooth- und Diagnoseansichten
- `Makefile` – Docker-Buildziele
- `Makefile.r36s` – nativer R36S-Build im Container
- `Makefile.gpm2804` – GPM2804/Batocera-Build

## Build

Docker wird für reproduzierbare ARM64-Builds verwendet.

### R36S

```bash
make r36s
```

Das Ergebnis wird nach `dist-r36s/` exportiert. Das Paket enthält das Player-Binary sowie die benötigten dynamischen Laufzeitbibliotheken und den ARM64-Loader.

### GPM2804 / Batocera

```bash
make gpm2804
```

Das Ergebnis wird nach `dist-batocera/` exportiert. Zusätzlich werden eine Beispielkonfiguration `config.gpm2804.ini` und `hoerspiel.sh` erzeugt.

### Aufräumen

```bash
make clean
```

## Laufzeitabhängigkeiten

Der Quellcode verwendet unter anderem:

- SDL2
- SDL2_mixer
- SDL2_ttf
- libcurl
- libsystemd
- libqrencode
- `mpv` für Online-Streams
- BlueZ / D-Bus für Bluetooth-Funktionen

Die Docker-Builds sammeln die für das Player-Binary benötigten dynamischen Bibliotheken in das jeweilige Distributionsverzeichnis ein.

## Konfiguration

Die Datei `config.ini` liegt normalerweise neben dem Player-Binary. Falls sie fehlt, erzeugt der Player eine Grundkonfiguration.

### Speicherpfade

```ini
[storage]
path=/roms/hoerspiele
path=/roms2/hoerspiele
path=/mnt/usbdrive/hoerspiele
```

Es können mehrere `path=`-Einträge vorhanden sein. Nicht verfügbare Pfade werden ignoriert.

### Hardware

```ini
[hardware]
led_gpio=-1
led_gpio_mode=auto
```

`-1` mit `auto` aktiviert die automatische GPIO-Erkennung.

### Wiedergabe

```ini
[playback]
repeat_book=0
```

Weitere Wiedergabeoptionen werden vom Player ebenfalls persistent verwaltet.

### Downloads

```ini
[download]
enabled=0
base_url=
target_path=/roms/hoerspiele
verify_peer=1
verify_host=1
ca_cert=
client_cert=
client_key=
client_key_password=
```

Downloads unterstützen HTTPS und optional Client-Zertifikate. Bestehende Dateien mit identischer bekannter Servergröße können übersprungen werden. Downloads erfolgen über `.part`-Dateien und werden erst nach erfolgreichem Abschluss ersetzt.

### Streams

```ini
[streams]
xml_url=
client_cert_mode=none
ca_cert=
client_cert=
client_key=
client_key_password=
```

`xml_url` kann eine lokale Datei oder eine HTTP-/HTTPS-Quelle sein. Der Streamparser verarbeitet Stationseinträge mit Informationen wie Name, URL, `url_resolved`, Codec, Bitrate, HLS, Favicon, Tags und `stationuuid`.

Favoriten werden anhand der `stationuuid` gespeichert. Die eigentliche Stream-Wiedergabe läuft über `mpv` und einen lokalen IPC-Socket.

### Controller

Standard für R36S:

```ini
[input]
profile=r36s
dpad_mode=buttons
```

Für andere Geräte kann ein Custom-Profil verwendet werden, zum Beispiel:

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

Nicht vorhandene Tasten können mit `-1` deaktiviert werden.

## Bedienung

Die genaue Belegung kann über das Input-Profil abweichen. Im R36S-Standard gelten unter anderem:

- D-Pad Hoch/Runter – Navigation in Listen
- D-Pad Links – in vielen Listen zurück
- D-Pad Rechts – in vielen Listen auswählen
- Wiedergabe: Hoch/Runter – ±15 Sekunden
- Wiedergabe: Links/Rechts – vorheriger/nächster Track
- `SELECT` – Tastensperre
- `MID` / evdev Keycode 708 – Display an/aus
- Schultertasten – Seiten-/Sprungnavigation in Listen

In Menüs unterstützt Hoch/Runter Key-Repeat beim Gedrückthalten.

## Lokale Hörspiele und Resume

Beim Start werden die konfigurierten Speicherorte nach Hörspielen durchsucht. Wiedergabeposition und relevante Zustände werden persistent gespeichert. Nach erneutem Öffnen kann ein Hörspiel an seiner letzten Position fortgesetzt werden.

Wenn ein Bluetooth-Adapter vorhanden ist, können persistente Hörspiel-IDs in der Bibliothek als Suffix angezeigt werden, beispielsweise `Hörspielname [1001]`. Diese IDs werden für HFP/PBAP verwendet.

## Online-Streams

Der Menüpunkt für Online-Inhalte lädt die konfigurierte XML-Quelle. Die Wiedergabe erfolgt über `mpv`. Metadaten wie ICY-Name und ICY-Titel können im Player angezeigt werden.

In der Streamliste:

- `Y` – Favorit setzen/entfernen
- `X` – zwischen allen Stationen und Favoriten wechseln

Beim Start eines Streams wird lokale Wiedergabe pausiert, damit nicht zwei Audioquellen gleichzeitig gegeneinander antreten.

## Bluetooth, AVRCP und MPRIS

Der Player enthält eine MPRIS2-Bridge und BlueZ-Media-Anbindung.

- MPRIS-Service: `org.mpris.MediaPlayer2.HoerspielPlayer`
- BlueZ-Adapter werden zur Laufzeit erkannt
- Bluetooth-Hotplug wird berücksichtigt
- AVRCP-/MPRIS-Kommandos können Play, Pause, Next und Previous steuern
- ein separates `mpris-proxy` ist für die integrierte Player-Brücke nicht vorgesehen

USB-/Headset-Mediatasten werden zusätzlich über Linux evdev verarbeitet. Getrennte Play- und Pause-Tasten bleiben getrennte Aktionen; `KEY_PLAYPAUSE` arbeitet als Toggle.

## HFP und PBAP

### HFP

Der Player lauscht auf einem Unix-Datagram-Socket für gespiegelte HFP-Wählbefehle. Ein Kommando wie `DIAL 1001` kann einer Hörspiel-ID bzw. einer Playeraktion zugeordnet werden.

Die aktuelle Testbeschreibung steht in [HFP_TEST.md](HFP_TEST.md).

### PBAP

Nach dem Bibliotheksscan kann der Player für Hörspiele vCards unter

```text
$HOME/phonebook/telecom/pb/
```

erzeugen. Die persistente Hörspiel-ID wird als Telefonnummer verwendet. Für die vorgesehene BlueZ-PBAP-Nutzung muss `obexd` mit einem passenden Phonebook-Backend betrieben werden.

Details stehen in [PBAP_TEST.md](PBAP_TEST.md).

## Akku, Display und Timer

Der Player liest Akku- und Ladeinformationen aus Linux-Power-Supply-Schnittstellen. Wenn möglich, wird aus Ladestand und Strom-/Energiewerten eine Laufzeit bzw. Ladezeit bis 100 % geschätzt. Fehlende Kernelwerte werden durch Fallback-Schätzungen abgefangen.

Unterstützt werden außerdem:

- Display-Inaktivitätstimer
- Idle-Timer
- Sleeptimer
- persistente Lautstärke und UI-Einstellungen
- einstellbare Menüschriftgröße
- Tastensperre
- optionale LED-Steuerung

Während aktiver Downloads wird der Idle-Timer entsprechend behandelt, damit ein längerer Download nicht versehentlich als Benutzerinaktivität gewertet wird.

## Diagnose

Das Projekt enthält Diagnosefunktionen für die Entwicklung auf realer Hardware:

- integriertes Programm-Log
- Systeminformationen
- Button-Debug
- Anzeige unbekannter evdev-Keycodes bei geeigneten Media-Geräten
- Bluetooth-/MPRIS-Statusmeldungen

Diese Funktionen sind insbesondere beim Portieren auf neue Controller und bei BlueZ-/Media-Problemen hilfreich.

## Entwicklungsworkflow

Für dieses Projekt gilt:

1. `main` ist der führende Stand.
2. Vor Arbeitsbeginn `git pull --ff-only origin main`.
3. Funktionierende Zwischenstände regelmäßig committen und pushen.
4. Größere/riskante Änderungen auf Feature-Branches durchführen.
5. ZIP-Dateien nur als Backup verwenden, nicht als primäre Versionsverwaltung.
6. Stabile Versionen nach Möglichkeit taggen.

Die vollständigen Regeln und Wiederherstellungsschritte stehen in [PROJECT_WORKFLOW.md](PROJECT_WORKFLOW.md).

## Weitere Dokumentation

- [PROJECT_WORKFLOW.md](PROJECT_WORKFLOW.md) – Git-/Entwicklungsworkflow und Wiederaufnahme nach Kontextverlust
- [HFP_TEST.md](HFP_TEST.md) – HFP-Dial-Test und lokaler Socket-Test
- [PBAP_TEST.md](PBAP_TEST.md) – PBAP-vCard-Test
- [TEST_0.2.27.md](TEST_0.2.27.md) – ältere Hardware-/Regressionstests
- `CHANGELOG_*.md` – versionsbezogene Änderungsnotizen
- `RELEASE_*.md` – Release-spezifische Hinweise

## Versionshistorie

Die README beschreibt bewusst den **aktuellen** Projektstand und ist kein chronologisches Changelog. Ältere Entwicklungsschritte bleiben über Git-Historie, Changelog-, Release- und Testdateien nachvollziehbar.

Aktuelle Version laut `config.h`: **0.3.49**.

## Lizenz und Kontakt

Siehe [LICENSE](LICENSE).

Repository: `Gunnar82/book_player_r36s`
