# Contexto actual de trabajo — CE1113 Raspberry Pi 4

Fecha de actualización: 2 de octubre de 2026.

## Objetivo actual

Mantener una única imagen Yocto `ce1113-p1` para Raspberry Pi 4, con los
componentes organizados en recetas independientes y reutilizables. El trabajo
inmediato consiste en recuperar el PWM por software sobre GPIO17 sin regresar
al árbol monolítico anterior de AuraBot.

La arquitectura deseada para esta parte es:

```text
aurabot -> libpwm.so.1 -> libgpio.so.1 -> GPIO17 mediante sysfs
```

`libgpio` debe ocuparse únicamente del acceso GPIO. `libpwm` debe implementar
el PWM por software y depender de `libgpio`. La aplicación `aurabot` debe usar
la API pública de `libpwm`.

## Incidente del último flasheo

La última SD probada no llegó a arrancar la Raspberry Pi. La auditoría separó
este incidente del fallo de PWM:

- El WIC generado tiene una tabla MBR válida, partición FAT activa, rootfs ext4
  íntegro, firmware, kernel y `root=/dev/mmcblk0p2`.
- El script de flasheo escribía el WIC y luego ampliaba en vivo la partición 2 y
  su ext4 antes de crear `AURA_AUDIO`.
- El script no releía los bytes del dispositivo para comprobar el `dd` y
  tampoco validaba la geometría y firmas finales antes de declarar éxito.

La corrección conserva el rootfs exactamente como fue producido y validado por
WIC. Después de `dd`, compara el SHA-256 de los bytes leídos desde la SD contra
el WIC descomprimido. Solo entonces añade la partición 3, sin modificar las
particiones 1 y 2, y comprueba sus inicios, tamaños y tipos de filesystem.

## Estructura vigente

La capa de producto es `meta-ce1113`. La receta de imagen continúa en:

```text
meta-ce1113/recipes-core/images/ce1113-p1.bb
```

La composición se realiza mediante el packagegroup raíz y los packagegroups
por área:

```text
packagegroup-ce1113
├── packagegroup-ce1113-lib
├── packagegroup-ce1113-test
├── packagegroup-ce1113-hw
└── packagegroup-ce1113-auraapp
```

Las categorías modernas son:

- `recipes-lib`: bibliotecas GPIO, PWM, audio, LED y sensores.
- `recipes-test`: aplicaciones de prueba, como `app-operaciones`.
- `recipes-hw`: configuración física de Raspberry Pi, audio, almacenamiento,
  Wi-Fi, kernel, firmware y boot.
- `recipes-auraapp`: aplicación principal, servidor de audio y aplicación web.
- `recipes-core`: imagen y packagegroups.

La auditoría estructural del 2 de octubre dejó estas categorías como conjuntos
cerrados. Se eliminaron los directorios antiguos `recipes-api` y
`recipes-webapp`, así como sus packagegroups sin uso. La receta de imagen
`ce1113-p1.bb` no fue modificada: continúa instalando únicamente el packagegroup
raíz.

También se corrigieron dos responsabilidades cruzadas:

- La configuración ALSA de `mpg123` pasó a `recipes-lib/audio`.
- BusyBox Wi-Fi pertenece solo a `recipes-hw`; BusyBox HTTP/CGI pertenece solo
  a `recipes-auraapp/webapp`.

Las dependencias de ejecución quedan así:

```text
aurabot -> libpwm.so.1 -> libgpio.so.1
aurabot-audio-server -> libaudio.so.1
webapp -> FIFO de control -> aurabot-audio-server -> libaudio.so.1
```

## Implementación actual de GPIO y PWM

Las únicas fuentes activas de GPIO y PWM son:

```text
meta-ce1113/recipes-lib/libgpio/
├── libgpio_1.0.0.bb
└── files/
    ├── CMakeLists.txt
    ├── include/libgpio.h
    └── src/libgpio.c

meta-ce1113/recipes-lib/pwm/
├── pwm_1.0.0.bb
└── files/
    ├── CMakeLists.txt
    ├── include/libpwm.h
    ├── src/libpwm.c
    └── tests/
        ├── fake_gpio.c
        ├── fake_gpio.h
        └── test_pwm.c
```

La receta `pwm` declara:

```bitbake
DEPENDS = "libgpio"
```

La receta `aurabot` declara:

```bitbake
DEPENDS = "pwm"
```

La cadena ELF comprobada mediante una compilación nativa es:

```text
aurabot: necesita libpwm.so.1
libpwm:  necesita libgpio.so.1
```

## Funcionamiento confirmado en la Raspberry Pi

