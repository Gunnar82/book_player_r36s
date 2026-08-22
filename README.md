# book_player_r36s

Hörspiel-Player für den R36S und weitere Linux-Handhelds auf Basis von SDL2, SDL2_mixer und SDL2_ttf.

## Version

**0.2.35-input-config**

### Änderungen 0.2.35

- Controllerbelegung ist über den Abschnitt `[input]` in `config.ini` konfigurierbar.
- Ohne Anpassung bleibt die bisherige R36S-Belegung aktiv.
- Im R36S-Profil werden analoge Achsen weiterhin vollständig ignoriert. Dadurch bleibt der Schutz gegen Phantom-Eingaben bei USB-/Ladezustandswechseln erhalten.
- Ein `custom`-Profil kann Buttons frei zuordnen und ein D-Pad über SDL-Achsen auswerten.
- Nicht vorhandene Tasten können mit `-1` deaktiviert werden.
- Getestetes Profil für Waveshare GPM2804 unter Batocera/Recalbox ist dokumentiert.
- Alternatives ARM64-Dockerfile `Dockerfile.gpm2804-recalbox` erzeugt ein portables Runtime-Paket mit Shared Libraries, ARM64-Loader, Starter-Script und Beispielkonfiguration.

### Änderungen 0.2.34

- Neue persistente Einstellung `Schriftgroesse` unter `Einstellungen -> AUDIO / DISPLAY`.
- Stufen: `Klein`, `Normal`, `Gross`, `Sehr gross`.
- Hörspiel-, Track-, Download-, System- und Einstellungslisten passen Schriftgröße, Zeilenhöhe und sichtbare Zeilenanzahl an.
- `Normal` entspricht dem bisherigen R36S-Layout. `Gross` und `Sehr gross` eignen sich für kleinere 640×480-Panels wie den Waveshare GPM2804.
- Der Wiedergabebildschirm bleibt bewusst unverändert, damit Fortschritts-, Akku- und Statuslayout stabil bleiben.

### Änderungen 0.2.33

- In Hörspiel-Browser, Trackauswahl, Systemmenü und Download-Browser gilt zusätzlich: D-Pad links = zurück, D-Pad rechts = A/Auswählen. Im Wiedergabebildschirm bleibt links/rechts Track zurück/weiter; in den Einstellungen bleibt links/rechts für Wertänderungen reserviert.
- Bei aktiver Tastensperre bleibt der Wiedergabebildschirm sichtbar. Oben rechts zeigt ein kleines grafisches Schloss den gesperrten Zustand an.
- Ein Tastendruck im gesperrten Zustand blendet die Entsperrsequenz ein. Jeder weitere Tastendruck startet die 5-Sekunden-Anzeige erneut. Nach 5 Sekunden ohne Eingabe erscheint wieder die Wiedergabe; die Sperre bleibt aktiv.
- Unter Einstellungen gibt es eine persistente Nutzungsstatistik mit App-Starts, gesamter Laufzeit und tatsächlicher Hörzeit.

### Änderungen 0.2.32 / 0.2.31

- Tastendrücke sowie abgeschlossene oder abgebrochene Downloads setzen einen aktiven Idle-Timer auf seinen vollständigen Ausgangswert zurück.
- Während des Downloads läuft der Idle-Timer nicht ab.
- Lokal vorhandene Dateien werden übersprungen, wenn ihre Größe exakt der Servergröße entspricht.
- Bei abweichender oder unbekannter Größe wird weiterhin sicher über `.part` geladen und erst nach Erfolg ersetzt.
- Beim Zurückgehen im Download-Browser bleibt der zuvor geöffnete Ordner markiert.

### Änderungen 0.2.29

- Hörspiel-IDs werden nur bei vorhandenem Bluetooth-Adapter sichtbar und als Suffix angezeigt, z. B. `Hörspielname [1001]`.
- Bluetooth-Hotplug aktualisiert die sichtbaren IDs ohne Player-Neustart.
- Die persistente ID bleibt unabhängig von der Anzeige für HFP/PBAP erhalten.

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

