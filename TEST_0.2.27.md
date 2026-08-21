# Testhinweise 0.2.27

0.2.27 basiert auf dem getesteten 0.2.26-dpad-only-Stand und bündelt die folgenden Änderungen.

## Änderungen

- Analoge Joystick-Achsen bleiben global ignoriert; Bedienung nur per D-Pad/Buttons.
- Ohne Bluetooth-Adapter werden BatteryProvider, HFP-IPC und BlueZ/MPRIS nicht gestartet oder abgefragt.
- Bluetooth-Hotplug wird periodisch erkannt. Beim Einstecken werden die Bluetooth-Funktionen aktiviert, beim Abziehen sauber beendet.
- In den Einstellungen ist Bluetooth ohne Adapter als `nicht verfuegbar` markiert.
- Beim Zurückgehen im Hörspiel-Browser bleibt der zuvor betretene Ordner markiert.
- `Button Debug` und `Programm-Log` wurden aus dem Systemmenü entfernt und stehen nun ganz unten in `Einstellungen -> DIAGNOSE`.
- Ladezeit-bis-voll-Anzeige und PBAP/HFP-Hörspielwahl aus den vorherigen Testständen bleiben enthalten.

## Empfohlene Tests

1. Player ohne Bluetooth-Adapter starten und Log auf wiederholte BlueZ-Fehler prüfen.
2. Bluetooth-Adapter während laufendem Player einstecken und HFP/PBAP testen.
3. Bluetooth-Adapter wieder abziehen; lokale Wiedergabe muss ungestört weiterlaufen.
4. Im Hörspiel-Browser mehrere Ebenen öffnen und zurückgehen; der verlassene Ordner muss markiert bleiben.
5. `Einstellungen` bis ans Ende scrollen und `Button Debug` sowie `Programm-Log` öffnen.
6. Ladekabel ein-/ausstecken und prüfen, dass keine Tracks/Positionen springen.
