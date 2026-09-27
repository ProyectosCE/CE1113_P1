# Integración y validación de audio en Raspberry Pi 4

## Estado

La reproducción por el conector analógico de 3.5 mm fue validada en la
Raspberry Pi 4 con la imagen `ce1113-p1`. La tarjeta `Headphones` aparece en
ALSA y el archivo MP3 de prueba puede reproducirse desde la interfaz web.

La imagen validada se generó con Poky Scarthgap 5.0.19 para la máquina
`raspberrypi4`. La compilación terminó correctamente con 5533 tareas.

## Causa del problema

La configuración de audio y los programas de usuario no eran suficientes para
crear la tarjeta analógica. La versión histórica donde funcionaba el audio se
había probado en el contexto de `rpi-test-image`, basada en
`core-image-base`. La imagen actual `ce1113-p1` se basa en
`core-image-minimal` y no incorporaba automáticamente todo el soporte de la
máquina.

La diferencia importante no era únicamente `mpg123` ni la presencia de las
capas `meta-oe`, `meta-python` y `meta-multimedia`. Faltaban el conjunto
completo de módulos y reglas de la máquina Raspberry Pi que normalmente llega
mediante `core-image-base`.

Para conservar intacta la receta principal de `meta-ce1113`, el soporte se
restauró desde `meta-aura-apps`:

```bitbake
IMAGE_INSTALL:append:raspberrypi4 = " \
    packagegroup-machine-base \
    packagegroup-base-alsa \
"
```

El manifiesto de la imagen resultante confirmó:

- `packagegroup-machine-base`;
- `packagegroup-base-alsa`;
- `kernel-modules`;
- `udev-rules-rpi`;
- `alsa-state`;
- `kernel-module-snd-bcm2835`.

La imagen contenía 1758 paquetes `kernel-module-*`. Esto eliminó la diferencia
de soporte de máquina existente entre la imagen mínima y la imagen histórica.

## Configuración de arranque y kernel

La capa `meta-aura-apps` añade para `raspberrypi4`:

```bitbake
RPI_EXTRA_CONFIG:append:raspberrypi4 = " \n \
dtparam=audio=on \n \
"

KERNEL_MODULE_AUTOLOAD:append:raspberrypi4 = " snd-bcm2835"
```

También se añaden a la línea de comandos del kernel:

```text
snd_bcm2835.enable_headphones=1 snd_bcm2835.enable_hdmi=0
```

El archivo `/etc/modprobe.d/snd-bcm2835.conf` conserva los mismos parámetros:

```text
options snd-bcm2835 enable_headphones=1 enable_hdmi=0
```

Esta configuración habilita explícitamente `Headphones` y evita que las
salidas HDMI creadas por el driver legado alteren la enumeración esperada.

El servicio temprano `/etc/init.d/aurabot-audio-kernel` ejecuta:

```sh
modprobe snd-bcm2835 enable_headphones=1 enable_hdmi=0
```

Luego verifica la aparición de `Headphones` en `/proc/asound/cards` y registra
el resultado mediante `logger`.

## Conflicto con PWM

El jack analógico de esta Raspberry Pi usa el bloque PWM. Por eso la prueba de
audio no debe activar simultáneamente el overlay de `meta-pwm` utilizado para
los motores.

La explicación detallada del periférico compartido, sus canales y las
alternativas de arquitectura se encuentra en
[Conflicto entre audio analógico y PWM](audio-pwm-conflict.md).

Durante la prueba se mantuvo PWM desactivado y se verificó que el `config.txt`
generado tuviera `dtparam=audio=on` sin ningún `dtoverlay=pwm`. El script
`build.sh` excluye `meta-pwm` de la selección normal; solo se vuelve a incluir
deliberadamente con `--enable-pwm`.

## Receta `audio-mp3`

La receta está en:

```text
layers/meta-aura-apps/recipes-apps/audio-mp3/audio-mp3_1.0.0.bb
```

Instala estos paquetes de ejecución:

