#!/bin/bash
# R36S USB-Gadget-Netzwerk mit lokalem DHCP-Server
# Windows erhält automatisch eine Adresse; SSH-Ziel ist 192.168.7.1.
# Ohne Parameter aus EmulationStation: Start/Stop-Umschalter.
# Optional in der Shell: start|stop|restart|status

set -u

if [ "$(id -u)" -ne 0 ]; then
    exec sudo -n -- /bin/bash "$0" "$@"
fi

IFACE="${USB_IFACE:-usb0}"
R36S_ADDR="${USB_R36S_ADDR:-192.168.7.1/24}"
DEV_MAC="${USB_DEV_MAC:-02:00:00:00:00:02}"
HOST_MAC="${USB_HOST_MAC:-02:00:00:00:00:01}"
RUN_DIR="/run/r36s-usb-dhcp-server"
DNSMASQ_CONF="$RUN_DIR/dnsmasq.conf"
DNSMASQ_PID="$RUN_DIR/dnsmasq.pid"

log() { printf '[r36s-usb] %s\n' "$*"; }
die() { log "FEHLER: $*" >&2; exit 1; }

require_root() {
    [ "$(id -u)" -eq 0 ] || die "Bitte mit sudo ausführen."
}

wait_for_iface() {
    local n=0
    while [ ! -d "/sys/class/net/$IFACE" ] && [ "$n" -lt 30 ]; do
        sleep 0.1
        n=$((n + 1))
    done
    [ -d "/sys/class/net/$IFACE" ] || die "Netzwerkschnittstelle $IFACE wurde nicht erzeugt."
}

start_server() {
    require_root
    command -v dnsmasq >/dev/null 2>&1 || \
        die "dnsmasq fehlt. Installiere es einmalig mit: sudo apt install dnsmasq"

    mkdir -p "$RUN_DIR"

    if [ -d "/sys/class/net/$IFACE" ]; then
        log "$IFACE existiert bereits; vorhandene Adressen werden entfernt."
    else
        modprobe g_ether dev_addr="$DEV_MAC" host_addr="$HOST_MAC" || \
            die "g_ether konnte nicht geladen werden."
    fi

    wait_for_iface
    ip link set "$IFACE" up
    ip -4 addr flush dev "$IFACE"
    ip address add "$R36S_ADDR" dev "$IFACE"

    cat >"$DNSMASQ_CONF" <<EOF
interface=$IFACE
bind-interfaces
port=0
dhcp-authoritative
dhcp-range=192.168.7.10,192.168.7.50,255.255.255.0,12h
dhcp-option=3
dhcp-option=6
dhcp-leasefile=$RUN_DIR/dnsmasq.leases
pid-file=$DNSMASQ_PID
EOF

    dnsmasq --conf-file="$DNSMASQ_CONF" || {
        ip -4 addr flush dev "$IFACE" 2>/dev/null || true
        modprobe -r g_ether 2>/dev/null || true
        die "Der DHCP-Server konnte nicht gestartet werden."
    }

    systemctl start ssh 2>/dev/null || service ssh start 2>/dev/null || \
        log "WARNUNG: SSH-Dienst konnte nicht automatisch gestartet werden."

    log "Bereit. OTG-Port mit Windows verbinden."
    log "Windows bezieht seine Adresse automatisch; SSH-Ziel: ark@192.168.7.1"
    ip -4 -br addr show dev "$IFACE"
}

stop_server() {
    require_root

    if [ -f "$DNSMASQ_PID" ]; then
        kill "$(cat "$DNSMASQ_PID")" 2>/dev/null || true
        rm -f "$DNSMASQ_PID"
    fi

    if [ -d "/sys/class/net/$IFACE" ]; then
        ip -4 addr flush dev "$IFACE" 2>/dev/null || true
        ip link set "$IFACE" down 2>/dev/null || true
    fi

    modprobe -r g_ether 2>/dev/null || \
        log "WARNUNG: g_ether konnte nicht entladen werden (eventuell noch in Benutzung)."
    rm -rf "$RUN_DIR"
    log "USB-Netzwerk und DHCP-Server beendet."
}

show_status() {
    if [ -d "/sys/class/net/$IFACE" ]; then
        ip -4 -br addr show dev "$IFACE"
        if [ -f "$DNSMASQ_PID" ] && kill -0 "$(cat "$DNSMASQ_PID")" 2>/dev/null; then
            log "DHCP-Server aktiv (PID $(cat "$DNSMASQ_PID"))."
        else
            log "DHCP-Server nicht aktiv."
        fi
        systemctl is-active ssh 2>/dev/null || true
    else
        log "$IFACE ist nicht aktiv."
    fi
}

case "${1:-toggle}" in
    start)   start_server ;;
    stop)    stop_server ;;
    restart) stop_server; start_server ;;
    status)  show_status ;;
    toggle)
        if [ -d "$RUN_DIR" ]; then
            stop_server
        else
            start_server
        fi
        ;;
    *) echo "Verwendung: $0 [start|stop|restart|status]" >&2; exit 2 ;;
esac
