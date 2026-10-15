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
    BOOT[multi-user.target] --> WIFI[aurabot-wifi.service]
    WIFI --> DHCP[aurabot-wifi-dhcp.service]
```

Los servicios de audio, robot, web, Wi-Fi y DHCP usan
`Restart=on-failure`. `StartLimitBurst` evita ciclos de reinicio ilimitados en
fallos permanentes. Los procesos permanecen en primer plano para que systemd
observe directamente su PID y registre stdout/stderr en el journal.

`aurabot-wifi.service` ejecuta `wpa_supplicant` en modo cliente y
`aurabot-wifi-dhcp.service` mantiene `udhcpc` en primer plano para obtener y
renovar la dirección entregada por el router. La configuración de la Raspberry
Pi 4 asume la interfaz `wlan0`.

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
actualizarse juntos los servicios Wi-Fi y DHCP.
