# Scripts de compilación, comprobación y grabación

`POKY_DIR` puede definir el árbol Poky; el valor predeterminado es
`$HOME/poky-scarthgap-5.0.19`.

## `build.sh`

Configura el build, mantiene activa `meta-ce1113`, selecciona capas y ejecuta
BitBake. En terminal puede mostrar menús; para automatización use `--no-ui`.

```bash
./build.sh -m raspberrypi4 -i ce1113-p1 -l all --no-ui
./build.sh -m qemuarm64 -i ce1113-p1 -l none --no-ui
./build.sh -m raspberrypi4 -l meta-red,meta-server --no-ui
```

Requiere Poky Scarthgap y, para RPi, `meta-raspberrypi` dentro de Poky. El build
predeterminado es `<poky>/build-<máquina>`. `meta-red` depende del BSP y se omite
de `--layers all` en QEMU; pedirla explícitamente fuera de RPi es un error.

## `check_image.sh`

Lee el manifest del build/máquina exactos, muestra artefactos y paquetes propios:

```bash
./check_image.sh -m raspberrypi4 -i ce1113-p1
./check_image.sh -m qemuarm64 -b /ruta/build-qemuarm64
```

`[--]` puede indicar una capa deliberadamente no seleccionada.

## `flash_sd.sh`

Graba el `.wic.bz2`/`.wic` más reciente. Exige el disco completo y rechaza por
defecto dispositivos no removibles:

```bash
lsblk -o NAME,SIZE,MODEL,TRAN,RM,MOUNTPOINTS
./flash_sd.sh --device /dev/sdX
```

La operación destruye todo el dispositivo. Nunca use una partición como
`/dev/sdX1`. `--yes` omite la confirmación y `--force` admite discos no marcados
como removibles; ambos se reservan para casos controlados.

Flujo recomendado: `build.sh` → `check_image.sh` → `flash_sd.sh`.
