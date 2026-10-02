Sí se puede mantener un único `meta-ce1113` sin convertir `layer.conf` en un archivo enorme ni tener que modificarlo cada vez que se agrega o elimina una receta.

La idea central sería:

- `layer.conf` solo descubre recetas y declara la capa.
- Las recetas contienen su configuración local.
- Los packagegroups agrupan paquetes por funcionalidad.
- Las imágenes deciden qué grupos instalar.
- `DISTRO_FEATURES` y `MACHINE_FEATURES` controlan decisiones que afectan globalmente la compilación.
- `PACKAGECONFIG` controla opciones internas de una receta.
- Las incompatibilidades se validan explícitamente.

# 1\. Estructura propuesta

```
meta-ce1113/
├── conf/
│   ├── layer.conf
│   ├── distro/
│   │   ├── ce1113.conf
│   │   └── include/
│   │       └── ce1113-features.inc
│   └── machine/
│       └── include/
│           └── ce1113-rpi4.inc
│
├── classes-recipe/
│   └── ce1113-image.bbclass
│
├── recipes-core/
│   ├── images/
│   │   ├── ce1113-image-common.inc
│   │   ├── ce1113-p1.bb
│   │   ├── ce1113-p1-minimal.bb
│   │   └── ce1113-p1-debug.bb
│   ├── packagegroups/
│   │   ├── packagegroup-ce1113-core.bb
│   │   ├── packagegroup-ce1113-audio.bb
│   │   ├── packagegroup-ce1113-web.bb
│   │   ├── packagegroup-ce1113-network.bb
│   │   └── packagegroup-ce1113-development.bb
│   └── busybox/
│       └── busybox_%.bbappend
│
├── recipes-apps/
│   ├── aurabot/
│   ├── audio-mp3/
│   └── app-operaciones/
│
├── recipes-connectivity/
│   ├── wifi-config/
│   └── brcmfmac-firmware-fix/
│
├── recipes-web/
│   └── webapp/
│
├── recipes-bsp/
│   └── bootfiles/
│       └── rpi-cmdline.bbappend
│
└── recipes-kernel/
    └── linux/
        └── linux-raspberrypi_%.bbappend
```

Las carpetas `recipes-core`, `recipes-web`, `recipes-apps`, etc., son categorías organizativas. Para BitBake todas son descubiertas por el patrón genérico de `layer.conf`.

# 2\. `layer.conf` no necesita cambiar

Puede conservarse prácticamente así:

```
BBPATH .= ":${LAYERDIR}"

BBFILES += " \
    ${LAYERDIR}/recipes-*/*/*.bb \
    ${LAYERDIR}/recipes-*/*/*.bbappend \
"

BBFILE_COLLECTIONS += "ce1113"
BBFILE_PATTERN_ce1113 = "^${LAYERDIR}/"
BBFILE_PRIORITY_ce1113 = "6"

LAYERVERSION_ce1113 = "1"
LAYERDEPENDS_ce1113 = "core raspberrypi"
LAYERSERIES_COMPAT_ce1113 = "scarthgap"
```

Con estos comodines, puedes crear:

```
recipes-audio/
recipes-web/
recipes-robots/
recipes-sensors/
recipes-connectivity/
```

sin modificar `layer.conf`.

También puedes eliminar cualquiera de esas carpetas sin modificarlo.

Solo sería necesario cambiar `LAYERDEPENDS_ce1113` si una receta comienza a depender de otra capa externa, por ejemplo `meta-oe` o `meta-python`. Eso sí es una dependencia real de la capa y debe declararse.

# 3\. Qué debe ir en una receta normal `.bb`

Una receta representa una unidad instalable.

Por ejemplo:

```
recipes-apps/aurabot/aurabot_1.0.0.bb
recipes-web/webapp/webapp_1.0.bb
recipes-connectivity/wifi-config/wifi-config_1.0.bb
```

Cada receta debe declarar:

- Fuentes.
- Dependencias de compilación.
- Dependencias de ejecución.
- Archivos instalados.
- Servicios SysVinit.
- Configuración específica de ese componente.

Ejemplo:

```
SUMMARY = "Servidor web de CE1113"
LICENSE = "MIT"

SRC_URI = " \
    file://webapp-httpd.init \
    file://webapp-httpd-supervisor \
    file://index.html \
"

inherit update-rc.d

RDEPENDS:${PN} += "busybox"

INITSCRIPT_NAME = "webapp-httpd"
INITSCRIPT_PARAMS = "defaults 80"

do_install() {
    install -d ${D}/www
    install -d ${D}${sysconfdir}/init.d

    install -m 0644 ${WORKDIR}/index.html ${D}/www/index.html
    install -m 0755 ${WORKDIR}/webapp-httpd.init \
        ${D}${sysconfdir}/init.d/webapp-httpd
}
```

Al eliminar esta receta, desaparece su funcionalidad sin necesidad de cambiar `layer.conf`.

Naturalmente, también hay que eliminarla del packagegroup o imagen que la instalaba.

# 4\. Recetas de configuración separadas

No todo tiene que incluirse dentro de la receta principal de una aplicación.

Puedes separar:

```
aurabot
aurabot-audio-config
aurabot-web-config
ce1113-network-config
```

Por ejemplo:

```
recipes-config/audio/ce1113-audio-config_1.0.bb
```

Esta receta podría instalar únicamente:

```
/etc/modprobe.d/snd-bcm2835.conf
/etc/init.d/aurabot-audio-kernel
/etc/default/aurabot-audio
```

Ejemplo:

```
SUMMARY = "Configuración del audio analógico de CE1113"
LICENSE = "MIT"

SRC_URI = " \
    file://snd-bcm2835.conf \
    file://aurabot-audio-kernel.init \
"

inherit update-rc.d

INITSCRIPT_NAME = "aurabot-audio-kernel"
INITSCRIPT_PARAMS = "start 07 S ."

RDEPENDS:${PN}:append:raspberrypi4 = " kernel-module-snd-bcm2835"

do_install() {
    install -d ${D}${sysconfdir}/modprobe.d
    install -d ${D}${sysconfdir}/init.d

    install -m 0644 ${WORKDIR}/snd-bcm2835.conf \
        ${D}${sysconfdir}/modprobe.d/snd-bcm2835.conf

    install -m 0755 ${WORKDIR}/aurabot-audio-kernel.init \
        ${D}${sysconfdir}/init.d/aurabot-audio-kernel
}
```

Esto mantiene separadas:

- La aplicación.
- Los archivos MP3.
- La configuración ALSA.
- La configuración del kernel.
- Los servicios.

# 5\. Packagegroups

Un packagegroup es una receta `.bb` que agrupa otros paquetes. No compila una aplicación; describe una funcionalidad.

## Packagegroup base

```
recipes-core/packagegroups/packagegroup-ce1113-core.bb
```

```
SUMMARY = "Componentes básicos de CE1113"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    aurabot \
    app-operaciones \
"
```

## Packagegroup de audio

```
recipes-core/packagegroups/packagegroup-ce1113-audio.bb
```

```
SUMMARY = "Audio analógico y reproducción MP3 de CE1113"
LICENSE = "MIT"

inherit packagegroup features_check

REQUIRED_DISTRO_FEATURES = "ce1113-audio-jack"

RDEPENDS:${PN} = " \
    audio-mp3 \
    ce1113-audio-config \
    alsa-utils-aplay \
    alsa-utils-amixer \
    alsa-utils-alsamixer \
    mpg123 \
    kernel-module-snd-bcm2835 \
"
```

## Packagegroup web

```
SUMMARY = "Interfaz web de CE1113"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    webapp \
    busybox \
"
```

## Packagegroup de red

```
SUMMARY = "Conectividad de CE1113"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    wifi-config \
    brcmfmac-firmware-fix \
"
```

## Ventaja

La imagen queda muy clara:

```
IMAGE_INSTALL:append = " \
    packagegroup-ce1113-core \
    packagegroup-ce1113-audio \
    packagegroup-ce1113-web \
    packagegroup-ce1113-network \
"
```

Si se elimina toda la funcionalidad web, solo se quita:

```
packagegroup-ce1113-web
```

No es necesario modificar `layer.conf`.

# 6\. Diferencia entre packagegroup e `IMAGE_FEATURES`

Ambos pueden instalar conjuntos de paquetes, pero tienen responsabilidades diferentes.

## Packagegroup

Es una agrupación explícita de paquetes:

```
packagegroup-ce1113-audio
    ├── audio-mp3
    ├── mpg123
    ├── alsa-utils-amixer
    └── ce1113-audio-config
```

Es la opción más directa y fácil de mantener.

## `IMAGE_FEATURES`

Permite seleccionar características por nombre:

```
IMAGE_FEATURES += "ce1113-web ce1113-audio"
```

Primero hay que declarar esos nombres como válidos:

```
IMAGE_FEATURES[validitems] += " \
    ce1113-web \
    ce1113-audio \
    ce1113-network \
"

FEATURE_PACKAGES:ce1113-web = "packagegroup-ce1113-web"
FEATURE_PACKAGES:ce1113-audio = "packagegroup-ce1113-audio"
FEATURE_PACKAGES:ce1113-network = "packagegroup-ce1113-network"
```

Esto puede colocarse en una clase común:

```
classes-recipe/ce1113-image.bbclass
```

```
IMAGE_FEATURES[validitems] += " \
    ce1113-web \
    ce1113-audio \
    ce1113-network \
    ce1113-development \
"

FEATURE_PACKAGES:ce1113-web = "packagegroup-ce1113-web"
FEATURE_PACKAGES:ce1113-audio = "packagegroup-ce1113-audio"
FEATURE_PACKAGES:ce1113-network = "packagegroup-ce1113-network"
FEATURE_PACKAGES:ce1113-development = "packagegroup-ce1113-development"
```

La imagen usaría:

```
inherit core-image ce1113-image

IMAGE_FEATURES += " \
    ce1113-web \
    ce1113-audio \
    ce1113-network \
"
```

## Limitación importante

`IMAGE_FEATURES` es adecuado para decidir paquetes de una imagen, pero no debe usarse para cambiar globalmente cómo se compila el kernel, `rpi-config`, BusyBox o una receta externa.

Cada receta se analiza en su propio contexto. Una opción declarada solamente en `ce1113-p1.bb` no necesariamente estará disponible mientras BitBake procesa `linux-raspberrypi` o `rpi-config`.

Para configuración global de compilación se deben usar `DISTRO_FEATURES` o `MACHINE_FEATURES`.

# 7\. `DISTRO_FEATURES`

Se utiliza para características globales del sistema operativo.

Ejemplos:

```
ce1113-audio-jack
ce1113-pwm-software
ce1113-pwm-hardware
ce1113-web
ce1113-wifi
```

En:

```
conf/distro/ce1113.conf
```

se puede partir de Poky:

```
require conf/distro/poky.conf

DISTRO = "ce1113"
DISTRO_NAME = "CE1113 Poky Distribution"
DISTRO_VERSION = "1.0"

DISTRO_FEATURES:append = " \
    ce1113-audio-jack \
    ce1113-pwm-software \
    ce1113-web \
    ce1113-wifi \
"
```

Luego el build seleccionaría:

```
DISTRO = "ce1113"
```

## Configuración condicional de recetas

Ejemplo para `mpg123`:

```
PACKAGECONFIG:append:pn-mpg123 = " \
    ${@bb.utils.contains('DISTRO_FEATURES', \
        'ce1113-audio-jack', ' alsa', '', d)} \
"
```

Ejemplo para instalar un packagegroup:

```
IMAGE_INSTALL:append = " \
    ${@bb.utils.contains('DISTRO_FEATURES', \
        'ce1113-audio-jack', \
        ' packagegroup-ce1113-audio', '', d)} \
"
```

Ejemplo para seleccionar PWM por software:

```
IMAGE_INSTALL:append = " \
    ${@bb.utils.contains('DISTRO_FEATURES', \
        'ce1113-pwm-software', \
        ' packagegroup-ce1113-pwm-software', '', d)} \
"
```

## Cuándo usar `DISTRO_FEATURES`

Cuando una opción afecta varias recetas:

- Kernel.
- Bootloader.
- BusyBox.
- ALSA.
- Servicios.
- Packagegroups.
- Configuración de arranque.
- Backend común de todo el sistema.

# 8\. `MACHINE_FEATURES`

Debe representar capacidades del hardware, no elecciones generales de software.

Ejemplos apropiados:

```
wifi
bluetooth
sound
usbhost
```

Para hardware específico de CE1113 podrían existir:

```
ce1113-jack
ce1113-motor-controller
ce1113-hardware-pwm
```

Una configuración de máquina podría añadir:

```
MACHINE_FEATURES:append = " \
    ce1113-jack \
    ce1113-motor-controller \
"
```

Una receta puede exigir una capacidad:

```
inherit features_check

REQUIRED_MACHINE_FEATURES = "ce1113-jack"
```

O impedir una:

```
CONFLICT_MACHINE_FEATURES = "ce1113-hardware-pwm"
```

## Distinción recomendada

```
MACHINE_FEATURES = el hardware existe
DISTRO_FEATURES  = el sistema operativo lo habilita
IMAGE_FEATURES   = la imagen instala la interfaz o herramientas
```

Ejemplo:

```
MACHINE_FEATURES: ce1113-jack
DISTRO_FEATURES:  ce1113-audio-jack
IMAGE_FEATURES:   ce1113-audio-tools
```

# 9\. `PACKAGECONFIG`

`PACKAGECONFIG` habilita o deshabilita opciones dentro de una receta específica.

Ejemplo en `mpg123`:

```
PACKAGECONFIG:append:pn-mpg123 = " alsa"
```

Ejemplo en una receta propia:

```
PACKAGECONFIG ??= "audio web"

PACKAGECONFIG[audio] = "-DENABLE_AUDIO=ON,-DENABLE_AUDIO=OFF,alsa-lib"
PACKAGECONFIG[web] = "-DENABLE_WEB=ON,-DENABLE_WEB=OFF,libmicrohttpd"
```

CMake recibiría automáticamente las opciones correspondientes.

También puede depender de una feature global:

```
PACKAGECONFIG ??= " \
    ${@bb.utils.contains('DISTRO_FEATURES', \
        'ce1113-audio-jack', 'audio', '', d)} \
    ${@bb.utils.contains('DISTRO_FEATURES', \
        'ce1113-web', 'web', '', d)} \
"
```

Utiliza `PACKAGECONFIG` cuando la variación pertenece a una receta, no a toda la imagen.

# 10\. Archivos `.inc`

Un `.inc` permite compartir configuración sin crear otra receta.

Ejemplo:

```
recipes-core/images/ce1113-image-common.inc
```

```
IMAGE_INSTALL:append = " \
    packagegroup-ce1113-core \
"

IMAGE_LINGUAS = ""

IMAGE_ROOTFS_EXTRA_SPACE = "131072"
```

Imágenes distintas pueden reutilizarlo:

```
# ce1113-p1.bb
require ce1113-image-common.inc

IMAGE_FEATURES += " \
    ce1113-audio \
    ce1113-web \
    ce1113-network \
"
```

```
# ce1113-p1-minimal.bb
require ce1113-image-common.inc

IMAGE_FEATURES = ""
```

```
# ce1113-p1-debug.bb
require ce1113-image-common.inc

IMAGE_FEATURES += " \
    ce1113-audio \
    ce1113-web \
    ce1113-network \
    ce1113-development \
    debug-tweaks \
"
```