Se confirmó manualmente que GPIO17, el kernel y el cableado funcionan. La
siguiente secuencia encendió y apagó correctamente el LED:

```sh
echo 529 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio529/direction
echo 1 > /sys/class/gpio/gpio529/value
sleep 3
echo 0 > /sys/class/gpio/gpio529/value
```

En esta Raspberry Pi, el controlador tiene base 512 y por eso BCM GPIO17 se
representa como `gpio529`.

Esto descarta, para la prueba realizada:

- Un fallo físico del LED.
- Un error de cableado básico.
- La ausencia del controlador GPIO sysfs.
- Un número global incorrecto para GPIO17.

## Fallo observado

Las imágenes anteriores iniciaban `aurabot` y creaban un segundo hilo, pero el
LED no parpadeaba. El valor leído desde:

```text
/sys/class/gpio/gpio529/value
```

permanecía en `0`.

En una prueba también ocurrió que el directorio `gpio529` desapareció después
de iniciar AuraBot. Ese caso fue provocado por ejecutar `unexport` justo antes
del programa; la operación del kernel puede completarse de forma asíncrona.

El mensaje anterior:

```text
AuraBot: PWM activo en GPIO17 (2 Hz, 50%).
```

solo demostraba que `setPWM()` había retornado correctamente. No garantizaba
que todas las transiciones posteriores del hilo se realizaran.

## Referencia histórica

El commit usado como referencia funcional es:

```text
53fdd12a65b62f2392be09f6f06f210a0165257f
```

En ese commit, GPIO y PWM por software se compilaban juntos dentro de la receta
monolítica de AuraBot. La implementación usaba un hilo POSIX, `nanosleep()` y
escrituras a sysfs.

El código antiguo fue restaurado temporalmente para compararlo. La auditoría
detectó que la receta moderna no compilaba esas copias restauradas, porque su
`SRC_URI` solamente incluía el CMake principal y `src/main.c`. Por tanto,
restaurar archivos dentro de `aurabot/files/libgpio` no modificaba el binario
instalado.

Esas copias heredadas se retiraron nuevamente para evitar dos fuentes de
verdad. El código útil se migró a `recipes-lib/libgpio` y `recipes-lib/pwm`.

## Cambios recientes todavía pendientes de validar en hardware

La implementación moderna fue corregida de esta manera:

- `libgpio` usa `open()` y `write()` directamente sobre sysfs.
- Las escrituras incluyen salto de línea, como el comando `echo` probado.
- `pinMode()` exporta GPIO17 y espera específicamente el atributo `direction`,
  no solo el directorio que puede aparecer antes que sus archivos.
- `libpwm` realiza una primera escritura sincrónica antes de crear el hilo.
- El hilo reintenta la configuración si una escritura GPIO falla y no termina
  por un error transitorio; el diagnóstico se limita a una vez por racha para
  evitar llenar el log.
- Los períodos y tiempos alto/bajo se calculan con `int64_t`. En ARM de 32 bits,
  el cálculo anterior `period_ns * duty_percent` desbordaba a 2 Hz y 50%.
- `nanosleep()` reanuda el tiempo restante cuando recibe `EINTR`.
- Las fallas del nivel alto o bajo se informan por `stderr`.
- AuraBot comprueba el retorno de `setPWM()` y muestra un error si no inicia.
- Existe una prueba CTest con un GPIO falso que confirma transiciones, apagado
  limpio, temporización a 2 Hz y recuperación tras errores transitorios.

La compilación CMake de `libgpio`, `libpwm` y `aurabot` fue satisfactoria. El
validador del repositorio también terminó correctamente.

Todavía falta confirmar esta nueva implementación en la Raspberry Pi. No debe
considerarse resuelto el PWM hasta observar físicamente el LED y verificar que
el valor de GPIO17 alterna entre cero y uno.

## Limpieza realizada durante la migración

Se retiraron componentes heredados que duplicaban recetas modernas:

- `aurabot/files/libgpio`
- `aurabot/files/libaudio`
- `aurabot/files/libaurabot_hw`
- El servidor de audio antiguo dentro de la receta `aurabot`
- La receta antigua `recipes-api/app-operaciones`
- La receta antigua `audio-mp3`
- El MP3 incluido dentro de metadata Yocto

El audio de prueba permanece fuera de Yocto en `media/`. El archivo eliminado
`test.mp3` era idéntico a `media/lean.mp3`, por lo que no se perdió la canción.

## Imagen y audio externo

Los MP3 no forman parte de la imagen Yocto. Se almacenan en una tercera
partición ext4 de aproximadamente 2 GiB, etiquetada `AURA_AUDIO`. El script
`load_audio.sh` se utiliza para copiar canciones desde `media/` sin recompilar
la imagen.