Beim getesteten Microntek-USB-Joystick wird das D-Pad unter Batocera als SDL-Achse geliefert. Das passende Profil ist:

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

## Schriftgröße

Unter `Einstellungen -> AUDIO / DISPLAY -> Schriftgroesse` stehen vier Stufen zur Verfügung:

- `Klein` = 18 px
- `Normal` = 20 px
- `Gross` = 26 px
- `Sehr gross` = 32 px

Die Einstellung wird persistent in der Statusdatei gespeichert.

## Downloads

Downloads stammen aus nginx-XML-Listings über HTTP/HTTPS, optional mit eigener CA und mTLS. Bereits vollständig vorhandene Dateien werden anhand identischer Dateigröße übersprungen. Downloads laufen zuerst in eine `.part`-Datei. Erst nach erfolgreichem Abschluss ersetzt diese die Zieldatei.

## Bluetooth, HFP und PBAP

Die Navi-Integration besteht aus Player-Logik und zwei Systempatches:

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

Die vollständige Anleitung für PulseAudio 17 und BlueZ 5.82 liegt in [`pulse_patch/SYSTEM_PATCHES.md`](pulse_patch/SYSTEM_PATCHES.md). Der PulseAudio-Quellpatch liegt unter `pulse_patch/patch_backend_native.py`.

### BlueZ/OBEX-PBAP

Das serienmäßige DarkOSRE-`obexd` verwendet `phonebook-ebook.c`. Für das vom Player erzeugte dateibasierte Telefonbuch wird BlueZ 5.82 mit `--with-phonebook=dummy` benötigt.

Die Patch-Dokumentation beschreibt unter anderem:

- ARM64-Build von BlueZ 5.82 mit `--enable-obex --with-phonebook=dummy`
- Runtime-Paket `libical3t64`
- Sicherung und Austausch von `/usr/libexec/bluetooth/obexd`
- Prüfung auf `obexd/plugins/phonebook-dummy.c`
- Telefonbuchpfad `/home/ark/phonebook/telecom/pb/`
- Boot-Service `/etc/systemd/system/obex-pbap.service` mit User-D-Bus-Anbindung
- Wiederherstellung des originalen `obexd`

Ohne diesen Patch funktioniert die normale lokale Wiedergabe weiterhin, das erzeugte vCard-Telefonbuch wird jedoch nicht über PBAP an das Navi geliefert.

Fehlt ein Bluetooth-Adapter, werden Bluetooth-spezifische Funktionen deaktiviert. Hotplug wird während der Laufzeit erkannt.

## Standard-ARM64-Build

```bash
docker buildx build \
  --platform linux/arm64 \
  --output type=local,dest=./out \
  .
```

## Alternatives GPM2804/Recalbox/Batocera-Dockerfile

Zusätzlich liegt `Dockerfile.gpm2804-recalbox` bei. Es baut ein portables ARM64-Paket inklusive benötigter dynamischer Bibliotheken und `/lib/ld-linux-aarch64.so.1`. Der Starter kopiert die Runtime von einem nicht ausführbaren SHARE-Dateisystem nach `/tmp/hoerspiel-player` und startet den Player dort mit eigenem Library-Pfad.

```bash
docker buildx build \
  --platform linux/arm64 \
  -f Dockerfile.gpm2804-recalbox \
  --output type=local,dest=./out-gpm2804 \
  .
```

Die Ausgabe enthält zusätzlich `config.gpm2804.ini` mit der getesteten Controllerbelegung.

## Konfiguration

Beispiel:

```ini
[storage]
path=/roms/hoerspiele
path=/roms2/hoerspiele

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
- `pulse_patch/SYSTEM_PATCHES.md`

## Lizenz

Siehe `LICENSE`. Das Projekt enthält eine Einschränkung gegen kommerzielle Nutzung.

## Projekt / Kontakt

- GitHub: `github.com/Gunnar82/book_player_r36s`
- Kontakt: `gunnar_82@hotmail.com`
