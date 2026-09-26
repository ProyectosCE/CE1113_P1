# Añadir funcionalidades a CE1113

`meta-ce1113` no recibe recetas nuevas: conserva exclusivamente la imagen original.
Toda función, esencial o separable, va en una capa `layers/meta-<módulo>`. Para
cambiar una receta de Poky/BSP use `.bbappend`, no una copia. Fuentes y
configuraciones instaladas viven en `files/`.

```text
layers/meta-ejemplo/
├── conf/layer.conf
├── recipes-ejemplo/mi-servicio/
│   ├── files/{CMakeLists.txt,main.c}
│   └── mi-servicio_1.0.0.bb
└── recipes-core/images/ce1113-p1.bbappend
```

La receta declara `SUMMARY`, `LICENSE`, `LIC_FILES_CHKSUM`, `SRC_URI` y `S`.
Para CMake use `inherit cmake`; Yocto ya proporciona toolchain y sysroot.
El `layer.conf` usa una colección única y contiene:

```bitbake
LAYERDEPENDS_ejemplo = "core ce1113"
LAYERSERIES_COMPAT_ejemplo = "scarthgap"
```

La instalación en el producto se agrega, sin duplicar la imagen:

```bitbake
# recipes-core/images/ce1113-p1.bbappend
IMAGE_INSTALL:append = " mi-servicio"
```

Para una receta existente, nombre el append como el destino y exponga archivos:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "file://fragment.cfg"
```

Use `%` (`busybox_%.bbappend`) solo si el cambio soporta todas las versiones.
Compruebe cada cambio con:

```bash
./scripts/validate-repository.sh
./build.sh -m qemuarm64 -i ce1113-p1 -l meta-aura-apps,meta-operaciones,meta-server --no-ui
bitbake-layers show-layers
bitbake-layers show-appends
./check_image.sh -m qemuarm64
```
