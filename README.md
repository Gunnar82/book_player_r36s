# book_player_r36s

Hörspiel- und Audio-Player für Handhelds auf Basis von SDL2. Das Projekt ist primär für den **R36S** entwickelt und enthält zusätzlich einen **Waveshare GPM2804 / Batocera**-Build.

**Aktueller Stand: 0.3.58**

`main` ist die maßgebliche Quelle für stabile Projektstände. Entwicklungsregeln und die Wiederaufnahme nach Kontextverlust stehen in [PROJECT_WORKFLOW.md](PROJECT_WORKFLOW.md).

## Funktionen

- lokale Hörspiel- und Trackauswahl mit Resume
- mehrere konfigurierbare Speicherpfade und verschachtelte Hörspielordner
- Track- und Gesamtfortschritt
- ID3-Tracktitel mit Dateinamen als Fallback
- App-Lautstärke sowie globale Ausgangslautstärke; bei aktivem Bluetooth-Audio zusätzlich separate Bluetooth-Sink-Lautstärke
- Audio-Systemwerte werden im Hintergrund aktualisiert, damit Menü und Scrolling nicht durch `amixer`/`pactl` blockieren
- Akku- und Ladezeitanzeige
- Sleep-, Idle- und Display-Timer
- Tastensperre und persistente Nutzungsstatistik
- Downloads aus XML-/nginx-Listings, HTTPS und optional mTLS
- rekursive Ordnerdownloads, Mehrfachauswahl, Fortschritt, Downloadrate und Restzeitschätzung
- GPM2804/Batocera: asynchrone Hintergrunddownloads mit Statusanzeige im Wiedergabe-Screen
- Online-Streams aus XML-Quellen und Stream-Favoriten über `stationuuid`
- Stream-Wiedergabe über `mpv`
- MPRIS2, BlueZ/AVRCP, Bluetooth-Hotplug und USB-/Headset-Mediatasten
- HFP-Wählkommandos und PBAP-vCard-Telefonbuch
- integriertes Programm-Log und Button-Debug
- konfigurierbare Controller-Belegung

## Unterstützte Plattformen

### R36S

Hauptziel des Projekts. Der R36S-Build verwendet `BUILD_R36S`, die Standard-Controllerbelegung aus `config.h` und den Debian-DejaVu-Fontpfad. Der System-Shutdown läuft über den bestehenden logind-/D-Bus-Pfad.

Die globale **Ausgangslautstärke** verwendet den ALSA-`Master`-Regler. Auf dem getesteten R36S beeinflusst dieser den internen Lautsprecher, den Kopfhörerausgang und die Bluetooth-Ausgabe.

### Waveshare GPM2804 / Batocera

Der Batocera-Build verwendet `BUILD_BATOCERA`. Der Export enthält Binary, benötigte Laufzeitbibliotheken und einen Starter. Da Batocera auf dem getesteten Gerät kein `busctl/loginctl` bereitstellt, wird Poweroff im Batocera-Build auf `/sbin/shutdown -P -h now` abgebildet. Dieser Pfad ist auf dem GPM2804 getestet.

Auch hier wird die globale Ausgangslautstärke über den ALSA-`Master`-Regler gesteuert. Ist ein Bluetooth-Audio-Sink verfügbar, erscheint in den Einstellungen zusätzlich dessen eigene Lautstärke. Die Statusabfragen für `Master` und Bluetooth-Sink laufen in Hintergrundthreads mit einem Aktualisierungsintervall von drei Sekunden; das UI liest nur die gecachten Werte.

Ab 0.3.54 laufen Downloads im GPM2804/Batocera-Build über einen separaten SDL-Worker. Ein gestarteter Download kann mit `Y` in den Hintergrund geschickt werden; die Wiedergabe bleibt danach bedienbar. Beim erneuten Öffnen von **Downloads** wird ein laufender Job wieder mit aktuellem Fortschritt angezeigt.

## Projektstruktur

Wichtige Dateien und Bereiche:

