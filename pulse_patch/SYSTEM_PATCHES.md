# System-Patches für HFP-Wahl und PBAP-Telefonbuch

Diese Datei dokumentiert die zwei Systemanpassungen, die für die Navi-Integration des R36S-Hörspielplayers benötigt werden. Beide Änderungen wurden auf DarkOSRE/Debian Trixie (AArch64) mit PulseAudio 17.0 und BlueZ 5.82 getestet.

## Überblick

Der Player stellt Hörspiele dem Navi als Telefonkontakte bereit. Jeder Eintrag besitzt eine feste numerische Hörspiel-ID. Wählt das Navi diesen Kontakt, wird die Nummer über HFP als `ATD<ID>;` übertragen. Der PulseAudio-Patch reicht diese ID an den Player weiter, der anschließend das zugehörige Hörspiel startet.

Datenfluss:

```text
Hörspielbibliothek
  -> vCard: Hörspielname + numerische ID
  -> BlueZ obexd / PBAP
  -> Telefonbuch des Navis
  -> Navi wählt die ID
  -> HFP: ATD<ID>;
  -> gepatchtes PulseAudio libbluez5-util.so
  -> hoerspiel-player-hfp.sock
  -> Player startet das Hörspiel
```

Die vCards werden vom Player unter `/home/ark/phonebook/telecom/pb/` erzeugt.

---

# 1. PulseAudio 17.0: HFP-DIAL-Patch

## Zweck

PulseAudio übernimmt auf dem R36S die native HFP-AG-Funktion. Das Navi sendet beim Wählen eines Telefonbucheintrags beispielsweise:

```text
ATD1001;
```

Der Patch in `patch_backend_native.py` erweitert `src/modules/bluetooth/backend-native.c`. Erkannte Wählbefehle werden an den Unix-Socket des Players weitergereicht.

Verwendeter Socket:

```text
/run/user/1000/hoerspiel-player-hfp.sock
```

Im gepatchten `libbluez5-util.so` lassen sich unter anderem folgende Marker finden:

```text
DIAL
hoerspiel_forward_dial
hoerspiel-player-hfp.sock
```

## Build auf einem x86-64-Rechner mit Docker

Für den R36S muss für `linux/arm64` gebaut werden. Das Ergebnis muss ein AArch64-ELF sein.

Beispiel:

```bash
docker buildx build \
  --platform linux/arm64 \
  --progress=plain \
  --output type=local,dest=./out \
  .
```

Nach dem Build prüfen:

```bash
file out/libbluez5-util.so
readelf -h out/libbluez5-util.so | grep -E 'Class:|Machine:'
strings out/libbluez5-util.so | grep -Ei 'ATD|DIAL|hoerspiel|hfp'
```

Erwartet werden `ELF64`, `AArch64` und die oben genannten HFP-/DIAL-Marker.

## Installation auf dem R36S

Das Original unbedingt zuerst sichern:

```bash
sudo cp \
  /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so \
  /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so.orig
```

Danach die gepatchte Datei installieren:

```bash
sudo cp /tmp/libbluez5-util.so \
  /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so
sudo chown root:root /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so
sudo chmod 755 /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so
sudo systemctl restart pulseaudio
```

Prüfen:

```bash
systemctl status pulseaudio --no-pager
pactl info
```

Ein HFP-Wähltest kann mit `btmon` beobachtet werden:

```bash
sudo btmon | grep --line-buffered -E 'RFCOMM|ATD|CLCC|CIND|CMER|CHUP' \
  > /tmp/hfp-dial-test.txt
```

Bei einer Wahl von ID 1001 muss im Mitschnitt `ATD1001;` erscheinen.

## Wiederherstellung

```bash
sudo cp \
  /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so.orig \
  /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so
sudo systemctl restart pulseaudio
```

---

# 2. BlueZ 5.82: PBAP mit Dummy-Phonebook

## Warum ein eigener BlueZ-Build nötig ist

