# book_player_r36s

Hörspiel-Player für den R36S und weitere Linux-Handhelds auf Basis von SDL2, SDL2_mixer und SDL2_ttf.

## Version

**0.3.14**

## Änderungen 0.3.14

- Streams verwenden `stationuuid` als stabile Sender-ID. Änderungen an Sendername, Stream-URL, Codec oder anderen XML-Feldern verändern Favoriten und Auswahlzustand nicht, solange die UUID gleich bleibt.
- Favoriten werden anhand der UUID gespeichert und im RAM gecacht.
- Beim Wechsel zwischen `Alle` und `Favoriten` sowie nach Rückkehr aus der Stream-Wiedergabe bleibt derselbe Sender anhand seiner UUID ausgewählt, sofern er in der Zielansicht vorhanden ist.
- Unter `Einstellungen -> DOWNLOADS / STREAMS -> Streams` gibt es einen eigenen Streams-Einstellungsbildschirm analog zu Downloads. Er zeigt XML-Quelle, Streaming-Backend und TLS-/Zertifikatskonfiguration.
- Der Zertifikatsmodus `Keins`, `Downloads` oder `Separat` wird im Streams-Einstellungsbildschirm geändert.
- Die Stationsliste ist dynamisch und nicht auf 4096 Einträge begrenzt.
- Stream-Wiedergabe verwendet bevorzugt `mpv`, andernfalls FFmpeg. ICY-/`StreamTitle`-Metadaten werden soweit verfügbar angezeigt.
- Der Stream-Wiedergabebildschirm zeigt rechts einen QR-Code der aktuellen Stream-URL.
- Der Idle-Timer berücksichtigt laufende Stream-Wiedergabe; Eingaben in der Streams-Liste setzen ihn zurück.
- R36S/DarkOSRE und GPM2804/Batocera verwenden getrennte Build-Konfigurationen und TTF-Pfade.

## Build-System

Der Build ist in drei Makefiles getrennt:

```text
Makefile                 Docker-Orchestrierung auf dem Entwicklungsrechner
Makefile.r36s            interner C-Build für R36S / DarkOSRE
Makefile.gpm2804         interner C-Build für GPM2804 / Batocera
```

Die C-Dateien werden einzeln zu `.o`-Dateien kompiliert und anschließend gelinkt. `-Wall -Wextra` bleibt aktiv. Die Docker-Ausgabe verwendet `--progress=plain`, damit Compilerwarnungen vollständig sichtbar sind.

### R36S

```bash
make r36s
```

Das äußere Makefile startet Docker mit `--platform linux/arm64`. Im Docker-Container wird `Makefile.r36s` als internes `Makefile` verwendet.

### GPM2804 / Batocera

```bash
make gpm2804
```

Das Paket wird nach `dist-batocera` exportiert. `Dockerfile.gpm2804-batocera` verwendet intern `Makefile.gpm2804`.

## Stream-Abhängigkeiten

Für die QR-Code-Anzeige wird zur Laufzeit `libqrencode.so.4` benötigt.

Auf R36S/DarkOSRE gilt: Wenn nur `hoerspiel_player` auf das Gerät kopiert wird, muss `libqrencode.so.4` auf dem Gerät installiert sein. Prüfen:

```bash
ldd ./hoerspiel_player | grep qrencode
find /lib /usr/lib -name 'libqrencode.so*' 2>/dev/null
```

Beim GPM2804/Batocera-Paket wird `libqrencode.so.4` im `lib/`-Verzeichnis mit ausgeliefert und durch `hoerspiel.sh` über `/tmp/hoerspiel-libs` eingebunden.

FFmpeg muss für den Fallback verfügbar sein und ALSA unterstützen. `mpv` wird verwendet, wenn es vorhanden und auf dem Gerät lauffähig ist.

## Streams-Konfiguration

```ini
[streams]
xml_url=/roms/ports/Hoerspiel Player/stations.xml
client_cert_mode=none
ca_cert=
client_cert=
client_key=
client_key_password=
```

Die Stationsdatei wird nicht mit dem Projekt ausgeliefert.

### XML-Syntax

```xml
<stations>
  <station
    stationuuid="eindeutige-stabile-id"
    name="Beispiel Sender"
    url="https://example.org/stream.mp3"
    url_resolved="https://cdn.example.org/stream.mp3"
    codec="MP3"
    bitrate="128"
    hls="0" />
</stations>
```

`stationuuid` muss pro Sender eindeutig und dauerhaft stabil sein. `name` und Stream-URLs dürfen sich ändern. Für die Wiedergabe wird bevorzugt `url_resolved` verwendet, andernfalls `url`. XML-Entities wie `&amp;`, `&quot;`, `&apos;`, `&lt;` und `&gt;` werden dekodiert.

## Streams-Steuerung

- `A` / D-Pad rechts: Stream starten
- `B` / D-Pad links: zurück
- `Y`: Favorit setzen/entfernen
- `X`: Alle/Favoriten umschalten
- `L1` / `R1`: seitenweise zurück/vor
- `L2` / `R2`: Anfang/Ende

Auf dem Stream-Wiedergabebildschirm steuern `A`/`START` Pause/Weiter und `B` beendet den Stream bzw. kehrt zur Streams-Liste zurück.

## Konfiguration und Diagnose

Die mitgelieferte `config.ini.example` dokumentiert Storage-, Download-, Bluetooth-, Input- und Streams-Einstellungen. Fehlende `[streams]`-Optionen werden beim Start ergänzt. `Button Debug` und `Programm-Log` befinden sich unter `Einstellungen -> DIAGNOSE`.

## Lizenz

Siehe `LICENSE`. Das Projekt enthält eine Einschränkung gegen kommerzielle Nutzung.

## Projekt / Kontakt

- GitHub: `github.com/Gunnar82/book_player_r36s`
- Kontakt: `gunnar_82@hotmail.com`
