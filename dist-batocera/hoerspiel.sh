#!/bin/bash

APPDIR="$(cd "$(dirname "$0")" && pwd)"
cd "$APPDIR" || exit 1

if [ -d "$APPDIR/lib" ]; then
  export LD_LIBRARY_PATH="$APPDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

exec "$APPDIR/hoerspiel_player"
