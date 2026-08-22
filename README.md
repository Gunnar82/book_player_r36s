# book_player_r36s

Hörspiel-Player für den R36S und weitere Linux-Handhelds auf Basis von SDL2, SDL2_mixer und SDL2_ttf.

## Version

**0.3**

### Änderungen 0.3

- Neuer Menüpunkt **Streams** für Online-Radio/Online-Audio.
- Streamlisten werden aus einer lokalen XML-Datei oder über HTTP/HTTPS geladen.
- HTTPS unterstützt optional Client-Zertifikate. Der Zertifikatsmodus ist unter `Einstellungen -> Streams Zertifikat` wählbar: `Keins`, `Downloads` oder `Separat`.
- Im Modus `Downloads` werden die bereits für Downloads konfigurierten CA-/Client-Zertifikate verwendet. `Separat` nutzt eigene Zertifikatspfade aus `[streams]`.
- Die `[streams]`-Optionen werden bei Updates automatisch in einer vorhandenen `config.ini` ergänzt, ohne bestehende Werte zu überschreiben.
- Stream-Favoriten werden anhand der stabilen `stationuuid` gespeichert. Im Streams-Menü gilt: **Y** Favorit setzen/entfernen, **X** zwischen `Alle` und `Favoriten` wechseln.
- Im Streams-Menü entsprechen die hinteren Tasten dem Download-Browser: **L1/R1** seitenweise, **L2/R2** Anfang/Ende.
- Die Stationsliste ist dynamisch und nicht mehr auf 4096 Einträge begrenzt. Sie wächst per `realloc()` entsprechend dem verfügbaren RAM.
- Streams werden über **FFmpeg** als externes Audio-Backend wiedergegeben. Pause/Weiter erfolgt über `SIGSTOP`/`SIGCONT`; soweit vorhanden werden ICY-/`StreamTitle`-Metadaten auf dem Wiedergabebildschirm angezeigt.
- D-Pad Hoch/Runter unterstützt in Menüs und Listen Halte-Wiederholung: nach ca. 400 ms beginnt Auto-Repeat, nach 5 Sekunden wird beschleunigt.
- Der GPM2804-Port ist als **Batocera**-Variante dokumentiert. Das alternative Dockerfile heißt `Dockerfile.gpm2804-batocera`.
- Der Batocera-Starter verwendet `/userdata/roms/ports/Hoerspiel Player`, richtet den DejaVu-Fontpfad ein und stellt `libsystemd.so.0` über `LD_LIBRARY_PATH` bereit.

### Änderungen 0.2.35

- Controllerbelegung ist über den Abschnitt `[input]` in `config.ini` konfigurierbar.
- Ohne Anpassung bleibt die bisherige R36S-Belegung aktiv.
- Im R36S-Profil werden analoge Achsen weiterhin vollständig ignoriert. Dadurch bleibt der Schutz gegen Phantom-Eingaben bei USB-/Ladezustandswechseln erhalten.
- Ein `custom`-Profil kann Buttons frei zuordnen und ein D-Pad über SDL-Achsen auswerten.
- Nicht vorhandene Tasten können mit `-1` deaktiviert werden.

### Änderungen 0.2.34

- Persistente Einstellung `Schriftgroesse` unter `Einstellungen -> AUDIO / DISPLAY`.
- Stufen: `Klein`, `Normal`, `Gross`, `Sehr gross`.
- Hörspiel-, Track-, Download-, System- und Einstellungslisten passen Schriftgröße, Zeilenhöhe und sichtbare Zeilenanzahl an.
- `Normal` entspricht dem bisherigen R36S-Layout. `Gross` und `Sehr gross` eignen sich für kleinere 640×480-Panels wie den Waveshare GPM2804.

### Änderungen 0.2.33

- In Hörspiel-Browser, Trackauswahl, Systemmenü und Download-Browser gilt zusätzlich: D-Pad links = zurück, D-Pad rechts = A/Auswählen.
- Bei aktiver Tastensperre bleibt der Wiedergabebildschirm sichtbar; oben rechts zeigt ein Schloss den gesperrten Zustand an.
- Ein Tastendruck im gesperrten Zustand blendet die Entsperrsequenz ein. Nach 5 Sekunden ohne Eingabe erscheint wieder die Wiedergabe; die Sperre bleibt aktiv.
- Unter Einstellungen gibt es eine persistente Nutzungsstatistik mit App-Starts, gesamter Laufzeit und tatsächlicher Hörzeit.

