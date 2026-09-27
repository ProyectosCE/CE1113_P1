# Por qué el audio analógico y el PWM de motores no pueden funcionar simultáneamente

## Resumen

En la Raspberry Pi 4 usada por AuraBot, el jack analógico de 3.5 mm y el
control PWM por hardware de los motores necesitan el mismo periférico PWM del
SoC BCM2711. No son dos dispositivos PWM independientes:

- el audio analógico usa los canales **PWM0 y PWM1** para producir los canales
  izquierdo y derecho;
- `meta-pwm` usa esos mismos canales **PWM0 y PWM1** y los enruta a GPIO12 y
  GPIO13 para controlar los motores.

Aunque las señales aparecen en pines físicos diferentes, proceden del mismo
bloque generador. El bloque, sus canales, su reloj, su FIFO y su configuración
no pueden ser controlados al mismo tiempo como audio y como dos señales PWM de
motores con frecuencias y ciclos útiles independientes.

Por ello, la configuración actual debe escoger uno de estos modos:

```text
Modo audio:   jack 3.5 mm disponible, meta-pwm desactivada
Modo motores: PWM0/PWM1 en GPIO12/GPIO13, jack 3.5 mm no disponible
```

## No es solamente un conflicto de pines

Es importante distinguir tres conceptos:

1. **Periférico PWM:** bloque electrónico dentro del BCM2711 que genera PWM.
2. **Canal PWM:** salida lógica PWM0 o PWM1 de ese periférico.
3. **GPIO:** pin al que se enruta una función alternativa del canal.

Un canal del periférico puede dirigirse a distintos GPIO mediante el
multiplexor de pines. Cambiar GPIO12 por GPIO18, por ejemplo, cambia el pin de
salida, pero continúa usando PWM0. No crea un canal nuevo.

El overlay oficial `pwm-2chan` enumera las rutas posibles:

| Canal | Posibles GPIO relevantes |
| --- | --- |
| PWM0 | GPIO12, GPIO18, GPIO40, GPIO52 |
| PWM1 | GPIO13, GPIO19, GPIO41, GPIO45, GPIO53 |

Por eso mover el motor de GPIO12 a GPIO18, o de GPIO13 a GPIO19, **no resuelve
el conflicto**. Los canales internos siguen siendo PWM0 y PWM1.

