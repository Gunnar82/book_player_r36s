#!/bin/bash
# DarkOS / EmulationStation: USB-Netzwerk mit SSH und DHCP-Server ausschalten
# Installation auf dem R36S: /roms/tools/USB-Network-Stop.sh

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
MAIN_SCRIPT="$SCRIPT_DIR/r36s-usb-ssh-dhcp-server.sh"

if [ ! -f "$MAIN_SCRIPT" ]; then
    echo "FEHLER: Hauptscript $MAIN_SCRIPT wurde nicht gefunden." >&2
    exit 1
fi

exec sudo -n -- /bin/bash "$MAIN_SCRIPT" stop
