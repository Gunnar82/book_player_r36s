# HFP Test 2

Diese Fassung verwendet **keinen zweiten BlueZ-HFP-Endpunkt** mehr. PulseAudio 17 bleibt der HFP Audio Gateway und wird mit dem beiliegenden kleinen Patch so erweitert, dass `ATD...;` zusätzlich an den Player gespiegelt wird.

## Testziel

Am Navi `1001` wählen. Im Player-Log sollen erscheinen:

```
HFP Dial: 1001
HFP Playerkommando: 1001
```

Für den Test sind weiterhin Nummern hinterlegt:

- `1001`: Play/Pause
- `1002`: nächster Track
- `1003`: vorheriger Track
- `1004`: Play
- `1005`: Pause
- `1006`: Stop

Die endgültige Zuordnung `Nummer -> Hörspiel` kommt erst nach erfolgreichem HFP-Dial-Test.

## Socket ohne Bluetooth testen

Während der Player läuft:

```sh
python3 - <<'PY'
import os,socket
p=os.path.join(os.environ.get('XDG_RUNTIME_DIR','/run/user/1000'),'hoerspiel-player-hfp.sock')
s=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM)
s.sendto(b'DIAL 1001',p)
PY
```

Damit lässt sich zuerst prüfen, ob Player und IPC funktionieren, bevor PulseAudio gepatcht wird.
