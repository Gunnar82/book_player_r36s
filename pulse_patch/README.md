# PulseAudio 17 HFP dial forwarder - Test 2

Ziel: PulseAudio bleibt alleiniger HFP Audio Gateway. Eingehende `ATD...;`-Befehle werden nur zusätzlich an den Hörspielplayer gespiegelt. Das bestehende HFP-Verhalten wird noch nicht verändert.

Der Player lauscht auf:

`$XDG_RUNTIME_DIR/hoerspiel-player-hfp.sock`

Typische Nachricht:

`DIAL 1001`

## Auf dem R36S bauen

Für den exakt installierten Debian-Trixie-Stand `17.0+dfsg1-2+b1` zuerst die Build-Werkzeuge und Quellen bereitstellen. Falls `apt source pulseaudio` meldet, dass keine `deb-src`-Quelle konfiguriert ist, muss diese für Debian Trixie einmal aktiviert werden.

```sh
sudo apt install build-essential devscripts dpkg-dev meson ninja-build
mkdir -p ~/src && cd ~/src
apt source pulseaudio=17.0+dfsg1-2
cd pulseaudio-17.0+dfsg1
python3 /pfad/zum/player/pulse_patch/patch_backend_native.py src/modules/bluetooth/backend-native.c
```

Danach das Debian-Paket regulär neu bauen. Für den ersten Test reicht es auch, nur das erzeugte `libbluez5-util.so` aus dem passenden Build zu installieren, aber ein korrekt gebautes Debian-Paket ist sicherer und leichter rückgängig zu machen.

## Was der Patch absichtlich NICHT macht

- Kein Fake-Anrufstatus.
- Kein `OK` für `ATD`.
- Kein Umschalten von A2DP/HFP.
- Keine Hörspiel-Zuordnung.

Das Navi kann deshalb weiterhin `Anruf fehlgeschlagen` anzeigen. Für Test 2 zählt nur, ob im Player-Log `HFP Dial: 1001` erscheint.
