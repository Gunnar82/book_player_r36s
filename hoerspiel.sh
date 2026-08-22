#!/bin/bash

APPDIR="/userdata/roms/ports/Hoerspiel Player"

# Debian/R36S-kompatiblen Fontpfad auf Batocera bereitstellen
mkdir -p /usr/share/fonts/truetype/dejavu
ln -sf /usr/share/fonts/dejavu/DejaVuSans.ttf \
  /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf

# Fehlende libsystemd fuer den Player bereitstellen
mkdir -p /tmp/hoerspiel-libs
cp "$APPDIR/lib/libsystemd.so.0" /tmp/hoerspiel-libs/

cd "$APPDIR"

export LD_LIBRARY_PATH="/tmp/hoerspiel-libs"

exec "$APPDIR/hoerspiel_player"
