# Estructura del repositorio

```text
CE1113_P1/
├── .github/workflows/validation.yml
├── docs/{architecture,development,yocto}/
├── meta-ce1113/                    # única capa activa del producto
│   ├── conf/layer.conf
│   ├── recipes-core/               # imagen y packagegroups
│   ├── recipes-lib/                # libgpio, PWM, audio, LEDs y sensores
│   ├── recipes-test/               # aplicaciones de prueba
│   ├── recipes-hw/                 # activación/configuración física de la RPi
│   └── recipes-auraapp/            # app, servidor de audio y webapp
├── layers/meta-pwm/                # referencia experimental, fuera del build
├── scripts/
│   ├── check-repository.sh         # entrada estable local/CI
│   ├── validate-repository.sh      # implementación de las reglas
│   ├── create-recipe.sh
│   └── lib/image-common.sh         # selección e integridad de artefactos
├── build.sh
├── check_image.sh
└── flash_sd.sh
```

`meta-ce1113` contiene toda la metadata activa. La imagen `ce1113-p1` instala
únicamente `packagegroup-ce1113`; este compone los grupos `lib`, `test`, `hw`
y `auraapp`. Añadir una aplicación requiere modificar solo el packagegroup de
su área, nunca la receta de imagen ni `layer.conf`.

## Categorías

| Categoría | Responsabilidad |
|---|---|
| `recipes-lib` | Bibliotecas dinámicas `gpio`, `pwm`, `audio`, `leds` y `sensors` |
| `recipes-test` | Pruebas aisladas: `app-operaciones` y `prueba-encoder` |
| `recipes-hw` | Kernel, bootfiles, firmware, Wi-Fi, audio físico y montaje de música |
| `recipes-auraapp` | Aplicación principal, servidor de audio, CGI, HTTP y frontend |

El contenido de las categorías funcionales es cerrado:

```text
recipes-lib/      {libgpio,pwm,audio,leds,sensors}
recipes-test/     {app-operaciones,prueba-encoder}
recipes-auraapp/  {aurabot,aurabot-audio-server,webapp}
recipes-hw/       {audio-config,audio-storage,bootfiles,
                   brcmfmac-firmware-fix,busybox,linux,wifi-config}
```

Las aplicaciones no vuelven a compilar implementaciones de hardware. Las
cadenas de uso de bibliotecas dinámicas son:

```text
aurabot -> libpwm.so.1 -> libgpio.so.1
aurabot-audio-server -> libaudio.so.1
webapp -> FIFO /run/aurabot-audio/control -> servidor -> libaudio.so.1
```

La webapp no abre ALSA directamente: delega la reproducción al servidor para
que exista un solo dueño del dispositivo de audio. PWM, LEDs y sensores
consumen `libgpio.so` como dependencia de receta.

Las ampliaciones de BusyBox también tienen dueño único. Wi-Fi se habilita desde
`recipes-hw/busybox`; HTTP/CGI se habilita desde `recipes-auraapp/webapp`.

Yocto/RPM aplica nombres basados en SONAME a los paquetes finales (`libgpio1`,
`libpwm1`, `libaudio1`, `libleds1`, `libsensors1`). Por ello el packagegroup de
bibliotecas usa `${TUNE_PKGARCH}` y no `allarch`; las recetas y dependencias de
compilación conservan sus nombres `libgpio`, `pwm`, `audio`, `leds`, `sensors`.

Una receta normal se registra en el packagegroup de su área. Una receta de
configuración puede ser dependencia de la receta que la consume si no representa
una funcionalidad seleccionable. Los `.bbappend` se reservan para recetas
externas de Poky o `meta-raspberrypi`.

El código instalado vive en `files/` junto a su receta, permitiendo que BitBake
calcule hashes reproducibles. No se admite C/CMake suelto en la raíz. Los
directorios `build-*`, `downloads` y `sstate-cache` son resultados locales.

## Reglas comprobadas

- Existe una sola imagen: `ce1113-p1` para `raspberrypi4`.
- Todas las recetas usan minúsculas y nombres permitidos.
- No existen `.bbappend` de la imagen; la composición usa packagegroups.
- Los cuatro packagegroups (`lib`, `test`, `hw`, `auraapp`) forman el producto;
  no existen grupos heredados `api` o `webapp`.
- Solo `meta-ce1113` se activa; `meta-pwm` permanece excluida.
- El código C/CMake se encuentra dentro de `files/` de una receta.
- No se versionan builds, cachés, logs ni archivos mayores de 10 MiB.
- Build y flash invocan la comprobación de integridad.

Ejecute `./scripts/check-repository.sh` antes de abrir un PR.