```bitbake
RDEPENDS:${PN} = " \
    alsa-utils \
    alsa-utils-aplay \
    alsa-utils-amixer \
    alsa-utils-alsamixer \
    mpg123 \
"

RDEPENDS:${PN}:append:raspberrypi4 = " kernel-module-snd-bcm2835"
```

También instala:

- `/usr/share/aurabot/audio/test.mp3`;
- `/etc/modprobe.d/snd-bcm2835.conf`;
- `/etc/init.d/aurabot-audio-kernel`.

`PACKAGECONFIG:append:pn-mpg123 = " alsa"` garantiza que `mpg123` use la
salida ALSA.

## Implementación en C

Se reutilizaron y ampliaron las bibliotecas existentes de AuraBot.

`libaudio` administra un proceso persistente de `mpg123` en modo remoto:

```sh
mpg123 -R -o alsa -a DISPOSITIVO
```

Los comandos `LOAD`, `PAUSE`, `STOP` y `QUIT` se envían mediante un FIFO
privado. La salida de diagnóstico de `mpg123` queda en `/tmp/mpg123.log`.

`libaurabot_hw` expone las operaciones:

- `aurabot_audio_play_file()`;
- `aurabot_audio_pause()`;
- `aurabot_audio_stop()`;
- `aurabot_audio_set_device()`.

No se fija `plughw:2,0`, porque el número de tarjeta cambia al habilitar HDMI,
USB u otros controladores. El dispositivo seleccionado se expresa mediante el
identificador ALSA estable, por ejemplo `plughw:Headphones,0`.

## Servidor de audio y SysVinit

El ejecutable `/usr/bin/aurabot-audio-server` crea el FIFO:

```text
/run/aurabot-audio/control
```

Acepta estos comandos:

```text
PLAY
PAUSE
STOP
DEVICE N
VOLUME N
```

`PLAY` reproduce `/usr/share/aurabot/audio/test.mp3`. `DEVICE N` consulta
`/proc/asound/cardN/id`, valida el identificador y configura
`plughw:IDENTIFICADOR,0`. `VOLUME N` acepta valores de 0 a 100 y ajusta el
control ALSA `PCM` de la tarjeta elegida mediante `amixer`.

El servicio `/etc/init.d/aurabot-audio` se inicia en los niveles 2, 3, 4 y 5.
Ejecuta `/usr/sbin/aurabot-audio-supervisor` sin terminal. El supervisor:

1. busca una tarjeta cuyo identificador contenga `Headphones`;
2. exporta `AURABOT_AUDIO_DEVICE`;
3. inicia `aurabot-audio-server`;
4. lo reinicia después de dos segundos si termina inesperadamente.

Se puede fijar manualmente el PCM en `/etc/default/aurabot-audio`:

```sh
AURABOT_AUDIO_DEVICE="plughw:Headphones,0"
```

## Integración CGI y página web

El CGI `/www/cgi-bin/operaciones.cgi` comunica la interfaz web con el FIFO del
servidor. Las operaciones disponibles son:

- `audio-devices`: enumera `/proc/asound/cards`;
- `audio-device&card=N`: selecciona una tarjeta;
- `audio-play`;
- `audio-pause`;
- `audio-stop`.
- `audio-volume&volume=N`: establece el volumen entre 0 y 100%.

La página permite actualizar la lista, seleccionar la salida, controlar el
volumen del jack y ejecutar Play, Pausa y Stop. La enumeración dinámica evita
depender del orden `card0`, `card1` o `card2`.

BusyBox `httpd` se ejecuta mediante el servicio SysVinit `webapp-httpd`. Su
supervisor equivale a:

```sh
httpd -p 80 -h /www
```

Se añade `-f` internamente para mantener el proceso en primer plano ante el
supervisor, mientras el servicio completo se ejecuta en segundo plano y se
reinicia si `httpd` falla.

## Flujo completo

```text
Navegador
  -> BusyBox httpd
  -> operaciones.cgi
  -> /run/aurabot-audio/control
  -> aurabot-audio-server
  -> libaurabot_hw
  -> libaudio
  -> mpg123
  -> ALSA
  -> snd-bcm2835
  -> jack 3.5 mm
```

