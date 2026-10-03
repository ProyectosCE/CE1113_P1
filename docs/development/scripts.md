# Scripts del proyecto

`POKY_DIR` puede señalar el árbol Poky. Su valor predeterminado es
`$HOME/poky-scarthgap-5.0.19`.

## `build.sh`

Limpia referencias a capas antiguas, activa `meta-raspberrypi` y `meta-ce1113`,
construye la única combinación `ce1113-p1`/`raspberrypi4` y verifica el resultado.

```bash
./build.sh --no-ui
./build.sh --no-ui --build-dir build-raspberrypi4
```

Reutiliza el build, descargas y sstate existentes. `--skip-check` omite la
validación final solo para diagnóstico; no debe usarse antes de flashear.

## `check_image.sh`

Comprueba manifest, paquetes obligatorios, módulo del jack, lectura completa del
WIC, SHA-256 y una huella del contenido de la metadata activa. Devuelve un código
de error si falla cualquiera de esas reglas. No compara fechas de archivos,
porque Git puede cambiarlas aunque BitBake determine que el contenido es igual.

```bash
./check_image.sh
./check_image.sh --no-ui --report /tmp/integridad.txt
./check_image.sh --no-ui --build-dir /ruta/build-raspberrypi4
```

## `flash_sd.sh`

Vuelve a ejecutar la verificación antes de borrar y solo acepta un disco completo
removible, USB o MMC que no sea el disco raíz.

```bash
./flash_sd.sh
./flash_sd.sh --device /dev/sdX
```

La operación destruye todos los datos. `--yes` omite la confirmación, pero no
las comprobaciones de integridad, tipo de dispositivo o protección del disco raíz.

## `load_audio.sh`

Copia MP3 directamente a la partición persistente `AURA_AUDIO`; no modifica la
imagen ni ejecuta BitBake. Busca por defecto `./media` y después `./audios`:

```bash
./load_audio.sh
./load_audio.sh --source /ruta/a/mis-audios
./load_audio.sh --source ./audios --partition /dev/mmcblk0p3 --no-ui
```

Conserva las canciones existentes, reemplaza únicamente nombres coincidentes y
regenera `/media/audio/playlist.txt`. Verifica etiqueta, ext4, espacio disponible,
disco removible y que el destino no pertenezca al sistema host.

## `scripts/check-repository.sh`

Es la entrada estable para desarrollo y CI. Comprueba estructura, categorías,
packagegroups, nombres, `.bbappend`, ubicación de fuentes, documentación y
sintaxis. `validate-repository.sh` se conserva como implementación compatible.

## `scripts/create-recipe.sh`

Funciona como asistente o sin interacción:

```bash
./scripts/create-recipe.sh
./scripts/create-recipe.sh --category auraapp --name mi-panel --version 1.0.0
```

Flujo recomendado:

```text
check-repository.sh → build.sh → flash_sd.sh → load_audio.sh
```
