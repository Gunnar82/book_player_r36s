# book_player_r36s

Hörspiel-Player für den R36S und weitere Linux-Handhelds auf Basis von SDL2, SDL2_mixer und SDL2_ttf.

## Version

**0.3.4**

### Änderungen 0.3.4

- Streams unterstützen mpv mit FFmpeg-Fallback und zeigen ICY-Metadaten auf dem Wiedergabebildschirm an.
- Der Stream-Wiedergabebildschirm zeigt rechts einen QR-Code der aktuellen Stream-URL.
- Stream-Favoriten werden im RAM gecacht; die Auswahl bleibt beim Wechsel zwischen `Alle` und `Favoriten` sowie nach Rückkehr aus der Wiedergabe erhalten.
- Die Stationsliste ist dynamisch und nicht auf 4096 Einträge begrenzt.
- Idle-Timer und Tastenwiederholung berücksichtigen die Stream-Wiedergabe.
- R36S/DarkOSRE und GPM2804/Batocera verwenden beim Build getrennte TTF-Pfade.
- `make r36s` und `make gpm2804` bauen die jeweiligen ARM64-Ziele.
- Für den QR-Code wird zur Laufzeit `libqrencode.so.4` benötigt. Auf R36S/DarkOSRE muss diese Bibliothek installiert sein, wenn nur `hoerspiel_player` auf das Gerät kopiert wird. Beim Batocera-Paket wird sie im `lib/`-Verzeichnis mit ausgeliefert und vom Starter eingebunden.

### Änderungen 0.2.35

- Controllerbelegung ist über den Abschnitt `[input]` in `config.ini` konfigurierbar.
- Ohne Anpassung bleibt die bisherige R36S-Belegung aktiv.
- Im R36S-Profil werden analoge Achsen weiterhin vollständig ignoriert. Dadurch bleibt der Schutz gegen Phantom-Eingaben bei USB-/Ladezustandswechseln erhalten.
- Ein `custom`-Profil kann Buttons frei zuordnen und ein D-Pad über SDL-Achsen auswerten.
- Nicht vorhandene Tasten können mit `-1` deaktiviert werden.

## Steuerung

Die Standardbelegung für den R36S ist in `config.ini.example` dokumentiert. Im Streams-Menü gilt zusätzlich: `A`/rechts startet, `B`/links geht zurück, `Y` setzt oder entfernt einen Favoriten, `X` wechselt zwischen Alle/Favoriten, `L1/R1` blättert seitenweise und `L2/R2` springt an Anfang/Ende.

## Streams

Die Streamliste wird über `[streams]` in `config.ini` konfiguriert. `xml_url` darf ein lokaler Dateipfad oder eine HTTP/HTTPS-Adresse sein. Die eigentliche Stationsdatei wird nicht mit dem Projekt ausgeliefert.

```ini
[streams]
xml_url=/roms/ports/Hoerspiel Player/stations.xml
client_cert_mode=none
ca_cert=
client_cert=
client_key=
client_key_password=
```

Die XML verwendet `<station ...>`-Elemente. `stationuuid` muss eindeutig und stabil sein, weil sie für Favoriten verwendet wird. Für die Wiedergabe wird bevorzugt `url_resolved` verwendet, andernfalls `url`. XML-Entities wie `&amp;`, `&quot;`, `&apos;`, `&lt;` und `&gt;` werden dekodiert.

### Stream-Wiedergabe und Abhängigkeiten

Streams verwenden bevorzugt `mpv`, sofern es auf dem Zielsystem verfügbar und lauffähig ist; andernfalls wird FFmpeg verwendet. FFmpeg muss ALSA-Ausgabe unterstützen. ICY-/`StreamTitle`-Metadaten werden soweit verfügbar angezeigt.

Der QR-Code wird mit `libqrencode` erzeugt. Dadurch benötigt das Binary zur Laufzeit **`libqrencode.so.4`**.

Auf dem R36S/DarkOSRE ist das besonders wichtig: Wer wie üblich **nur die Datei `hoerspiel_player` auf das Gerät kopiert**, muss sicherstellen, dass `libqrencode.so.4` bereits im System installiert ist. Andernfalls beendet der Loader den Start mit einer Meldung wie:

```text
error while loading shared libraries: libqrencode.so.4: cannot open shared object file
```

Prüfen lässt sich das mit:

```bash
ldd ./hoerspiel_player | grep qrencode
find /lib /usr/lib -name 'libqrencode.so*' 2>/dev/null
```

Fehlt die Bibliothek, muss das zur DarkOSRE-Distribution passende `libqrencode`-Paket installiert werden. Nicht einfach eine beliebige Bibliothek eines anderen Linux-Systems kopieren, da Architektur und ABI zum ARM64-System passen müssen.

Beim **GPM2804/Batocera-Paket** wird `libqrencode.so.4` dagegen unter `lib/` mit ausgeliefert. `hoerspiel.sh` stellt sie zusammen mit den zusätzlich benötigten Laufzeitbibliotheken über `/tmp/hoerspiel-libs` bereit.

## R36S ARM64-Build

Empfohlen:

```bash
make r36s
```

Alternativ kann das Dockerfile direkt mit `docker build --platform linux/arm64` gebaut werden. Wenn anschließend nur `hoerspiel_player` auf den R36S kopiert wird, gilt weiterhin die oben beschriebene Voraussetzung: `libqrencode.so.4` muss auf DarkOSRE installiert sein.

## GPM2804 / Batocera-Build

Empfohlen:

```bash
make gpm2804
```

Das separate `Dockerfile.gpm2804-batocera` erzeugt ein ARM64-Paket in `dist-batocera`. Die Ausgabe enthält den Player, `hoerspiel.sh`, die GPM2804-Konfiguration und das benötigte `lib/`-Verzeichnis. Das komplette Paket muss nach `/userdata/roms/ports/Hoerspiel Player` kopiert werden.

## Konfiguration

Die mitgelieferte `config.ini.example` zeigt Storage-, Download-, Bluetooth-, Input- und Streams-Einstellungen. Fehlende `[streams]`-Optionen werden beim Start automatisch ergänzt.

## Diagnose und Tests

`Button Debug` und `Programm-Log` befinden sich unter `Einstellungen -> DIAGNOSE`. Weitere technische Hinweise befinden sich in den Test- und Patch-Dokumenten des Repositorys sowie in `RELEASE_0.3.4.md`.

## Lizenz

Siehe `LICENSE`. Das Projekt enthält eine Einschränkung gegen kommerzielle Nutzung.

## Projekt / Kontakt

- GitHub: `github.com/Gunnar82/book_player_r36s`
- Kontakt: `gunnar_82@hotmail.com`
