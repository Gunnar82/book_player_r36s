# Testhinweise 0.2.27-ui-bt-hotplug

Dieser Teststand basiert auf 0.2.26-dpad-only und ist noch nicht fuer GitHub vorgesehen.

## Aenderungen

- Analoge Joystick-Achsen bleiben global ignoriert; Bedienung nur per D-Pad/Buttons.
- Ohne Bluetooth-Adapter werden BatteryProvider, HFP-IPC und BlueZ/MPRIS nicht gestartet oder abgefragt.
- Bluetooth-Hotplug wird periodisch erkannt. Beim Einstecken werden die Bluetooth-Funktionen aktiviert, beim Abziehen sauber beendet.
- In den Einstellungen ist Bluetooth ohne Adapter als `nicht verfuegbar` markiert.
- Beim Zurueckgehen im Hoerspiel-Browser bleibt der zuvor betretene Ordner markiert.
- `Button Debug` und `Programm-Log` wurden aus dem Systemmenue entfernt und stehen nun ganz unten in `Einstellungen -> DIAGNOSE`.
- Ladezeit-bis-voll-Anzeige und PBAP/HFP-Hoerspielwahl aus den vorherigen Teststaenden bleiben enthalten.

## Empfohlene Tests

1. Player ohne Bluetooth-Adapter starten und Log auf wiederholte BlueZ-Fehler pruefen.
2. Bluetooth-Adapter waehrend laufendem Player einstecken und HFP/PBAP testen.
3. Bluetooth-Adapter wieder abziehen; lokale Wiedergabe muss ungestoert weiterlaufen.
4. Im Hoerspiel-Browser mehrere Ebenen oeffnen und mit B zurueckgehen; der verlassene Ordner muss markiert bleiben.
5. `Einstellungen` bis ans Ende scrollen und `Button Debug` sowie `Programm-Log` oeffnen.
6. Ladekabel ein-/ausstecken und pruefen, dass keine Tracks/Positionen springen.
