# PulseAudio-HFP- und BlueZ-PBAP-Patches

Dieser Ordner enthält die Systemanpassungen für die Navi-Integration des Hörspielplayers.

- `patch_backend_native.py`: Patch für PulseAudio 17 `backend-native.c`. Eingehende HFP-Wählbefehle `ATD...;` werden zusätzlich als `DIAL <ID>` an den lokalen Player-Socket gespiegelt.
- `SYSTEM_PATCHES.md`: vollständige Dokumentation für PulseAudio-HFP und BlueZ-5.82-PBAP mit Dummy-Phonebook, ARM64-Docker-Build, Installation, Runtime-Abhängigkeiten, `obex-pbap.service`, Tests und Wiederherstellung.

Der Player-Socket lautet normalerweise:

```text
/run/user/1000/hoerspiel-player-hfp.sock
```

Das PBAP-Telefonbuch liegt bei Benutzer `ark` unter:

```text
/home/ark/phonebook/telecom/pb/
```

Die beiden Systempatches sind unabhängig vom Player rückgängig zu machen. Vor dem Ersetzen der Originaldateien müssen diese wie in `SYSTEM_PATCHES.md` beschrieben gesichert werden.
