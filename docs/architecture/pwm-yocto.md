# Soporte PWM por hardware de AuraBot

## Alcance

La capa `layers/meta-pwm` prepara la imagen Yocto de Raspberry Pi 4 para que una
biblioteca de usuario pueda controlar los dos canales PWM mediante
`/sys/class/pwm`. No instala bibliotecas de GPIO, no modifica las bibliotecas C
de AuraBot y no incorpora `meta-openembedded`.

## Configuración implementada

La compilación objetivo usa Yocto Scarthgap, `meta-raspberrypi`, el kernel
Raspberry Pi 6.6 y `MACHINE = "raspberrypi4"` (ARM de 32 bits).

El fragmento `layers/meta-pwm/recipes-kernel/linux/files/pwm.cfg` solicita:

```text
CONFIG_PWM=y
CONFIG_PWM_SYSFS=y
CONFIG_PWM_BCM2835=y
```

El controlador se integra en el kernel para no instalar ni cargar el paquete
`kernel-module-pwm-bcm2835` en la imagen mínima.

`layers/meta-pwm/conf/layer.conf` añade `overlays/pwm-2chan.dtbo` a
`RPI_KERNEL_DEVICETREE_OVERLAYS` y genera esta entrada en `config.txt`:

```text
dtoverlay=pwm-2chan,pin=12,func=4,pin2=13,func2=4
```

La correspondencia prevista es:

| Canal del controlador | GPIO BCM | Pin físico | Función |
|---|---:|---:|---|
| PWM0 | 12 | 32 | ALT0 (`func=4`) |
| PWM1 | 13 | 33 | ALT0 (`func=4`) |

Los dos canales comparten el reloj del bloque PWM. Para AuraBot ambos deben
usar el mismo periodo de 1 000 000 ns (1000 Hz).

## Compilación y comprobaciones en el host

La compilación normal se inicia desde el repositorio, seleccionando
`raspberrypi4`, `CE1113-P1` y la capa `meta-pwm` en los menús:

```sh
./build.sh
```

Después de que el script inicialice el entorno, las variables efectivas pueden
comprobarse desde el directorio de Poky con:

```sh
source oe-init-build-env build-raspberrypi4
bitbake -e CE1113-P1 | grep -E '^(MACHINE|TARGET_ARCH|RPI_KERNEL_DEVICETREE_OVERLAYS|KERNEL_DEVICETREE|RPI_EXTRA_CONFIG)='
```

Comprobar la configuración producida, sin asumir la ruta exacta del directorio
de trabajo:

```sh
find tmp/work -path '*linux-raspberrypi/*/*build/.config' -exec \
    grep -H -E '^CONFIG_PWM(=|_SYSFS=|_BCM2835=)' {} \;
find tmp/deploy/images/raspberrypi4 -maxdepth 1 -name 'pwm-2chan.dtbo' -ls
```

Para inspeccionar el contenido real de la partición de arranque de la imagen
sin modificar sus artefactos se puede usar `wic ls` y `wic cp`, o montar una
copia de la imagen descomprimida. Deben encontrarse:

```text
/overlays/pwm-2chan.dtbo
dtoverlay=pwm-2chan,pin=12,func=4,pin2=13,func2=4
```

Los artefactos finales se generan en:

```text
tmp/deploy/images/raspberrypi4/
```

## Audio y conflictos de pines

El audio analógico de Raspberry Pi utiliza el mismo bloque PWM y entra en
conflicto con `pwm-2chan`. Un nombre ALSA como `plughw:2,0` no identifica de
forma estable el hardware. En la placa se debe comprobar antes de conectar los
motores:

```sh
cat /proc/asound/cards
cat /proc/asound/pcm
for card in /sys/class/sound/card*; do
    printf '%s: ' "$card"
    cat "$card/id" 2>/dev/null
done
```

HDMI y USB no usan el generador PWM analógico. No se modifica la configuración
de audio automáticamente. GPIO17 (sensor IR digital) y GPIO27 (LED) no
colisionan con GPIO12 y GPIO13.

## Despliegue seguro

Antes de grabar la imagen, crear una copia de la microSD funcional desde otro
equipo, reemplazando los dispositivos por los identificadores verificados con
`lsblk`:

```sh
sudo umount /dev/sdX1 /dev/sdX2
sudo dd if=/dev/sdX of=aurabot-sd-respaldo.img bs=4M status=progress conv=fsync
sync
```

Después se puede grabar el `.wic.bz2` con el script del proyecto o con:

```sh
bzcat CE1113-P1-raspberrypi4.rootfs.wic.bz2 | \
    sudo dd of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

`/dev/sdX` debe validarse cuidadosamente; usar un dispositivo incorrecto
destruye sus datos.

## Validación en la Raspberry Pi

Confirmar arquitectura, arranque y Device Tree:

```sh
uname -a
uname -m
cat /proc/device-tree/model; echo
grep -nE '^(dtoverlay|dtparam)=' /boot/config.txt
find /boot -name 'pwm-2chan.dtbo' -print
dmesg | grep -i pwm
```

Descubrir los controladores PWM sin asumir `pwmchip0`:

```sh
for chip in /sys/class/pwm/pwmchip*; do
    [ -e "$chip" ] || continue
    printf '\n%s\n' "$chip"
    readlink -f "$chip/device"
    printf 'npwm='
    cat "$chip/npwm"
    printf 'driver='
    basename "$(readlink -f "$chip/device/driver")"
    tr -d '\0' < "$chip/device/of_node/compatible" 2>/dev/null; echo
done
```

El chip correcto debe exponer dos canales (`npwm=2`) y estar asociado al
controlador PWM BCM2835/Raspberry Pi. Registrar el nombre real de `pwmchipN`;
su índice global no forma parte de una ABI estable.

Antes de probar, comprobar que ningún canal está exportado o reclamado y que
GPIO12/GPIO13 son los pines conectados. Para probar PWM0 a 1000 Hz y 25 %,
reemplazar `pwmchipN` por el chip descubierto:

```sh
PWMCHIP=/sys/class/pwm/pwmchipN
test "$(cat "$PWMCHIP/npwm")" -ge 2
test ! -e "$PWMCHIP/pwm0"

echo 0 > "$PWMCHIP/export"
while [ ! -e "$PWMCHIP/pwm0/period" ]; do sleep 0.1; done
echo 1000000 > "$PWMCHIP/pwm0/period"
echo 250000 > "$PWMCHIP/pwm0/duty_cycle"
echo 1 > "$PWMCHIP/pwm0/enable"

# Medir GPIO12 (pin físico 32) y después detener y liberar el canal.
echo 0 > "$PWMCHIP/pwm0/enable"
echo 0 > "$PWMCHIP/pwm0/duty_cycle"
echo 0 > "$PWMCHIP/unexport"
```

La prueba necesita privilegios de escritura, normalmente una sesión `root`.
No debe conectarse un motor directamente a un GPIO: se necesita la etapa de
potencia adecuada y masa común.

## Contrato para la futura `libpwm.so`

La biblioteca debe enumerar `/sys/class/pwm/pwmchip*`, inspeccionar `npwm`, el
enlace `device/driver` y, cuando exista, `device/of_node/compatible`. No debe
codificar `pwmchip0`.

Una vez identificado el chip de dos canales:

- `pwm0` corresponde a PWM0/GPIO12/pin físico 32.
- `pwm1` corresponde a PWM1/GPIO13/pin físico 33.
- `export` y `unexport` reciben el número de canal, no el GPIO.
- `period` y `duty_cycle` se expresan en nanosegundos.
- Para 1000 Hz el periodo es 1 000 000 ns.
- El ciclo útil debe cumplir `0 <= duty_cycle <= period`.
- Se debe deshabilitar el canal antes de cambiar el periodo cuando el
  controlador lo requiera.
- Los dos canales deben conservar una frecuencia compatible porque comparten
  reloj.
- La aplicación necesita permisos sobre sysfs y debe manejar `EBUSY` cuando un
  canal ya esté exportado o reclamado.
- La numeración `pwmchipN` debe descubrirse en cada arranque.

La existencia del DTBO, la configuración final del kernel y el contenido de la
imagen pueden validarse en el host. La aparición del `pwmchip`, su índice, la
señal eléctrica y la ausencia definitiva de conflicto con audio requieren una
prueba sobre la Raspberry Pi física.
