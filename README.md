# book_player_r36s

Hörspiel-Player für den R36S auf Basis von SDL2/SDL2_mixer/SDL2_ttf.

## Version

**0.2.29-bt-id-hotplug**

### Änderungen 0.2.29

- Hörspiel-IDs werden nur angezeigt, wenn ein Bluetooth-Adapter vorhanden ist.
- Die ID steht als Suffix hinter dem Hörspielnamen, z. B. `Hörspielname [1001]`, nicht mehr als Präfix.
- Ohne Bluetooth-Adapter wird nur der Hörspielname angezeigt; die persistente ID bleibt intern für HFP/PBAP erhalten.
- Bluetooth-Hotplug aktualisiert die Hörspielanzeige während der Laufzeit. Beim Ein- oder Ausstecken des Adapters werden die sichtbaren IDs ohne Player-Neustart ein- bzw. ausgeblendet.
- Die aktuelle Auswahl und Browserposition bleiben beim Aktualisieren der Anzeige erhalten.

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
- persistente Hörspiel-ID für HFP/PBAP; sichtbar als Suffix nur bei vorhandenem Bluetooth-Adapter
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

Analoge Joystick-Achsen werden bewusst nicht mehr ausgewertet.

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

### Wichtiger Hinweis zum BlueZ/OBEX-PBAP-Patch

Das serienmäßige `obexd` von DarkOSRE verwendet das Evolution-Data-Server-Backend (`phonebook-ebook.c`). Für das vom Player erzeugte dateibasierte Telefonbuch wird stattdessen ein **BlueZ 5.82 `obexd` mit `--with-phonebook=dummy`** benötigt.

Die vollständige Anleitung in `pulse_patch/SYSTEM_PATCHES.md` beschreibt:

- den ARM64-Build von BlueZ 5.82 mit `--enable-obex --with-phonebook=dummy`,
- die zusätzlich benötigte Runtime `libical3t64`,
- Sicherung und Austausch von `/usr/libexec/bluetooth/obexd`,
- die Prüfung auf `obexd/plugins/phonebook-dummy.c`,
- den Telefonbuchpfad `/home/ark/phonebook/telecom/pb/`,
- den Boot-Service `/etc/systemd/system/obex-pbap.service` einschließlich User-D-Bus-Anbindung,
- sowie die Wiederherstellung des originalen Debian-/DarkOSRE-`obexd`.

Ohne diesen BlueZ/OBEX-Patch kann der Player zwar vCards erzeugen, das Navi erhält sie aber nicht über PBAP. Der Patch ist daher für die Hörspiel-Telefonbuchfunktion zwingend, nicht jedoch für die normale lokale Wiedergabe.

Der Player erzeugt nach dem Bibliotheksscan vCards in:

```text
$HOME/phonebook/telecom/pb/
```

Auf dem R36S mit Benutzer `ark` also normalerweise:

```text
/home/ark/phonebook/telecom/pb/
```

Fehlt ein Bluetooth-Adapter, werden Bluetooth-spezifische Funktionen deaktiviert. Der Adapterzustand wird während der Laufzeit geprüft; beim späteren Einstecken werden BatteryProvider, HFP-IPC, MPRIS/BlueZ und Autoconnect wieder initialisiert. Die sichtbaren Hörspiel-IDs werden bei Hotplug ebenfalls ohne Neustart aktualisiert.

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

Der Docker-Build prüft `APP_VERSION "0.2.29-bt-id-hotplug"` und baut unter anderem die integrierten HFP-/PBAP-Module mit.

## Tests

Zusätzliche Testhinweise befinden sich in:

- `TEST_0.2.27.md`
- `HFP_TEST.md`
- `PBAP_TEST.md`
- `pulse_patch/SYSTEM_PATCHES.md`

## Projekt / Kontakt

- GitHub: `https://github.com/Gunnar82/book_player_r36s`
- E-Mail: `gunnar_82@hotmail.com`
