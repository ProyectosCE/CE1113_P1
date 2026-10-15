# Añadir funcionalidades a CE1113

Toda la metadata activa vive en `meta-ce1113`. Las recetas se organizan en
cuatro categorías:

```text
recipes-lib      bibliotecas dinámicas reutilizables
recipes-test     aplicaciones y utilidades de prueba
recipes-hw       activación y configuración física de la Raspberry Pi
recipes-auraapp  aplicación, servidor de audio, CGI y frontend
```

`recipes-core` queda reservado para la imagen y los packagegroups.

## Añadir una receta

Una receta nueva se coloca en su categoría y conserva sus fuentes en `files/`:

```text
meta-ce1113/recipes-auraapp/mi-servicio/
├── files/
│   ├── CMakeLists.txt
│   └── main.c
└── mi-servicio_1.0.0.bb
```

La receta declara `SUMMARY`, `LICENSE`, `LIC_FILES_CHKSUM`, `SRC_URI` y `S`.
Para CMake use `inherit cmake`; Yocto proporciona toolchain y sysroot.

## Incorporarla al producto

No modifique `ce1113-p1.bb`. Añada el paquete al grupo de su área:

```bitbake
# recipes-core/packagegroups/packagegroup-ce1113-auraapp.bb
RDEPENDS:${PN} = " \
    aurabot \
    mi-servicio \
"
```

El flujo queda:

```text
ce1113-p1
  -> packagegroup-ce1113
     -> packagegroup-ce1113-auraapp
        -> mi-servicio
```

Una dependencia debe escribirse directamente en `RDEPENDS` de otra receta solo
cuando sea indispensable para que esa receta funcione. La decisión de incluir
componentes independientes pertenece a los packagegroups.

## Recetas de configuración

Separe una receta de configuración cuando instale políticas compartidas,
servicios de plataforma o archivos de `/etc` con ciclo de vida propio. Una
configuración exclusiva e inseparable de una aplicación puede permanecer en la
misma receta.

## Modificar recetas externas

Para cambiar Poky o `meta-raspberrypi`, use `.bbappend`, nunca una copia:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "file://fragment.cfg"
```

El nombre debe corresponder a la receta original:

```text
rpi-cmdline.bb       -> rpi-cmdline.bbappend
busybox_1.36.1.bb    -> busybox_%.bbappend
```

## Validación

```sh
./scripts/validate-repository.sh
./build.sh --no-ui --machine raspberrypi4 --image ce1113-p1
```