Los `.inc` también sirven para compartir contenido entre recetas relacionadas:

```
aurabot-common.inc
aurabot_1.0.0.bb
aurabot-tools_1.0.0.bb
```

# 11\. Clases `.bbclass`

Una clase encapsula comportamiento reutilizable.

Ejemplo:

```
classes-recipe/ce1113-image.bbclass
```

Puede contener:

- Definición de `IMAGE_FEATURES`.
- Verificación de combinaciones incompatibles.
- Paquetes comunes.
- Funciones auxiliares.
- Políticas de las imágenes CE1113.

Las imágenes usarían:

```
inherit core-image ce1113-image
```

Esto evita repetir lógica en cada imagen.

No conviene usar una clase para datos muy específicos de una sola receta. Para eso es mejor un `.inc` o la propia receta.

# 12\. Imágenes separadas

Es la separación más clara cuando se necesitan productos finales diferentes.

Ejemplos:

```
ce1113-p1.bb
ce1113-p1-minimal.bb
ce1113-p1-development.bb
ce1113-p1-audio.bb
ce1113-p1-motors.bb
```

Podrían compartir:

```
require ce1113-image-common.inc
```

Ejemplo de imagen de producción:

```
SUMMARY = "Imagen de producción CE1113"

require ce1113-image-common.inc

IMAGE_FEATURES += " \
    ce1113-audio \
    ce1113-web \
    ce1113-network \
"
```

Ejemplo de desarrollo:

```
SUMMARY = "Imagen de desarrollo CE1113"

require ce1113-image-common.inc

IMAGE_FEATURES += " \
    ce1113-audio \
    ce1113-web \
    ce1113-network \
    ce1113-development \
    debug-tweaks \
"
```

Esto permite construir:

```
bitbake ce1113-p1
bitbake ce1113-p1-minimal
bitbake ce1113-p1-development
```

# 13\. Exclusión explícita de funcionalidades

Hay varios niveles de exclusión.

## No instalar el packagegroup

La forma más limpia:

```
IMAGE_INSTALL:remove = "packagegroup-ce1113-web"
```

O simplemente no añadirlo.

## Exclusión condicional por feature

```
IMAGE_INSTALL:append = " \
    ${@bb.utils.contains('DISTRO_FEATURES', \
        'ce1113-web', ' packagegroup-ce1113-web', '', d)} \
"
```

Si `ce1113-web` no está presente, no se instala.

## Eliminar un paquete concreto

```
IMAGE_INSTALL:remove = "webapp"
```

Esto no funcionará si otro paquete tiene una dependencia obligatoria `RDEPENDS` sobre `webapp`.

## Evitar recomendaciones

Para paquetes que llegan mediante `RRECOMMENDS`:

```
BAD_RECOMMENDATIONS += "paquete-no-deseado"
```

Esto no elimina dependencias obligatorias.

## Exclusión absoluta

```
PACKAGE_EXCLUDE += "paquete-no-permitido"
```

Si un packagegroup o receta depende obligatoriamente de él, la construcción fallará. Esto es útil como protección porque evita que el paquete aparezca silenciosamente.

## Ocultar recetas

```
BBMASK += "/recipes-web/webapp/"
```

Esto impide que BitBake vea la receta. Es útil para depuración o políticas especiales, pero no debería ser el mecanismo normal para seleccionar funcionalidades.

# 14\. Incompatibilidades explícitas

Si en el futuro dos componentes no pueden coexistir, conviene proteger la configuración en varios niveles.

## `RCONFLICTS`

Para impedir que dos paquetes entren en el mismo rootfs:

```
# ce1113-audio-jack-config.bb
RCONFLICTS:${PN} = "ce1113-pwm-hardware-config"
```

Y en el otro:

```
# ce1113-pwm-hardware-config.bb
RCONFLICTS:${PN} = "ce1113-audio-jack-config"
```

Si ambos terminan en la imagen, el gestor de paquetes produce un error.

