# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2_ttf.

## Version

**0.2.27**

### Änderungen 0.2.27

- Analoge Joystick-Achsen werden global ignoriert. Die Bedienung erfolgt ausschließlich über D-Pad und Buttons. Dadurch lösen USB-/Ladezustandswechsel keine Phantom-Achsenbewegungen und damit keine Track- oder 15-Sekunden-Sprünge mehr aus.
- Ist kein Bluetooth-Adapter vorhanden, werden BlueZ-BatteryProvider, HFP-IPC und BlueZ/MPRIS nicht gestartet oder periodisch abgefragt. Der Player bleibt vollständig lokal nutzbar und erzeugt kein fortlaufendes BlueZ-Fehlerspam.
- Bluetooth-Hotplug wird während der Laufzeit erkannt. Beim Einstecken werden die Bluetooth-Funktionen initialisiert, beim Abziehen wieder sauber beendet.
- In den Einstellungen wird Bluetooth ohne Adapter als `nicht verfuegbar` angezeigt.
- Beim Zurückgehen im Hörspiel-Browser bleibt der zuvor betretene Ordner markiert.
- `Button Debug` und `Programm-Log` befinden sich jetzt ganz unten unter `Einstellungen -> DIAGNOSE`.
- Während des Ladens wird, sofern die Kernelwerte eine belastbare Schätzung erlauben, die Restzeit bis zum vollen Akku angezeigt.
- Die HFP/PBAP-Hörspielwahl verwendet persistente numerische Hörspiel-IDs. Der Player erzeugt die zugehörigen vCards automatisch.

## Funktionen

- Hörspiel- und Trackauswahl mit Resume und verschachtelten Hörspielordnern
- persistente Hörspiel-ID für HFP/PBAP
- automatisches PBAP-vCard-Telefonbuch unter `$HOME/phonebook/telecom/pb/`
- HFP-Wahl eines Telefonbucheintrags startet das zugehörige Hörspiel
- Track- und Gesamtfortschritt mit Fortschrittsbalken
- ID3-Titelanzeige mit Dateinamen-Fallback
- Akkuanzeige, Restlaufzeit und Lade-Restzeit bis voll
- Lautstärke-, Helligkeits-, Sleep-, Idle- und Display-Timer
- MPRIS2/BlueZ-Media-Integration und Bluetooth-Autoconnect
- Bluetooth-Hotplug und sauberer Betrieb ohne Bluetooth-Adapter
- Downloads aus nginx-XML-Listings mit HTTPS/mTLS
- USB-/Headset-Mediatasten
- Programm-Log und Button-Debug unter `Einstellungen -> DIAGNOSE`
- Herunterfahren über systemd-logind/D-Bus

## Steuerung

- `Y`: Hörspielauswahl
- `X`: Systemmenü
- `A`: Auswahl bzw. Play/Pause
- `B`: zurück
- `SELECT`: Tastensperre
- Wiedergabe D-Pad Hoch/Runter: +15/-15 Sekunden
- Wiedergabe D-Pad Links/Rechts: Track zurück/weiter
- `L1`/`R1`: seitenweise navigieren
- `L2`/`R2`: Anfang/Ende

Analoge Joystick-Achsen werden seit 0.2.26/0.2.27 bewusst nicht mehr ausgewertet.

## Bluetooth, HFP und PBAP

Die Navi-Integration besteht aus zwei Systempatches plus Player-Logik:

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

Die Systempatches für PulseAudio 17 und BlueZ 5.82 sind ausführlich in [`pulse_patch/SYSTEM_PATCHES.md`](pulse_patch/SYSTEM_PATCHES.md) dokumentiert. Der PulseAudio-Quellpatch liegt unter `pulse_patch/patch_backend_native.py`.

Der Player erzeugt nach dem Bibliotheksscan vCards in:

```text
$HOME/phonebook/telecom/pb/
```

Auf dem R36S mit Benutzer `ark` also normalerweise:

```text
/home/ark/phonebook/telecom/pb/
```

Fehlt ein Bluetooth-Adapter, werden Bluetooth-spezifische Funktionen deaktiviert. Der Adapterzustand wird während der Laufzeit geprüft; beim späteren Einstecken werden BatteryProvider, HFP-IPC, MPRIS/BlueZ und Autoconnect wieder initialisiert.

## Akku

Der Player liest die Batterie über `/sys/class/power_supply`. Beim Entladen wird die Restlaufzeit bevorzugt aus Energie-/Leistungs- bzw. Ladungs-/Stromwerten geschätzt. Beim Laden wird entsprechend die Restzeit bis 100 % berechnet. Falls direkte Kernelwerte fehlen, kann die tatsächliche Prozentänderung als Fallback dienen.

## Einstellungen

Zu den Einstellungen gehören unter anderem:

- Sleeptimer
- Idle-Timer
- Shutdown nach Tracks / am Hörspielende
- Download-Einstellungen
- Bluetooth
- Lautstärke und Helligkeit
- Display-Inaktivität
- LED GPIO / LED-Test
- Wiederholverhalten
- System-, Leistungs-, Akku- und Speicherinformationen
- ganz unten: `DIAGNOSE` mit `Button Debug` und `Programm-Log`

## Konfiguration

`config.ini` liegt neben `hoerspiel_player`. Mehrere Speicherpfade sind möglich:

```ini
[storage]
path=/roms/hoerspiele
path=/roms2/hoerspiele

[bluetooth]
autoconnect=1
device=00:11:22:33:44:55
```

## ARM64-Build

DarkOS läuft auf AArch64. Beispiel:

```bash
docker buildx build \
  --platform linux/arm64 \
  --output type=local,dest=./out \
  .
```

Der Docker-Build prüft `APP_VERSION "0.2.27"` und baut unter anderem die integrierten HFP-/PBAP-Module mit.

## Tests

Zusätzliche Testhinweise befinden sich in:

- `TEST_0.2.27.md`
- `HFP_TEST.md`
- `PBAP_TEST.md`
- `pulse_patch/SYSTEM_PATCHES.md`

## Projekt / Kontakt

- GitHub: `https://github.com/Gunnar82/book_player_r36s`
- E-Mail: `gunnar_82@hotmail.com`
