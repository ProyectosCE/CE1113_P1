#!/bin/sh

### BEGIN INIT INFO
# Provides:          wifi-init
# Required-Start:    $local_fs
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO

CONFIG_FILE="/etc/wpa_supplicant/wpa_supplicant.conf"

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
        ip link set "$INTERFACE" up

        wpa_supplicant -B -i "$INTERFACE" -c "$CONFIG_FILE"
        sleep 3

        echo "Solicitando DHCP"
        udhcpc -i "$INTERFACE" -b -q
        ;;

    stop)
        killall wpa_supplicant
        ;;

    restart)
        "$0" stop
        sleep 2
        "$0" start
        ;;

    *)
        echo "Uso: $0 {start|stop|restart}"
        exit 1
        ;;
esac

exit 0