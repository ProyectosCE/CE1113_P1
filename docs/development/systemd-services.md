# Servicios systemd de AuraBot

`build.sh` selecciona `INIT_MANAGER = "systemd"`. Las recetas heredan la clase
`systemd`, instalan sus unidades en `${systemd_system_unitdir}` y las habilitan
durante la construcción de la imagen.

```mermaid
flowchart LR
    MOD[systemd-modules-load] --> AUDIO[aurabot-audio.service]
    MOUNT[media-audio.mount] --> AUDIO
    AUDIO --> ROBOT[aurabot.service]
    ROBOT --> WEB[webapp-httpd.service]
    BOOT[multi-user.target] --> TIMER[aurabot-wifi.timer]
    TIMER --> AP[aurabot-wifi.service]
    AP --> DHCP[aurabot-wifi-dhcp.service]
    AP --> DNS[aurabot-captive-dns.service]
```

Los servicios de audio, robot, web, AP, DHCP y DNS cautivo usan
`Restart=on-failure`. `StartLimitBurst` evita ciclos de reinicio ilimitados en
fallos permanentes. Los procesos permanecen en primer plano para que systemd
observe directamente su PID y registre stdout/stderr en el journal.

El temporizador espera cinco segundos desde el arranque y el servicio AP está
ordenado después de `multi-user.target`. Así, preparar el radio no bloquea el
camino crítico inicial. El AP usa `192.168.4.1/24`; BusyBox `udhcpd` entrega
direcciones `192.168.4.20`–`192.168.4.200` y el DNS cautivo dirige los nombres
al servidor web local. La configuración de la Raspberry Pi 4 asume la interfaz
`wlan0`.

Comandos de operación:

```sh
systemctl status aurabot.service
systemctl restart webapp-httpd.service
journalctl -b -u aurabot.service -f
systemctl show aurabot.service -p Restart -p NRestarts -p MemoryCurrent
systemd-analyze critical-chain webapp-httpd.service
```

La salida analógica se carga mediante `systemd-modules-load` y la configuración
`/etc/modprobe.d/snd-bcm2835.conf`. El servidor de audio detecta la tarjeta
`Headphones`; se puede fijar otra creando `/etc/default/aurabot-audio`:

```sh
AURABOT_AUDIO_DEVICE="plughw:Headphones,0"
```

La interfaz Wi-Fi configurada es `wlan0`; si el BSP usa otro nombre deben
actualizarse juntos el servicio AP, el servicio DNS y `udhcpd.conf`.