Esto protege la instalación, pero no necesariamente evita que ambas recetas se compilen.

## `CONFLICT_DISTRO_FEATURES`

```
inherit features_check

REQUIRED_DISTRO_FEATURES = "ce1113-audio-jack"
CONFLICT_DISTRO_FEATURES = "ce1113-pwm-hardware"
```

Para PWM hardware:

```
inherit features_check

REQUIRED_DISTRO_FEATURES = "ce1113-pwm-hardware"
CONFLICT_DISTRO_FEATURES = "ce1113-audio-jack"
```

## Validación en la imagen

La clase `ce1113-image.bbclass` puede detener la construcción inmediatamente:

```
python __anonymous() {
    features = set((d.getVar("DISTRO_FEATURES") or "").split())

    conflicts = [
        (
            "ce1113-audio-jack",
            "ce1113-pwm-hardware",
            "El jack analógico y PWM por hardware usan el mismo bloque PWM"
        ),
    ]

    for first, second, reason in conflicts:
        if first in features and second in features:
            bb.fatal(
                "Configuración CE1113 inválida: '%s' y '%s' no pueden "
                "habilitarse simultáneamente. %s"
                % (first, second, reason)
            )
}
```

Así la compilación falla con un mensaje comprensible, no posteriormente con un error ambiguo del kernel o del rootfs.

## Matriz central de conflictos

Se pueden añadir futuros conflictos sin tocar cada receta:

```
conflicts = [
    ("ce1113-audio-jack", "ce1113-pwm-hardware",
     "Comparten PWM0/PWM1"),

    ("ce1113-spi-display", "ce1113-spi-sensor",
     "Usan el mismo chip-select"),

    ("ce1113-uart-console", "ce1113-uart-device",
     "Usan la misma UART"),

    ("ce1113-i2c-device-a", "ce1113-i2c-device-b",
     "Tienen la misma dirección I2C"),
]
```

# 15\. Configuración actual: jack más PWM por software

Como ahora ambos pueden coexistir, las features predeterminadas podrían ser:

```
DISTRO_FEATURES:append = " \
    ce1113-audio-jack \
    ce1113-pwm-software \
    ce1113-web \
    ce1113-wifi \
"
```

La incompatibilidad solo se declararía contra PWM por hardware:

```
ce1113-audio-jack     + ce1113-pwm-software → permitido
ce1113-audio-jack     + ce1113-pwm-hardware → prohibido
ce1113-audio-usb      + ce1113-pwm-hardware → permitido
```

La política puede expresarse así:

| Audio | PWM | Resultado |
|---|---|---|
| Jack analógico | Software | Permitido |
| Jack analógico | Hardware BCM2835 | Prohibido |
| USB | Hardware BCM2835 | Permitido |
| HDMI | Hardware BCM2835 | Permitido |

# 16\. Recomendación concreta

Para este repositorio usaría:

1. Un solo `meta-ce1113`.
2. Un `layer.conf` pequeño y estable.
3. Una receta por componente.
4. Recetas de configuración separadas cuando la configuración no sea parte natural de la aplicación.
5. Packagegroups para:
   - core;
   - audio;
   - web;
   - red;
   - desarrollo;
   - PWM.
6. `IMAGE_FEATURES` para seleccionar herramientas y grupos de paquetes de cada imagen.
7. `DISTRO_FEATURES` para decisiones globales como backend de audio o tipo de PWM.
8. `MACHINE_FEATURES` para describir capacidades físicas.
9. `PACKAGECONFIG` para variantes internas de una receta.
10. Una clase `ce1113-image.bbclass` para validar incompatibilidades.
11. `RCONFLICTS` como segunda protección dentro del rootfs.
12. Imágenes separadas únicamente cuando realmente representan productos finales distintos.

Así se pueden añadir o eliminar carpetas `recipes-*` sin tocar `layer.conf`, y las configuraciones incompatibles fallan explícitamente durante BitBake en lugar de producir problemas silenciosos en la Raspberry Pi.