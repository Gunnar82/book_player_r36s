# Projekt-Workflow

Dieses Dokument beschreibt, wie am Projekt `book_player_r36s` gearbeitet wird, ohne dass Entwicklungsstände durch verlorene Chats, Rechnerwechsel oder lokale Dateien verloren gehen.

## Grundregel

**GitHub `main` ist die maßgebliche Quelle für stabile Projektstände.**

Lokale Verzeichnisse, ZIP-Dateien, Chat-Verläufe und Build-Artefakte sind keine führende Quelle. Ein getesteter Stand gehört auf GitHub.

## Normaler Ablauf

### 1. Lokalen stabilen Stand aktualisieren

```bash
git switch main
git pull --ff-only origin main
git status
```

Der Arbeitsbaum sollte sauber sein.

### 2. Änderung auf eigenem Branch

Größere, riskante oder mehrstufige Änderungen erfolgen auf einem Feature-/Refactoring-Branch, zum Beispiel:

```text
refactor/lifecycle-utils-0.3.52
feature/batocera-shutdown
```

Der Branch wird auf GitHub gepflegt. Der lokale Testrechner muss Änderungen nicht selbst committen oder pushen.

### 3. Änderungen lokal zum Test holen

```bash
git fetch origin
git switch <branch-name>
git pull
```

Danach den passenden Build starten, zum Beispiel:

```bash
make gpm2804
```

oder:

```bash
make r36s
```

### 4. Schrittweise testen

Nach jeder abgeschlossenen Teiländerung wird ein kleiner, klarer Test durchgeführt. Beispiele:

- Build erfolgreich
- Einstellung bleibt nach Neustart erhalten
- Session-Einstellung wird bewusst nicht gespeichert
- Bluetooth verbindet weiterhin
- große Streamliste lädt
- Shutdown funktioniert auf echter Hardware

Erst nach einem bestätigten Test folgt der nächste Refactoring-Schritt.

### 5. Pull Request nach `main`

Wenn der Branch vollständig getestet ist:

1. Versionsnummer prüfen bzw. erhöhen.
2. README und relevante Doku aktualisieren.
3. Branch gegen `main` prüfen.
4. Pull Request erstellen.
5. Nach erfolgreicher Prüfung per Squash Merge nach `main` übernehmen.

Danach lokal wieder auf den stabilen Stand wechseln:

```bash
git switch main
git pull origin main
git status
```

## Versionsregel

Die Anwendungsversion steht in `config.h` als `APP_VERSION`.

- Unfertige Zwischenstände müssen nicht für jeden Commit eine neue Version erhalten.
- Vor dem finalen Test eines abgeschlossenen Release-/Refactoring-Blocks wird die Zielversion gesetzt.
- README und `config.h` müssen beim Merge denselben Versionsstand nennen.

## Testregel

Ein erfolgreicher Compilerlauf ersetzt keinen Funktionstest, wenn die Änderung Hardware-, Netzwerk-, Config- oder Shutdown-Verhalten betrifft.

Bei plattformspezifischen Änderungen dokumentieren wir ausdrücklich, auf welcher Hardware getestet wurde. Kann eine Plattform nicht getestet werden, wird das vor dem Merge festgehalten.

## Checkpoint-Regel

Während einer längeren Arbeit gilt:

1. kleine logisch abgeschlossene Änderungen separat sichern,
2. nach jedem erfolgreichen Hardware-/Funktionstest einen nachvollziehbaren Git-Stand behalten,
3. vor größeren Umbauten einen funktionierenden Stand sichern,
4. keine wichtigen Änderungen ausschließlich im Chat belassen.

Damit kann bei einem verlorenen Chat direkt aus dem Branch und seiner Git-Historie weitergearbeitet werden.

## Wiederaufnahme nach verlorenem Chat oder Kontext

Nicht aus Erinnerung rekonstruieren. Zuerst GitHub prüfen:

```bash
git fetch origin
git branch -a
git log --oneline --decorate -15
```

Für einen stabilen Stand:

```bash
git switch main
git pull --ff-only origin main
```

Für eine laufende Änderung:

```bash
git switch <feature-branch>
git pull
```

Für die Fortsetzung reichen im Wesentlichen:

- Repository: `Gunnar82/book_player_r36s`
- aktueller Branch
- letzter relevante Commit
- letzter erfolgreicher Test
- nächster geplanter Schritt

## Umgang mit `config.ini`

Die reale `config.ini` ist gerätebezogen und darf nicht ins Repository eingecheckt werden. Sie kann Pfade, Zertifikate oder Schlüsselparameter enthalten.

- `config.ini` bleibt in `.gitignore`.
- Dokumentation und Beispiele gehören in `config.ini.example` bzw. README.
- Config-Schreibfunktionen sollen möglichst den gemeinsamen atomischen Writer `config_update.c/.h` verwenden.

## Umgang mit ZIP-Dateien

ZIP-Dateien sind nur Backup oder Transportformat. Sie ersetzen keine Git-Historie.

Bei Wiederherstellung eines ZIP-Stands:

1. aktuelles Repository klonen,
2. Snapshot darüber synchronisieren,
3. `git diff` prüfen,
4. bauen und testen,
5. Änderungen auf einem Branch sichern,
6. per Pull Request übernehmen.

## Release-Checkliste

Vor einem Merge eines neuen stabilen Versionsstands:

- `config.h` zeigt die korrekte Version
- README zeigt dieselbe Version
- betroffener Build läuft
- relevante Hardware-/Funktionstests sind durchgeführt
- bekannte ungetestete Plattformteile sind dokumentiert
- Branch ist nicht hinter `main`
- Pull Request beschreibt Änderungen und Testergebnisse

Optional kann ein stabiler Release nach dem Merge getaggt werden:

```bash
git tag -a v0.3.52 -m "book_player_r36s 0.3.52"
git push origin v0.3.52
```

## Kurzfassung

- `main` ist der stabile Referenzstand.
- Arbeiten erfolgen auf klar benannten Branches.
- Der lokale Testrechner pullt und testet; er muss nicht selbst pushen.
- Kleine Schritte werden einzeln getestet.
- Erst nach erfolgreichem Test wird per PR nach `main` gemergt.
- Version und README werden vor dem Merge synchronisiert.
- Ein verlorener Chat darf keinen Entwicklungsstand kosten.