### Änderungen 0.2.32 / 0.2.31

- Tastendrücke sowie abgeschlossene oder abgebrochene Downloads setzen einen aktiven Idle-Timer auf seinen vollständigen Ausgangswert zurück.
- Während eines Downloads läuft der Idle-Timer nicht ab.
- Lokal vorhandene Dateien werden übersprungen, wenn ihre Größe exakt der Servergröße entspricht.
- Bei abweichender oder unbekannter Größe wird sicher über `.part` geladen und erst nach Erfolg ersetzt.
- Beim Zurückgehen im Download-Browser bleibt der zuvor geöffnete Ordner markiert.

## Steuerung

Die Standardbelegung für den R36S ist:

```ini
[input]
profile=r36s
dpad_mode=buttons
dpad_up=8
dpad_down=9
dpad_left=10
dpad_right=11
dpad_x_axis=0
dpad_y_axis=1
dpad_deadzone=16000

a=1
b=0
x=2
y=3
l1=4
r1=5
l2=6
r2=7
start=13
select=12
```

Ohne `[input]`-Abschnitt gelten dieselben R36S-Standardwerte.

### Waveshare GPM2804 / Batocera

Beim getesteten Microntek-USB-Joystick wird das D-Pad unter Batocera als SDL-Achse geliefert:

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

Ermittelte Joystick-Belegung: A=2, B=1, X=3, Y=0, L1=4, Select=8, Start=9. R1 lieferte auf dem getesteten Gerät kein Event; L2/R2 sind dort nicht vorhanden.

## Menünavigation

In Menüs und Listen kann D-Pad **Hoch/Runter** gehalten werden. Nach ungefähr 400 ms startet die Wiederholung. Nach 5 Sekunden Halten wird schneller gescrollt. Wiedergabebildschirm und Button-Debug sind davon ausgenommen.

## Schriftgröße

Unter `Einstellungen -> AUDIO / DISPLAY -> Schriftgroesse` stehen vier Stufen zur Verfügung:

- `Klein` = 18 px
- `Normal` = 20 px
- `Gross` = 26 px
- `Sehr gross` = 32 px

Die Einstellung wird persistent gespeichert.

## Downloads

Downloads stammen aus nginx-XML-Listings über HTTP/HTTPS, optional mit eigener CA und mTLS. Bereits vollständig vorhandene Dateien werden anhand identischer Dateigröße übersprungen. Downloads laufen zuerst in eine `.part`-Datei; erst nach erfolgreichem Abschluss ersetzt diese die Zieldatei.

## Streams

Die Streamliste wird über `[streams]` in `config.ini` konfiguriert. `xml_url` darf ein lokaler Dateipfad oder eine HTTP/HTTPS-Adresse sein.

```ini
[streams]
# lokal:
xml_url=/roms/ports/Hoerspiel Player/stations.xml

# alternativ online:
# xml_url=https://example.org/stations.xml

# none | downloads | separate
client_cert_mode=none
ca_cert=
client_cert=
client_key=
client_key_password=
```

Die eigentliche Stationsdatei wird **nicht** mit dem Projekt ausgeliefert.

### Streams-XML: Syntax

Die XML verwendet `<station ...>`-Elemente mit Attributen. Beispiel:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<stations>
  <station
    stationuuid="eindeutige-stabile-id"
    name="Beispiel Sender"
    url="https://example.org/stream.mp3"
    url_resolved="https://cdn.example.org/stream.mp3"
    codec="MP3"
    bitrate="128"
    hls="0"
    favicon=""
    tags="beispiel">
  </station>
