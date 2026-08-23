# Release 0.3.4

Dieser Stand bündelt die aktuellen Streaming-Anpassungen für R36S/DarkOSRE und GPM2804/Batocera.

## Enthalten

- Stream-Wiedergabe mit bevorzugtem mpv-Backend und FFmpeg-Fallback.
- ICY-Metadaten auf dem Stream-Wiedergabescreen, inklusive Sendername, StreamTitle, Backend, Bitrate, Samplerate, Kanalzahl und Beschreibung, soweit verfügbar.
- QR-Code der aktuellen Stream-URL auf der rechten Seite des Wiedergabescreens.
- Favoriten in `config.ini` mit RAM-Cache für flüssige Favoritenansicht.
- Beibehaltung der ausgewählten Station beim Wechsel zwischen `Alle` und `Favoriten` sowie bei Rückkehr aus der Stream-Wiedergabe.
- Dynamische Stationsliste ohne feste 4096-Eintragsgrenze.
- Idle-Timer berücksichtigt laufende und pausierte Stream-Wiedergabe.
- L1/R1-Haltewiederholung im Streams-Menü.
- Make-Ziele `make r36s` und `make gpm2804`.
- Plattformabhängige TTF-Pfade über `BUILD_R36S` und `BUILD_BATOCERA`.
- Batocera-Starter lädt `libsystemd.so.0` und `libqrencode.so.4` gezielt über `/tmp/hoerspiel-libs`.
- QR-Code beginnt auf dem Stream-Wiedergabescreen bei Y=160.

## Build

```bash
make r36s
make gpm2804
```

Die Zielarchitektur wird über `docker build --platform linux/arm64` gesetzt; die Dockerfiles enthalten keine feste `FROM --platform=...`-Angabe mehr.