Das mit DarkOSRE installierte `/usr/libexec/bluetooth/obexd` wurde mit dem Evolution-Data-Server-Backend gebaut. Das lässt sich auf dem Gerät prüfen mit:

```bash
strings /usr/libexec/bluetooth/obexd | grep -E 'phonebook-(dummy|ebook)\.c'
ldd /usr/libexec/bluetooth/obexd | grep -E 'libebook|libedataserver|libcamel'
```

Beim Original wurden `phonebook-ebook.c` sowie `libebook`, `libedataserver` und `libcamel` gefunden.

Für den Hörspielplayer wird stattdessen BlueZ mit folgendem Backend benötigt:

```text
--with-phonebook=dummy
```

Dadurch kann `obexd` vCards direkt aus dem Dateisystem als PBAP-Telefonbuch anbieten.

## BlueZ 5.82 für ARM64 bauen

Der getestete Configure-Aufruf lautet:

```bash
./configure \
  --prefix=/usr \
  --libexecdir=/usr/libexec \
  --enable-obex \
  --disable-cups \
  --disable-mesh \
  --disable-midi \
  --with-phonebook=dummy \
  --with-udevdir=/usr/lib/udev \
  --with-systemdsystemunitdir=/usr/lib/systemd/system \
  --with-systemduserunitdir=/usr/lib/systemd/user
```

Für den Build wurden unter Debian Trixie unter anderem folgende Pakete benötigt:

```text
build-essential
autoconf
automake
libtool
pkg-config
git
ca-certificates
libdbus-1-dev
libglib2.0-dev
libical-dev
libbluetooth-dev
libreadline-dev
libudev-dev
libsystemd-dev
python3-docutils
```

`python3-docutils` stellt `rst2man` bereit, das der BlueZ-Build benötigt.

Der Docker-Build erfolgt ebenfalls für ARM64:

```bash
docker buildx build \
  --platform linux/arm64 \
  --progress=plain \
  --output type=local,dest=./out \
  .
```

## Build prüfen

```bash
file out/obexd
strings out/obexd | grep -E 'phonebook-(dummy|ebook)\.c'
strings out/obexd | grep -Ei 'Phonebook Access|x-bt/phonebook|PBAP' | head -30
```

Erwartet:

```text
ELF 64-bit ... ARM aarch64
obexd/plugins/phonebook-dummy.c
Phonebook Access server
x-bt/phonebook
PBAP
```

Auf einem x86-64-Buildrechner kann `ldd out/obexd` wegen der fremden ARM64-Architektur fehlschlagen. Die Abhängigkeiten daher auf dem R36S prüfen.

## Zusätzliche Runtime auf dem R36S

Der selbst gebaute BlueZ-5.82-`obexd` benötigt `libicalvcal.so.3`. Auf Debian Trixie wird diese durch folgendes Paket bereitgestellt:

```bash
sudo apt-get update
sudo apt-get install libical3t64
```

Danach:

```bash
ldd /usr/libexec/bluetooth/obexd | grep 'not found'
```

Die Ausgabe muss leer sein.

## Installation von obexd

Original sichern:

```bash
sudo cp /usr/libexec/bluetooth/obexd \
        /usr/libexec/bluetooth/obexd.orig
```

Neue Version installieren:

```bash
sudo cp /tmp/obexd-pbap /usr/libexec/bluetooth/obexd
sudo chown root:root /usr/libexec/bluetooth/obexd
sudo chmod 755 /usr/libexec/bluetooth/obexd
```

Danach prüfen:

```bash
strings /usr/libexec/bluetooth/obexd | grep -E 'phonebook-(dummy|ebook)\.c'
```

Es muss `phonebook-dummy.c` erscheinen.

## Telefonbuch-Verzeichnis

Das Dummy-Backend verwendet das Home-Verzeichnis des Benutzers `ark`. Der Player erzeugt die Einträge unter:

```text
/home/ark/phonebook/telecom/pb/
```

Eine minimale Test-vCard sieht beispielsweise so aus:

