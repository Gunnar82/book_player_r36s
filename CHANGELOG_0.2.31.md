# 0.2.31 – Download Skip & Rücksprungmarkierung

Diese Version erweitert den Download-Browser um zwei getestete Komfort- und Sicherheitsverbesserungen.

## Bereits vorhandene Dateien

Beim Download wird vor dem Übertragen einer Datei geprüft, ob am Ziel bereits eine reguläre Datei gleichen Namens existiert. Ist im Server-Listing eine Dateigröße bekannt und stimmt diese exakt mit der lokalen Dateigröße überein, wird die Datei übersprungen und nicht erneut übertragen.

Ist die Servergröße unbekannt oder unterscheidet sich die lokale Größe, wird die Datei wie bisher vollständig in eine temporäre `.part`-Datei geladen. Erst nach erfolgreichem Abschluss ersetzt `rename()` die Zieldatei. Bei Abbruch oder Fehler bleibt eine vorhandene Zieldatei unangetastet und die `.part`-Datei wird entfernt.

Übersprungene Dateien werden in der Gesamtfortschrittsrechnung als bereits erledigte Bytes berücksichtigt, damit Gesamtbalken und Restzeitschätzung konsistent bleiben.

## Navigation im Download-Browser

Beim Öffnen eines Unterordners merkt sich der Download-Browser dessen vollständigen relativen Pfad. Wird mit `B` in den übergeordneten Ordner zurückgegangen, wird der zuvor geöffnete Ordner wieder markiert und bei Bedarf in den sichtbaren Bereich gescrollt. Die Navigation entspricht damit dem bereits im Hörspiel-Browser verwendeten Verhalten.

## Idle-Timer

Die in 0.2.30 eingeführte Pause des Idle-Timers während aktiver Downloads bleibt unverändert erhalten. Downloadzeit wird nach Abschluss oder Abbruch nicht nachträglich vom Idle-Timer abgezogen.

## Version

`APP_VERSION` lautet `0.2.31-download-skip-return`. Der Docker-Build prüft diesen Versionsstand vor dem Kompilieren.