La imagen instala actualmente el soporte ALSA necesario:

- `packagegroup-base-alsa`
- `alsa-utils-amixer`
- `mpg123`
- `kernel-module-snd-bcm2835-*`

No se requieren `alsa-utils-aplay` ni `alsa-utils-alsamixer` para la aplicación
actual.

## Acceso SSH temporal retirado

La receta temporal `ssh-access` y su dependencia de Dropbear fueron retiradas
para que `recipes-hw` contenga únicamente activación y configuración de hardware
físico. La imagen de producto ya no debe ofrecer acceso root sin contraseña.

## Script de flasheo

`flash_sd.sh` fue actualizado para:

- Conservar o crear la partición `AURA_AUDIO`.
- Verificar por lectura el SHA-256 del WIC realmente escrito.
- Limitar la ejecución de `partprobe` mediante `timeout`.
- Usar `blockdev --rereadpt` como alternativa.
- Esperar a `udev` con tiempo limitado.
- Detenerse si el kernel no puede releer la tabla.
- Esperar explícitamente la aparición de las particiones.
- Mantener intacta la geometría de las particiones 1 y 2 del WIC.
- Validar las firmas FAT de arranque y ext4 del rootfs antes de terminar.

`check_image.sh` ahora trata la ausencia de la huella de metadata como error.
Una ejecución manual de `bitbake` ya no produce por sí sola un artefacto que el
script permita flashear; se debe usar `build.sh`, que crea la huella después de
una compilación satisfactoria.

El script modifica particiones reales y debe recibir siempre el disco completo,
por ejemplo `/dev/mmcblk0`, nunca `/dev/mmcblk0p1`.

## Comandos para la siguiente compilación

La vía soportada genera la imagen y su huella de metadata en una sola operación:

```sh
cd ~/CE1113_P1
./build.sh --no-ui
```

Luego debe verificarse la imagen:

```sh
cd ~/CE1113_P1
./check_image.sh --no-ui
```

Solo se debe flashear si termina con:

```text
RESULTADO: ÍNTEGRA
```

El artefacto construido después de estas correcciones es:

```text
ce1113-p1-raspberrypi4.rootfs-20261003033939.wic.bz2
SHA-256: 4065622eda026d0bd16abd48fefa1d338ea49ac594e380fa5662aa11dd462743
```

Contiene 1860 paquetes y pasó `check_image.sh --no-ui` con
`RESULTADO: ÍNTEGRA`.

## Prueba pendiente en la Raspberry Pi

Después de flashear:

```sh
pkill -x aurabot 2>/dev/null
aurabot >/tmp/aurabot.log 2>&1 &
PID=$!
sleep 1

cat /tmp/aurabot.log
ls -1 "/proc/$PID/task"
cat /sys/class/gpio/gpio529/direction

for delay in 0.07 0.11 0.13 0.17 0.19 0.23; do
    printf 'delay=%s valor=' "$delay"
    cat /sys/class/gpio/gpio529/value
    sleep "$delay"
done
```

Resultados esperados:

- AuraBot permanece activo.
- Aparecen dos hilos en `/proc/$PID/task`.
- `direction` contiene `out`.
- El valor alterna entre `0` y `1`.
- El LED conectado a BCM GPIO17, pin físico 11, parpadea a 2 Hz.

No se debe ejecutar `unexport` inmediatamente antes de esta prueba.

## Trabajo que todavía falta

1. Flashear la imagen nueva con `flash_sd.sh` y exigir que muestre el hash de
   lectura verificado.
2. Confirmar en la Raspberry la cadena dinámica con `ldd /usr/bin/aurabot`.
3. Ejecutar la prueba de GPIO17 anterior.
4. Si falla, conservar `/tmp/aurabot.log` y registrar los hashes de
   `/usr/bin/aurabot`, `/usr/lib/libpwm.so.1.0.0` y
   `/usr/lib/libgpio.so.1.0.0`.

## Estado resumido

- Estructura moderna: implementada.
- Duplicados heredados: retirados.
- Compilación CMake y prueba automatizada de PWM: correctas.
- Validación del repositorio: correcta.
- Estructura de recetas: validada también por un build Yocto completo, sin
  recetas ni `.bbappend` huérfanos.
- Imagen nueva después de las correcciones: construida y verificada localmente.
- Prueba del nuevo flujo contra una imagen de disco simulada: correcta; conserva
  sin cambios las particiones 1 y 2 y añade `AURA_AUDIO` como partición 3.
- Prueba física de la última implementación PWM: pendiente.
- PWM declarado resuelto: no todavía.
