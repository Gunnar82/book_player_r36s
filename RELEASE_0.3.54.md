# Release 0.3.54

## Schwerpunkt

0.3.54 erweitert den GPM2804/Batocera-Build um asynchrone Hintergrunddownloads.

## Downloads

- laufende Downloads können im Fortschrittsfenster mit `Y` in den Hintergrund geschickt werden
- `B` bricht den Download weiterhin kontrolliert ab
- der Wiedergabe-Screen zeigt bei aktivem Hintergrundjob kompakt `DL <Prozent> · <Rate> · <Datei>/<Gesamt>`
- beim erneuten Öffnen von **Downloads** wird der laufende Job wieder mit Fortschritt, aktuellem Ordner und Downloadrate angezeigt
- nur ein Downloadjob kann gleichzeitig aktiv sein
- der Worker wird beim Programmende kontrolliert beendet
- Downloadstatus und Workerzustand werden mutex-geschützt verwaltet; SDL-Rendering und Eingaben bleiben im UI-Thread

## Weitere Download-Anpassungen

- Fortschrittsfenster zeigt aktuellen Ordner und aktuelle Downloadrate
- lange Ordnerpfade werden für die Anzeige gekürzt
- Download-Abbruch verwendet die normalisierte Controller-Belegung; auf Batocera funktioniert damit die konfigurierte B-Taste
- die Legende im Fortschrittsfenster wurde ohne zusätzlichen Render-Pass integriert, um Flackern zu vermeiden

## Build / Version

- `APP_VERSION` auf `0.3.54` erhöht
- Versionsprüfung in `Dockerfile.gpm2804-batocera` auf `0.3.54` aktualisiert
- Versionsprüfung im R36S-`Dockerfile` von dem veralteten Stand auf `0.3.54` angehoben
- Batocera-Build prüft zusätzlich das Vorhandensein der neuen Hintergrunddownload-Module

## Plattformstatus

Die Hintergrunddownload-Erweiterung ist aktuell im GPM2804/Batocera-Build eingebunden und dort auf Hardware getestet. Der R36S-Build bleibt bei seinem bisherigen synchronen Downloadpfad, bis die Erweiterung dort separat portiert und getestet wurde.