- `main.c` – Hauptprogramm und Eventloop
- `state.c/.h` – Laufzeit-, Resume- und UI-Zustand
- `audio.c/.h` – lokale Audio-Wiedergabe
- `output_volume.c/.h` – globale ALSA-Master-Lautstärke und asynchron aktualisierter Cache
- `bluetooth_audio_sink.c` – Bluetooth-Sink-Lautstärke und asynchron aktualisierter Sink-Cache
- `scanner.c/.h` – Bibliotheks-/Hörspielscan
- `storage.c/.h` – Speicherpfade und Laden der `config.ini`
- `util.c/.h` – gemeinsame String-/Pfad-Helfer
- `config_update.c/.h` – gemeinsames atomisches Aktualisieren von INI-Sektionen
- `download_config_save.c/.h` – Speichern des Download-Schalters
- `hardware_config_save.c/.h` – Speichern der LED-Hardwareeinstellung
- `playback_runtime.c` – Trennung persistenter Wiedergabeeinstellungen von Session-Shutdownwerten
- `stream_config.c/.h` – Speichern der Stream-Zertifikatseinstellung
- `stream_favorites.c/.h` – Persistenz der Stream-Favoriten
- `download.c/.h` – synchrone Download-/Listinglogik und rekursive Auswahl
- `background_download.c/.h` – threadsicherer asynchroner Downloadjob für GPM2804/Batocera
- `background_download_ui.c` – Hintergrundumschaltung, Statusansicht und kompakte Player-Anzeige
- `streaming.c/.h` – Online-Streams und mpv-Steuerung
- `bluetooth.c/.h` – BlueZ-Erkennung, Bluetooth-Status und Autoconnect
- `mpris_bridge.c/.h` – MPRIS2-/BlueZ-Media-Anbindung
- `media_keys.c/.h` – Linux-evdev-Mediatasten
- `hfp_gateway.c/.h` – HFP-Dial-IPC
- `pbap_phonebook.c/.h` – PBAP-vCards
- `app_shutdown.c` – Batocera-spezifische Poweroff-Anpassung
- `screens/` – Menü-, Player-, Download-, Stream-, Bluetooth- und Diagnoseansichten
- `tests/test_util_config.c` – Regressionstests für Utility- und Config-Writer

## Build

Docker wird für reproduzierbare ARM64-Builds verwendet.

### R36S

```bash
make r36s
```

Das Ergebnis wird nach `dist-r36s/` exportiert.

### GPM2804 / Batocera

```bash
make gpm2804
```

Das Ergebnis wird nach `dist-batocera/` exportiert. Zusätzlich werden `config.gpm2804.ini` und `hoerspiel.sh` erzeugt.

### Utility-/Config-Regressionstest

```bash
make -f Makefile.gpm2804 test-utils
```

Der Test prüft `util_trim()`, sichere String-/Pfadfunktionen und das gemeinsame Aktualisieren von INI-Sektionen.

## Konfiguration

Die Datei `config.ini` liegt normalerweise neben dem Player-Binary. Falls sie fehlt, erzeugt der Player eine Grundkonfiguration. Die lokale `config.ini` wird bewusst **nicht** in Git versioniert, weil sie gerätebezogene Pfade sowie Zertifikats-/Schlüsselparameter enthalten kann. Als Vorlage dient `config.ini.example`.

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

`-1` mit `auto` aktiviert die automatische GPIO-Erkennung. Das Speichern der LED-Konfiguration verwendet ab 0.3.52 den gemeinsamen atomischen Config-Writer.

### Wiedergabe

```ini
[playback]
repeat_book=0
```

**Nur `repeat_book` ("Hörspielende") ist persistent.** `shutdown_after_tracks` und `shutdown_at_book_end` sind bewusst reine Session-Einstellungen und werden beim Programmstart auf Aus/0 zurückgesetzt, auch wenn eine ältere `config.ini` noch entsprechende Werte enthält.

### Audio-Lautstärke

Im Einstellungsmenü werden die Audiopegel getrennt dargestellt:

- **Ausgangslautstärke** – globaler ALSA-`Master`-Pegel
- **Lautstärke** – Lautstärke des Players über SDL_mixer
- **Bluetooth-Lautstärke** – Pegel des aktiven Bluetooth-Audio-Sinks; nur sichtbar, wenn ein entsprechender Sink verfügbar ist

Die Systemwerte werden alle drei Sekunden außerhalb des UI-Threads aktualisiert. Änderungen aktualisieren den Cache direkt, sodass die Anzeige nach einem Tastendruck nicht auf den nächsten Hintergrund-Refresh warten muss.

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

Downloads unterstützen HTTPS und optional Client-Zertifikate. Der An/Aus-Schalter wird über den gemeinsamen Config-Writer gespeichert. Downloads erfolgen über `.part`-Dateien und werden erst nach erfolgreichem Abschluss ersetzt.

Im Fortschrittsfenster werden aktuelle Datei, aktueller Ordner, Datei-/Gesamtfortschritt, Restzeit und Downloadrate angezeigt. Auf dem GPM2804/Batocera kann ein laufender Download mit `Y` in den Hintergrund geschickt werden. `B` bricht den Job kontrolliert ab. Während eines Hintergrunddownloads zeigt der Wiedergabe-Screen kompakt beispielsweise `DL 42% · 3.1 MB/s · 17/63`. Wird das Download-Menü während eines aktiven Jobs erneut geöffnet, erscheint wieder dessen Fortschrittsansicht. Es kann nur ein Downloadjob gleichzeitig laufen.

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

