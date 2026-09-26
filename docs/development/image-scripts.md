# Verificar y grabar imágenes con GUI

## Verificar una imagen

```bash
./check_image.sh
```

La interfaz permite elegir `qemuarm64` o `raspberrypi4` y después la imagen.
Busca exclusivamente en `build-<target>/tmp/deploy/images/<target>`, muestra el
manifest, artefactos y presencia de los paquetes propios.

## Grabar una tarjeta SD

```bash
./flash_sd.sh
```

La interfaz selecciona target, imagen y dispositivo. Solo muestra discos de tipo
removible, USB o MMC, y excluye el disco que contiene `/`. Antes de escribir:

1. verifica completamente el archivo `.wic.bz2`, `.wic.gz` o `.wic.xz`;
2. vuelve a comprobar que el dispositivo siga conectado;
3. solicita confirmación explícita y credenciales `sudo`;
4. desmonta sus particiones;
5. elimina firmas y los primeros 10 MiB de la tabla anterior;
6. escribe la imagen completa y sincroniza los datos.

La operación destruye el contenido de la tarjeta seleccionada. Confirme siempre
modelo y tamaño en la ventana antes de aceptar.