La documentación oficial del overlay indica expresamente que la salida
analógica integrada usa ambos canales PWM y que se debe tener cuidado al
combinarla con PWM. Véase el apartado `pwm`/`pwm-2chan` del
[README oficial de overlays de Raspberry Pi](https://github.com/raspberrypi/firmware/blob/master/boot/overlays/README).

## Cómo genera audio el jack de 3.5 mm

El jack no recibe audio digital directamente. Para producir una tensión
analógica, la Raspberry Pi utiliza los dos canales PWM a alta frecuencia:

```text
Muestras PCM de ALSA
        |
        v
snd-bcm2835 / firmware de Raspberry Pi
        |
        v
Periférico PWM del BCM2711
   PWM0         PWM1
     |            |
     v            v
Filtro/salida analógica estéreo
 izquierdo      derecho
        \        /
         jack 3.5 mm
```

La rápida secuencia PWM representa la señal de audio. La electrónica de la
placa filtra esa señal y entrega la forma de onda analógica al jack. Por eso
`dtparam=audio=on` y `snd-bcm2835` no se limitan a crear una tarjeta ALSA:
también habilitan una ruta que necesita el hardware PWM.

En la imagen funcional de AuraBot se configura:

```text
dtparam=audio=on
snd_bcm2835.enable_headphones=1
snd_bcm2835.enable_hdmi=0
```

`enable_headphones=1` hace que el driver materialice la tarjeta ALSA
`Headphones`. La reproducción con `mpg123` termina usando esa ruta PWM.

## Cómo usa PWM la capa de motores

La capa `layers/meta-pwm` habilita en el kernel:

```text
CONFIG_PWM=y
CONFIG_PWM_SYSFS=y
CONFIG_PWM_BCM2835=y
```

También aplica este overlay:

```text
dtoverlay=pwm-2chan,pin=12,func=4,pin2=13,func2=4
```

El resultado esperado es:

| Motor/canal | Canal interno | GPIO BCM | Pin físico | Uso |
| --- | --- | ---: | ---: | --- |
| Canal 0 | PWM0 | 12 | 32 | velocidad de un motor |
| Canal 1 | PWM1 | 13 | 33 | velocidad del otro motor |

La biblioteca `libgpio` configura los canales por `/sys/class/pwm`, ajustando:

- `period`;
- `duty_cycle`;
- `enable`.

AuraBot prevé una frecuencia de motores de 1000 Hz, equivalente a un periodo
de 1 000 000 ns. Esta configuración no coincide con el modo de operación que
el sistema de audio necesita para representar muestras de sonido.

## Recursos que entran en conflicto

### Los dos canales

El audio estéreo integrado usa PWM0 y PWM1. `pwm-2chan` también habilita PWM0
y PWM1. No queda un tercer canal libre en ese bloque para separar los usos.

### El reloj del bloque PWM

Los dos canales comparten la fuente de reloj del periférico. La reproducción
de audio necesita una temporización adecuada para la conversión de las
muestras, mientras los motores necesitan un periodo estable de 1000 Hz.
Reprogramar el reloj o el divisor para los motores altera la temporización del
audio; dejar la temporización de audio impide configurar libremente el periodo
requerido por los motores.

### Los registros de control

El driver de audio y `pwm-bcm2835` programan registros del mismo periférico.
Ambos esperan tener control sobre el modo, rango, datos, activación y estado de
los canales. Dos controladores independientes no pueden mantener
configuraciones contradictorias sobre los mismos registros.

### FIFO y flujo continuo

El audio es un flujo continuo de muestras. No es un ciclo útil fijo. El
contenido cambia constantemente para reconstruir la onda sonora. Un motor, en
cambio, espera una señal periódica con un ciclo útil relativamente estable que
representa su velocidad. La secuencia de audio no puede utilizarse como señal
válida de control para el motor, ni el PWM fijo del motor puede representar un
archivo MP3.

### Propiedad en Device Tree y kernel

Device Tree describe qué controlador posee y configura el hardware. Al
habilitar simultáneamente `dtparam=audio=on` y `pwm-2chan`, dos rutas intentan
usar el mismo recurso. Según el orden de inicialización y las versiones de
firmware/kernel, puede ocurrir que:

- la tarjeta `Headphones` no aparezca;
- `/sys/class/pwm/pwmchipN` no aparezca o no permita exportar canales;
- el controlador devuelva `EBUSY`;
- solo funcione uno de los canales;
- el audio se escuche distorsionado o deje de reproducirse;
- la señal de motor cambie de frecuencia o ciclo útil;
- el recurso que se inicializó de último interfiera con el primero.

No se debe considerar válido un arranque donde ambos dispositivos aparecen si
su operación simultánea no está garantizada. La presencia de nodos ALSA y
sysfs no convierte el periférico compartido en dos bloques independientes.

## Configuración concreta del repositorio

### Modo de audio validado

`layers/meta-aura-apps/conf/layer.conf` añade:

```bitbake
RPI_EXTRA_CONFIG:append:raspberrypi4 = " \n \
dtparam=audio=on \n \
"

KERNEL_MODULE_AUTOLOAD:append:raspberrypi4 = " snd-bcm2835"
```

El audio instala además el módulo `kernel-module-snd-bcm2835`, fuerza la salida
de auriculares y deshabilita las tarjetas HDMI del driver legado.

En este modo, `build.sh` omite `meta-pwm` de la selección normal de capas.

### Modo PWM explícito

`layers/meta-pwm/conf/layer.conf` añade `pwm-2chan.dtbo` y la asignación a
GPIO12/GPIO13. Esta capa solo se incluye deliberadamente con:

```sh
./build.sh --enable-pwm
```

Esta opción debe entenderse como un cambio al modo de motores, no como una
función adicional compatible con el jack analógico.

## Por qué algunas soluciones aparentes no funcionan

### «Usar otro índice ALSA»

Cambiar `plughw:2,0` por `plughw:0,0` solo elige una tarjeta ALSA. No cambia el
periférico físico que genera el audio analógico. Además, los índices ALSA no
son estables entre arranques.

### «Mover PWM a GPIO18 y GPIO19»

GPIO18 y GPIO19 son rutas alternativas para PWM0 y PWM1. Cambian los pines,
pero no los canales internos ni el reloj compartido.

### «Cargar primero el audio y luego el PWM»

El orden de carga solo decide qué controlador puede ganar o sobrescribir parte
de la configuración. No permite que ambos produzcan simultáneamente señales
independientes.

### «Usar un canal para audio y otro para motores»

La salida analógica estéreo necesita ambos canales y los canales comparten
reloj. Reducir audio a mono no convierte automáticamente la implementación de
`snd-bcm2835` en una solución compatible con `pwm-bcm2835`; exigiría un diseño
distinto y seguiría compartiendo recursos del bloque. No es una arquitectura
soportada por la configuración actual.

### «Instalar más paquetes ALSA»

`alsa-utils`, `mpg123` y los demás paquetes trabajan por encima del driver.
Pueden aportar herramientas o códecs, pero no duplican el periférico PWM del
SoC y no eliminan el conflicto de hardware.

## Alternativas para tener motores y audio a la vez

La solución real consiste en mover una de las dos funciones a hardware que no
use el mismo PWM.

### Opción recomendada: PWM externo para motores

Usar un controlador PWM externo, por ejemplo uno conectado por I2C, o un
microcontrolador dedicado a los motores:

```text
Raspberry Pi --I2C/SPI/UART--> controlador externo --> driver de potencia
          \
           `--> PWM interno --> jack 3.5 mm
```

Ventajas:

- conserva el jack actual;
- ofrece PWM estable para ambos motores;
- descarga trabajo de tiempo real de Linux;
- suele proporcionar más canales;
- separa mejor control y potencia.

### Audio por USB

Una tarjeta de sonido USB genera el audio fuera del PWM interno. Los motores
pueden conservar `pwm-2chan`. Requiere seleccionar la nueva tarjeta ALSA desde
la web y verificar alimentación/compatibilidad USB.

### DAC o códec I2S

Un DAC I2S externo evita el jack PWM integrado y normalmente ofrece mejor
calidad. Se necesita hardware adicional, un overlay/driver apropiado y revisar
que los GPIO de I2S no colisionen con otros periféricos del robot.

### Audio HDMI

HDMI no usa el generador PWM del jack analógico. Es válido técnicamente si el
destino final puede extraer o reproducir audio HDMI, aunque normalmente no es
práctico para un robot móvil.

### PWM por software

Es posible generar PWM por software en otros GPIO, pero Linux no garantiza
temporización estricta. Puede introducir fluctuación, consumo de CPU y ruido
en los motores. Solo debería considerarse después de medir la estabilidad bajo
carga; para control de motores se prefiere hardware dedicado.

## Cómo comprobar qué modo tiene una imagen

Revisar la configuración de arranque:

```sh
grep -nE '^(dtparam=audio|dtoverlay=pwm)' /boot/config.txt
```

Modo audio esperado:

```text
dtparam=audio=on
```

Sin una línea activa `dtoverlay=pwm` o `dtoverlay=pwm-2chan`.

Comprobar audio:

```sh
cat /proc/asound/cards
aplay -l
lsmod | grep snd_bcm2835
```

Comprobar PWM:

```sh
for chip in /sys/class/pwm/pwmchip*; do
    [ -e "$chip" ] || continue
    printf '%s: npwm=' "$chip"
    cat "$chip/npwm"
done
```

Comprobar mensajes de conflicto o inicialización:

```sh
dmesg | grep -Ei 'pwm|snd|audio|bcm2835|busy|resource'
```

## Regla para el proyecto

Mientras AuraBot utilice el jack de 3.5 mm de la Raspberry Pi 4 y PWM0/PWM1
para los dos motores, `meta-aura-apps` y `meta-pwm` representan modos de
hardware mutuamente excluyentes.

La regla de construcción es:

```text
Audio analógico activo  => no cargar pwm-2chan
pwm-2chan activo        => no depender del jack analógico
```

Para disponer de ambas funciones simultáneamente debe cambiarse la
arquitectura física: PWM externo para motores o una salida de audio que no use
el PWM interno.

## Referencias

- [Raspberry Pi firmware: documentación oficial de overlays](https://github.com/raspberrypi/firmware/blob/master/boot/overlays/README), apartados `pwm` y `pwm-2chan`.
- [Raspberry Pi 4 Model B Datasheet](https://datasheets.raspberrypi.com/rpi4/raspberry-pi-4-datasheet.pdf), descripción de la salida analógica de audio.
- [Choosing an Audio Option](https://pip-assets.raspberrypi.com/categories/1259/audio-camera-and-display/documents/RP-008124-WP/Choosing-an-Audio-option.pdf), alternativas oficiales de audio para placas Raspberry Pi.
- `layers/meta-pwm/conf/layer.conf`, configuración PWM concreta de AuraBot.
- `layers/meta-aura-apps/conf/layer.conf`, configuración del jack analógico.
- `docs/architecture/audio.md`, integración y validación del subsistema de audio.
- `docs/architecture/pwm-yocto.md`, integración y validación del subsistema PWM.
