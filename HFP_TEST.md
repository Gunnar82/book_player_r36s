# HFP-Test

PulseAudio 17 bleibt der HFP Audio Gateway. Der Patch spiegelt eingehende `ATD...;`-Befehle zusätzlich an den lokalen Player-Socket.

Der Player lauscht auf:

```text
$XDG_RUNTIME_DIR/hoerspiel-player-hfp.sock
```

Beispiel:

```text
DIAL 1001
```

Im Player-Log muss bei einer Wahl die empfangene Hörspiel-ID erscheinen. Details zum Build und zur Installation stehen in `pulse_patch/SYSTEM_PATCHES.md`.

## Socket lokal testen

```bash
python3 - <<'PY'
import os,socket
p=os.path.join(os.environ.get('XDG_RUNTIME_DIR','/run/user/1000'),'hoerspiel-player-hfp.sock')
s=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM)
s.sendto(b'DIAL 1001',p)
PY
```
