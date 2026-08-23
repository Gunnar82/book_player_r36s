#!/bin/bash
APPDIR="/userdata/roms/ports/Hoerspiel Player"
RUNTIME_LIBDIR="/tmp/hoerspiel-libs"
mkdir -p "$RUNTIME_LIBDIR"
for lib in libsystemd.so.0 libqrencode.so.4; do
  if [ -f "$APPDIR/lib/$lib" ]; then cp -L "$APPDIR/lib/$lib" "$RUNTIME_LIBDIR/$lib"; fi
done
cd "$APPDIR" || exit 1
export LD_LIBRARY_PATH="$RUNTIME_LIBDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$APPDIR/hoerspiel_player"
