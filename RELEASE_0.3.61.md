# Release 0.3.61

## Schwerpunkt

0.3.61 schließt die erste praktisch getestete Self-Update-Implementierung für R36S und Waveshare GPM2804/Batocera ab.

## Self-Update

- Update-Prüfung über `<base_url>/latest.json`
- plattformspezifische Manifest-Einträge `r36s` und `gpm2804`
- HTTPS ist für Update-Dateien verpflichtend
- optionale mTLS-/CA-Konfiguration; standardmäßig können die TLS-Werte aus `[download]` übernommen werden
- Download der neuen Binary nach `/tmp/hoerspiel_player.new`
- SHA256-Prüfung vor Freigabe der Installation
- bei Download- oder SHA256-Fehler wird die Staging-Datei entfernt und die Installation bleibt gesperrt
- Installation ermittelt die laufende Binary über `/proc/self/exe`
- neue Binary wird zunächst auf dasselbe Dateisystem wie die laufende Binary kopiert und synchronisiert
- bestehende Binary wird als `<binary>.old` gesichert
- Aktivierung erfolgt per `rename()`; bei Aktivierungsfehler wird die alte Binary zurückgesetzt
- nach erfolgreicher Installation ist ein manueller Neustart des Players erforderlich

## Getestete Plattformen

Der reale Self-Update-Ablauf wurde auf beiden unterstützten Targets erfolgreich getestet:

- R36S
- Waveshare GPM2804 / Batocera

Getestet wurden Versionsprüfung, plattformspezifische Download-URL, HTTPS/mTLS, SHA256-Prüfung, Installation, `.old`-Backup und Start der neuen Version.

Zusätzlich wurde auf dem GPM2804 ein SHA256-Fehler praktisch geprüft: Die Installation blieb gesperrt. Nach Korrektur des Manifests und erneutem erfolgreichen Download konnte das Update regulär installiert werden.

## Server-Manifest

Beispiel für `latest.json`:

```json
{
  "version": "0.3.61",
  "r36s": {
    "url": "https://example.org/updates/0.3.61/r36s/hoerspiel_player",
    "sha256": "<64-stelliger-sha256>"
  },
  "gpm2804": {
    "url": "https://example.org/updates/0.3.61/gpm2804/hoerspiel_player",
    "sha256": "<64-stelliger-sha256>"
  }
}
```

Die Binary jedes Manifest-Eintrags muss aus dem jeweiligen Docker-Build stammen.

## Build

R36S:

```bash
make r36s
```

GPM2804/Batocera:

```bash
make gpm2804
```

Beide Dockerfiles prüfen `APP_VERSION "0.3.61"` sowie das Vorhandensein der Self-Update-Quellen.
