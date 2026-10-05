#!/bin/sh

### BEGIN INIT INFO
# Provides:          wifi-init
# Required-Start:    $local_fs
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO

CONFIG_FILE="/etc/wpa_supplicant/wpa_supplicant.conf"
WPA_PIDFILE="/run/wifi-wpa.pid"
DHCP_PIDFILE="/run/wifi-dhcp.pid"

pid_running()
{
    [ -s "$1" ] && kill -0 "$(cat "$1")" 2>/dev/null
}

stop_pid()
{
    if pid_running "$1"; then
        kill "$(cat "$1")" 2>/dev/null
    fi
    rm -f "$1"
}

find_wifi()
{
    INTERFACE=""

    for i in $(seq 1 10); do
        INTERFACE=$(ls /sys/class/net | grep '^wl' | head -n1)

        if [ -n "$INTERFACE" ]; then
            break
        fi

        sleep 1
    done
}

case "$1" in
    start)
        find_wifi

        if [ -z "$INTERFACE" ]; then
            echo "No existe interfaz WiFi"
            exit 1
        fi

        echo "Inicializando WiFi en $INTERFACE"
        ip link set "$INTERFACE" up || exit 1

        if ! pid_running "$WPA_PIDFILE"; then
            wpa_supplicant -B -P "$WPA_PIDFILE" -f /var/log/wifi-wpa.log \
                -i "$INTERFACE" -c "$CONFIG_FILE" || exit 1
        fi
        attempts=0
        while [ "$attempts" -lt 30 ]; do
            if wpa_cli -i "$INTERFACE" status 2>/dev/null | grep -q '^wpa_state=COMPLETED'; then
                break
            fi
            attempts=$((attempts + 1))
            sleep 1
        done
        if [ "$attempts" -eq 30 ]; then
            echo "WiFi aun sin asociacion: revise wpa_cli status y /var/log/wifi-wpa.log"
            logger -t wifi-init "sin asociacion tras 30 s; DHCP esperara en segundo plano"
        fi

        echo "Solicitando DHCP"
        if ! pid_running "$DHCP_PIDFILE"; then
            udhcpc -i "$INTERFACE" -p "$DHCP_PIDFILE" -b \
                >>/var/log/wifi-dhcp.log 2>&1 || exit 1
        fi
        ;;

    stop)
        stop_pid "$DHCP_PIDFILE"
        stop_pid "$WPA_PIDFILE"
        ;;

    restart)
        "$0" stop
        sleep 2
        "$0" start
        ;;

    status)
        find_wifi
        [ -n "$INTERFACE" ] || exit 1
        wpa_cli -i "$INTERFACE" status
        ip -4 addr show dev "$INTERFACE"
        ;;

    *)
        echo "Uso: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac

exit 0
