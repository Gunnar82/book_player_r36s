#!/bin/bash
# EmulationStation-Starter: USB-SSH mit DHCP-Server einschalten

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
MAIN_SCRIPT="$SCRIPT_DIR/r36s-usb-ssh-dhcp-server.sh"

if [ ! -f "$MAIN_SCRIPT" ]; then
    echo "FEHLER: $MAIN_SCRIPT wurde nicht gefunden." >&2
    exit 1
fi

exec sudo -n -- /bin/bash "$MAIN_SCRIPT" start