```text
BEGIN:VCARD
VERSION:3.0
N:Hoerspiel Test 1001;;;;
FN:Hoerspiel Test 1001
TEL:1001
END:VCARD
```

Die Nummer muss mit der persistenten Hörspiel-ID des Players übereinstimmen.

---

# 3. OBEX/PBAP automatisch beim Boot starten

Der von Debian mitgelieferte User-Service `/usr/lib/systemd/user/obex.service` reicht für das Gerät nicht aus, wenn PBAP bereits ohne interaktive Anmeldung verfügbar sein soll.

Der bisherige User-Service wird deaktiviert:

```bash
systemctl --user disable --now obex.service
```

Hinweis: D-Bus kann einen deaktivierten User-Service weiterhin bedarfsgesteuert aktivieren. Für den R36S wird deshalb der unten beschriebene Systemdienst als definierter Boot-Pfad verwendet.

## Systemdienst

Datei:

```text
/etc/systemd/system/obex-pbap.service
```

Getesteter Inhalt:

```ini
[Unit]
Description=Bluetooth OBEX PBAP Server
After=bluetooth.service user@1000.service
Requires=bluetooth.service
Wants=user@1000.service

[Service]
Type=simple
User=ark
Environment=HOME=/home/ark
Environment=XDG_RUNTIME_DIR=/run/user/1000
Environment=DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus
ExecStart=/usr/libexec/bluetooth/obexd
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target
```

Wichtig sind `HOME=/home/ark` für das Dummy-Telefonbuch und die Verbindung zum User-D-Bus über `/run/user/1000/bus`. Ohne `DBUS_SESSION_BUS_ADDRESS` endet `obexd` typischerweise mit:

```text
manager_init failed
Unable to autolaunch a dbus-daemon without a $DISPLAY for X11
```

Service aktivieren:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now obex-pbap.service
```

Prüfen:

```bash
sudo systemctl status obex-pbap.service --no-pager -l
ls -l /run/user/1000/bus
```

Falls der User-Manager beim Boot nicht automatisch verfügbar ist, Linger für `ark` aktivieren:

```bash
sudo loginctl enable-linger ark
```

Der entscheidende Abschlusstest ist ein kompletter Neustart ohne anschließenden SSH-Login. Das Navi muss danach das PBAP-Telefonbuch direkt laden können.

---

# 4. Funktionstest der gesamten Kette

1. Player starten und prüfen, dass vCards unter `/home/ark/phonebook/telecom/pb/` vorhanden sind.
2. R36S und Navi per Bluetooth verbinden.
3. Telefonbuch im Navi synchronisieren.
4. Einen Hörspielnamen im Navi auswählen.
5. Das Navi wählt die in der vCard gespeicherte numerische ID.
6. HFP überträgt `ATD<ID>;`.
7. Der PulseAudio-Patch leitet die ID an den Player weiter.
8. Der Player startet das zugeordnete Hörspiel.

Dieser Ablauf wurde mit echten Hörspieleinträgen erfolgreich getestet.

---

# 5. Wiederherstellung des Originalzustands

BlueZ/OBEX-Systemdienst stoppen und deaktivieren:

```bash
sudo systemctl disable --now obex-pbap.service
```

Originales `obexd` wiederherstellen:

```bash
sudo cp /usr/libexec/bluetooth/obexd.orig \
        /usr/libexec/bluetooth/obexd
sudo chown root:root /usr/libexec/bluetooth/obexd
sudo chmod 755 /usr/libexec/bluetooth/obexd
```

Bei Bedarf den Debian-User-Service wieder aktivieren:

```bash
systemctl --user enable obex.service
```

PulseAudio wiederherstellen:

```bash
sudo cp \
  /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so.orig \
  /usr/lib/pulse-17.0+dfsg1/modules/libbluez5-util.so
sudo systemctl restart pulseaudio
```

Damit sind beide Systempatches unabhängig vom Player wieder rückgängig gemacht.