`xml_url` kann eine lokale Datei oder eine HTTP-/HTTPS-Quelle sein. Netzwerkzugriffe sind auf HTTP/HTTPS beschränkt. Dynamische Netzwerk-Allokationen sind gegen unbeschränktes Wachstum abgesichert; die aktuelle Grenze ist für große Radio-Browser-Listen ausgelegt und wurde mit einer etwa 6-MB-XML mit rund 6000 Stationen getestet.

Favoriten werden anhand der `stationuuid` gespeichert. Favoriten- und Zertifikatseinstellungen verwenden ab 0.3.52 den gemeinsamen Config-Writer. Die eigentliche Stream-Wiedergabe läuft über `mpv` und einen lokalen IPC-Socket.

### Controller

Standard für R36S:

```ini
[input]
profile=r36s
dpad_mode=buttons
```

Für andere Geräte kann `profile=custom` mit eigener Button-/Axis-Belegung verwendet werden. Nicht vorhandene Tasten können mit `-1` deaktiviert werden.

## Persistenz und Robustheit

Resume-/Nutzungsdaten werden atomar über temporäre Datei plus `rename()` gespeichert. Der Bibliotheksscan verwaltet Unterverzeichnisse dynamisch und hält große Verzeichnislisten nicht pro Rekursionsebene auf dem Stack.

Ab 0.3.52 steht mit `config_update.c/.h` eine gemeinsame atomische Schreibschicht für INI-Sektionen zur Verfügung. Bluetooth-, Stream-, Favoriten-, Download- und LED-Einstellungen wurden schrittweise darauf umgestellt. Dadurch gibt es weniger voneinander abweichende Datei-Rewrite-Implementierungen.

Der Hintergrunddownload verwendet einen einzelnen Worker-Thread und einen mutex-geschützten Status-Snapshot. SDL-Rendering und Controller-Events bleiben im UI-Thread. Beim Programmende wird ein laufender Worker kontrolliert abgebrochen und eingesammelt, bevor SDL beendet wird.

Die Audio-Systemabfragen verwenden ebenfalls Hintergrundthreads und mutex-geschützte Cachewerte. Dadurch führen die regelmäßigen externen `amixer`-/`pactl`-Aufrufe nicht zu Rucklern beim Scrollen im Einstellungsmenü.

Bluetooth-MAC-Adressen werden vor Shell-basierten `bluetoothctl`-Aufrufen strikt validiert.

## Getesteter Stand 0.3.58

Auf R36S und GPM2804/Batocera wurden während der Entwicklung unter anderem die plattformspezifischen Bluetooth- und Menüanpassungen geprüft. Für die Audioeinstellungen gilt:

- globale Ausgangslautstärke über ALSA `Master`
- getrennte Player-Lautstärke
- zusätzliche Bluetooth-Sink-Lautstärke bei verfügbarem Bluetooth-Audio-Sink
- asynchrone Statusaktualisierung im Drei-Sekunden-Intervall, damit das Einstellungsmenü flüssig bedienbar bleibt

Weitere bestehende Tests umfassen unter anderem Builds, Utility-/Config-Regressionstests, Bluetooth, Streams, Favoriten, Downloads, Hintergrunddownloads und System-Shutdown auf den jeweils unterstützten Plattformen.

## Entwicklungsworkflow

Größere Änderungen erfolgen auf Feature-/Refactoring-Branches. Der Branch wird gebaut und auf Hardware getestet, anschließend per Pull Request nach `main` übernommen. `main` bleibt der stabile Referenzstand.

Die vollständigen Regeln stehen in [PROJECT_WORKFLOW.md](PROJECT_WORKFLOW.md).

## Weitere Dokumentation

- [PROJECT_WORKFLOW.md](PROJECT_WORKFLOW.md) – Git-/Entwicklungsworkflow und Wiederaufnahme nach Kontextverlust
- [HFP_TEST.md](HFP_TEST.md) – HFP-Dial-Test und lokaler Socket-Test
- [PBAP_TEST.md](PBAP_TEST.md) – PBAP-vCard-Test
- `CHANGELOG_*.md` – versionsbezogene Änderungsnotizen
- `RELEASE_*.md` – Release-spezifische Hinweise

## Versionshistorie

Die README beschreibt den **aktuellen** Projektstand und ist kein chronologisches Changelog. Ältere Entwicklungsschritte bleiben über die Git-Historie sowie Changelog-, Release- und Testdateien nachvollziehbar.

Aktuelle Version laut `config.h`: **0.3.58**.

## Lizenz und Kontakt

Siehe [LICENSE](LICENSE).

Repository: `Gunnar82/book_player_r36s`
