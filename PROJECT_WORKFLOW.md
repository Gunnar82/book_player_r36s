# Projekt-Workflow

Dieses Dokument legt fest, wie am Projekt `book_player_r36s` gearbeitet wird und wie verhindert wird, dass Entwicklungsstand oder Kontext durch einen verlorenen Chat, Rechnerwechsel oder lokale Dateien verloren gehen.

## Grundregel

**GitHub `main` ist die maßgebliche Quelle für den aktuellen Projektstand.**

Lokale Verzeichnisse, ZIP-Dateien, Chat-Verläufe oder einzelne Build-Artefakte gelten nicht als führender Stand. Sie können als Backup dienen, ersetzen aber keinen Commit.

Aktueller Referenzstand zum Zeitpunkt der Einführung dieses Workflows:

- Version: `0.3.49`
- Referenz-Commit: `e6408857016381db23278641325ef19addf8509d`
- Branch: `main`

## Arbeitsweise

### 1. Vor Beginn einer Änderung

Immer zuerst den aktuellen Stand holen:

```bash
git checkout main
git pull --ff-only origin main
```

Danach prüfen:

```bash
git status
```

Der Arbeitsbaum sollte sauber sein, bevor neue Änderungen begonnen werden.

### 2. Kleine, abgeschlossene Änderungen direkt sichern

Sobald eine Änderung sinnvoll abgeschlossen und überprüfbar ist:

```bash
git add -A
git commit -m "Kurze Beschreibung der Änderung"
git push origin main
```

Nicht bis zum Ende einer langen Entwicklungs- oder Chat-Sitzung warten. Ein funktionierender Zwischenstand ist bereits einen Commit wert.

### 3. Größere oder riskante Änderungen

Für Umbauten, Experimente oder Änderungen, die den Build vorübergehend brechen können, einen eigenen Branch verwenden:

```bash
git checkout -b feature/beschreibung
```

Zwischenstände dort regelmäßig committen und pushen:

```bash
git add -A
git commit -m "WIP: Beschreibung"
git push -u origin feature/beschreibung
```

Nach erfolgreichem Test kann der Branch nach `main` übernommen werden.

## Checkpoint-Regel für Entwicklungs-Sitzungen

Während einer längeren Entwicklungs-Sitzung gilt:

1. Nach jeder funktionierenden Teilfunktion committen.
2. Spätestens vor einem Themenwechsel committen.
3. Vor größeren Umbauten committen.
4. Am Ende jeder Sitzung `git status` prüfen und einen letzten Checkpoint pushen.
5. Im Commit-Text beschreiben, **was funktioniert** und nicht nur, welche Datei geändert wurde.

Beispiele:

```text
Fix R36S package export with runtime libraries
Add Bluetooth hotplug handling to browser UI
WIP: PBAP phonebook integration works, HFP dial mapping pending
```

Auch ein `WIP`-Commit ist besser als ein wichtiger Stand, der ausschließlich in einem Chat oder auf einem einzelnen Rechner existiert.

## Dokumentation des aktuellen Entwicklungsstands

Wenn eine Änderung mehrere Komponenten betrifft oder noch offene Punkte hat, zusätzlich eine kurze Statusnotiz im Repository pflegen.

Eine Statusnotiz sollte enthalten:

- aktuelle Version
- zuletzt erfolgreich getestete Funktion
- bekannte Probleme
- offene nächste Schritte
- relevante Testhardware
- besondere Build- oder Installationshinweise

Für Release-spezifische Änderungen können weiterhin `CHANGELOG_*.md`, `RELEASE_*.md` und Testdokumente verwendet werden.

## Umgang mit ZIP-Dateien

ZIP-Dateien dürfen als Backup oder zum Transport verwendet werden, aber nicht als primäre Versionsverwaltung.

Wenn ein ZIP-Stand wiederhergestellt werden muss:

1. aktuelles Repository klonen,
2. ZIP-Inhalt darüber synchronisieren,
3. `git diff` prüfen,
4. Änderungen testen,
5. als einen nachvollziehbaren Commit sichern,
6. auf GitHub pushen.

Beispiel:

```bash
git clone https://github.com/Gunnar82/book_player_r36s.git
cd book_player_r36s
rsync -av --delete --exclude='.git' /pfad/zum/snapshot/ ./
git status
git diff --stat
git add -A
git commit -m "Sync project snapshot <version>"
git push origin main
```

## Wiederaufnahme nach verlorenem Chat oder Kontext

Wenn ein Chat oder eine Entwicklungsnotiz verloren geht, wird **nicht versucht, den Stand aus Erinnerung zu rekonstruieren**.

Stattdessen:

```bash
git checkout main
git pull --ff-only origin main
git log --oneline -10
git status
```

Danach dienen die letzten Commits und die Repository-Dokumentation als Ausgangspunkt.

Für die Fortsetzung einer Arbeit sollten mindestens diese Informationen verfügbar sein:

- Repository: `Gunnar82/book_player_r36s`
- Branch oder Feature-Branch
- letzter relevanter Commit
- gewünschtes nächstes Ziel

Damit ist ein verlorener Chat lästig, aber kein Verlust des Entwicklungsstands.

## Empfohlener Release-Ablauf

Bei einem neuen stabilen Stand:

1. Versionsnummer in `config.h` aktualisieren.
2. Builds für die betroffenen Targets prüfen.
3. relevante Testfälle durchführen.
4. Changelog bzw. Release-Dokumentation ergänzen.
5. Änderungen committen und pushen.
6. Optional einen Git-Tag setzen.

Beispiel:

```bash
git tag -a v0.3.49 -m "book_player_r36s 0.3.49"
git push origin v0.3.49
```

Tags machen einen getesteten Stand dauerhaft eindeutig referenzierbar und sind besonders hilfreich, wenn `main` später weiterentwickelt wird.

## Kurzfassung

- `main` ist die Wahrheit.
- Funktionierende Zwischenstände sofort committen und pushen.
- Riskante Arbeiten auf Feature-Branches durchführen.
- ZIPs nur als Backup verwenden.
- Releases taggen.
- Nach jeder Sitzung einen sauberen Git-Checkpoint hinterlassen.

Wenn diese Regeln eingehalten werden, kann die Entwicklung jederzeit aus GitHub vollständig wieder aufgenommen werden, unabhängig davon, ob ein Chat-Verlauf noch existiert.