## Construcción

```sh
./build.sh --no-ui --machine raspberrypi4 --image ce1113-p1
```

La imagen generada queda en el directorio de despliegue de Poky:

```text
tmp/deploy/images/raspberrypi4/ce1113-p1-raspberrypi4.rootfs.wic.bz2
```

## Comprobaciones en la Raspberry Pi

Detectar tarjetas y dispositivos PCM:

```sh
cat /proc/asound/cards
aplay -l
aplay -L
```

Comprobar el módulo y los parámetros:

```sh
lsmod | grep snd_bcm2835
cat /sys/module/snd_bcm2835/parameters/enable_headphones
cat /sys/module/snd_bcm2835/parameters/enable_hdmi
```

Comprobar servicios:

```sh
/etc/init.d/aurabot-audio-kernel status
/etc/init.d/aurabot-audio status
/etc/init.d/webapp-httpd status
```

Prueba directa sin la web:

```sh
aplay -D plughw:Headphones,0 /usr/share/sounds/alsa/Front_Center.wav
mpg123 -o alsa -a plughw:Headphones,0 /usr/share/aurabot/audio/test.mp3
```

Si la primera muestra de sonido no está instalada, la prueba con `mpg123` es
suficiente.

Enviar comandos directamente al servidor:

```sh
printf 'PLAY\n' > /run/aurabot-audio/control
printf 'PAUSE\n' > /run/aurabot-audio/control
printf 'STOP\n' > /run/aurabot-audio/control
```

Consultar registros:

```sh
dmesg | grep -Ei 'snd|audio|bcm2835|vchiq'
logread | grep -E 'aurabot-audio|webapp-httpd'
cat /tmp/mpg123.log
```

## Archivos relevantes

| Función | Archivo |
| --- | --- |
| Configuración global de la capa | `layers/meta-aura-apps/conf/layer.conf` |
| Inclusión en la imagen | `layers/meta-aura-apps/recipes-core/images/ce1113-p1.bbappend` |
| Parámetros del kernel | `layers/meta-aura-apps/recipes-bsp/bootfiles/rpi-cmdline.bbappend` |
| Paquetes, módulo y MP3 | `layers/meta-aura-apps/recipes-apps/audio-mp3/audio-mp3_1.0.0.bb` |
| Configuración modprobe | `layers/meta-aura-apps/recipes-apps/audio-mp3/files/snd-bcm2835.conf` |
| Diagnóstico temprano | `layers/meta-aura-apps/recipes-apps/audio-mp3/files/aurabot-audio-kernel.init` |
| Receta de la aplicación | `layers/meta-aura-apps/recipes-apps/aurabot/aurabot_1.0.0.bb` |
| Servidor C | `layers/meta-aura-apps/recipes-apps/aurabot/files/src/aurabot-audio-server.c` |
| Servicio persistente | `layers/meta-aura-apps/recipes-apps/aurabot/files/aurabot-audio.init` |
| Supervisor | `layers/meta-aura-apps/recipes-apps/aurabot/files/aurabot-audio-supervisor` |
| Backend `mpg123` | `layers/meta-aura-apps/recipes-apps/aurabot/files/libaudio/lib/libaudio.c` |
| API de audio | `layers/meta-aura-apps/recipes-apps/aurabot/files/libaurabot_hw/lib/audio.c` |
| CGI | `layers/meta-server/recipes-web/webapp/files/operaciones.cgi.c` |
| Interfaz web | `layers/meta-server/recipes-web/webapp/files/index.html` y `app.js` |

## Resultado final

La solución quedó validada de extremo a extremo: soporte de máquina, driver del
kernel, tarjeta ALSA analógica, reproducción MP3, servidor persistente,
selección dinámica de dispositivo y control desde el navegador. La causa
principal era la pérdida del soporte completo de máquina al pasar de una imagen
basada en `core-image-base` a `core-image-minimal`, combinada con la necesidad
de habilitar y cargar explícitamente `snd-bcm2835`.