</stations>
```

Wichtig:

- `stationuuid` muss pro Sender eindeutig und dauerhaft stabil sein. Der Player verwendet sie für Favoriten.
- `name` ist der angezeigte Sendername.
- Für die Wiedergabe wird bevorzugt `url_resolved` verwendet; wenn es fehlt, fällt der Player auf `url` zurück.
- `codec`, `tags` und `favicon` werden eingelesen. Weitere Attribute können in der XML vorhanden sein.
- XML-Entities wie `&amp;`, `&quot;`, `&apos;`, `&lt;` und `&gt;` werden dekodiert.
- Sehr große Listen werden dynamisch eingelesen; es gibt keine feste 4096-Sender-Grenze mehr.

### Streams-Steuerung

- `A` / D-Pad rechts: Stream starten
- `B` / D-Pad links: zurück
- `Y`: Favorit setzen/entfernen
- `X`: Alle/Favoriten umschalten
- `L1` / `R1`: eine Seite zurück/vor
- `L2` / `R2`: Anfang/Ende der Liste

Auf dem Wiedergabebildschirm steuern `A`/`START` Pause/Weiter und `B` beendet den Stream.

### FFmpeg-Backend

Streams verwenden `ffmpeg` als Audio-Backend. Voraussetzung auf dem Zielsystem:

```bash
which ffmpeg
ffmpeg -version
ffmpeg -devices | grep -i alsa
```

FFmpeg wird Audio-only gestartet und gibt über ALSA `default` aus. Netzwerkstreams verwenden Reconnect-Optionen. ICY-/`StreamTitle`-Metadaten werden aus der FFmpeg-Ausgabe gelesen, soweit der Sender sie liefert.

## Bluetooth, HFP und PBAP

Die Navi-Integration besteht aus Player-Logik und Systempatches:

```text
Hörspielbibliothek
  -> vCard: Hörspielname + numerische ID
  -> BlueZ obexd / PBAP
  -> Navi-Telefonbuch
  -> Navi wählt die ID
  -> HFP: ATD<ID>;
  -> gepatchtes PulseAudio libbluez5-util.so
  -> hoerspiel-player-hfp.sock
  -> Player startet das Hörspiel
```

Die Patch-Dokumentation liegt im Verzeichnis `pulse_patch/`. Ohne die Systempatches funktioniert die normale lokale Wiedergabe weiterhin; PBAP-/HFP-Sonderfunktionen können jedoch fehlen.

## R36S ARM64-Build

```bash
docker build --no-cache --platform linux/arm64 -t hoerspiel-player-r36s .
```

Binary herauskopieren:

```bash
docker create --name hoerspiel-r36s hoerspiel-player-r36s /bin/true
docker cp hoerspiel-r36s:/build/hoerspiel_player ./hoerspiel_player
docker rm hoerspiel-r36s
```

## GPM2804 / Batocera-Build

Das separate `Dockerfile.gpm2804-batocera` erzeugt ein portables ARM64-Paket. Für die Ausgabe ist die `export`-Stage vorgesehen:

```bash
rm -rf ./dist-batocera

docker build \
  --no-cache \
  --platform linux/arm64 \
  -f Dockerfile.gpm2804-batocera \
  --target export \
  --output type=local,dest=./dist-batocera \
  .
```

Die Ausgabe enthält unter anderem:

```text
dist-batocera/
├── hoerspiel_player
├── hoerspiel.sh
├── config.gpm2804.ini
└── lib/
```

Der Starter erwartet den Player unter `/userdata/roms/ports/Hoerspiel Player`, legt den Debian-kompatiblen DejaVu-Fontpfad an und stellt die benötigte `libsystemd.so.0` über `/tmp/hoerspiel-libs` bereit.

## Konfiguration

Die mitgelieferte `config.ini.example` zeigt Storage-, Download-, Bluetooth-, Input- und Streams-Einstellungen. Fehlende `[streams]`-Optionen werden beim Start automatisch ergänzt.

Beispiel für Bluetooth:

```ini
[bluetooth]
autoconnect=1
device=00:11:22:33:44:55
```

Die MAC-Adresse ist absichtlich ein Dummy-Beispiel.

## Diagnose und Tests

`Button Debug` und `Programm-Log` befinden sich unter `Einstellungen -> DIAGNOSE`.

Weitere Hinweise:

- `TEST_0.2.27.md`
- `HFP_TEST.md`
- `PBAP_TEST.md`
- `pulse_patch/README.md`

## Lizenz

Siehe `LICENSE`. Das Projekt enthält eine Einschränkung gegen kommerzielle Nutzung.

## Projekt / Kontakt

- GitHub: `github.com/Gunnar82/book_player_r36s`
- Kontakt: `gunnar_82@hotmail.com`
