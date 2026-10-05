# Integridad y grabación de la imagen

El producto admite únicamente `ce1113-p1` para `raspberrypi4`. Los scripts no
ofrecen targets ni imágenes ajenos al producto.

## Comprobar

```bash
./check_image.sh
```

Con `whiptail` y una terminal, el reporte aparece en una ventana desplazable;
con `--no-ui` se imprime en stdout. Se selecciona el manifest versionado más
reciente y el WIC que comparte exactamente su nombre base, evitando mezclar
artefactos de compilaciones distintas.

La comprobación incluye:

1. existencia del manifest y `.wic`, `.wic.bz2`, `.wic.gz` o `.wic.xz`;
2. lectura o descompresión completa del artefacto;
3. SHA-256, tamaño y cantidad de paquetes;
4. packagegroup raíz, cuatro grupos de área y paquetes obligatorios;
5. `kernel-module-snd-bcm2835-*` para el jack analógico;
6. comparación de una huella SHA-256 del contenido completo de `meta-ce1113`.

La huella no utiliza fechas de modificación. Esto evita marcar como obsoleta
una imagen después de `git checkout`, `stash`, `clone` o una restauración que
cambie los tiempos de archivo sin cambiar su contenido. `build.sh` escribe la
huella junto al manifest después de que BitBake termina correctamente.
Si la huella falta, la comprobación falla: una ejecución directa de `bitbake`
no se considera suficiente para autorizar el flasheo; use `build.sh`.

Para guardar evidencia:

```bash
./check_image.sh --no-ui --report /tmp/ce1113-integridad.txt
```

## Grabar

```bash
./flash_sd.sh
```

La GUI selecciona solamente el dispositivo. Antes de escribir, el script:

1. ejecuta toda la comprobación de integridad;
2. calcula y muestra el SHA-256 del origen;
3. confirma que el destino sigue conectado y es removible;
4. solicita confirmación explícita y credenciales `sudo`;
5. desmonta las particiones;
6. comprueba que el WIC no alcance los últimos 2 GiB;
7. escribe la imagen del sistema sin limpiar los bloques finales;
8. relee los bytes escritos y compara su SHA-256 con el WIC descomprimido;
9. comprueba que reaparezcan las particiones 1 y 2 del WIC;
10. restaura o crea como partición 3 el ext4 `AURA_AUDIO` al final de la SD;
11. verifica que las geometrías de arranque y rootfs no hayan cambiado y que
    sus firmas sigan siendo FAT y ext4.

En el primer flash se crea y formatea `AURA_AUDIO`. En flashes posteriores se
guarda su inicio/tamaño antes de escribir el WIC y se restaura la misma entrada
en la tabla sin formatear, por lo que las canciones permanecen intactas. La
imagen monta la partición por etiqueta en `/media/audio` mediante SysVinit. El script se detiene
si la imagen crece hasta solapar la partición o si la geometría no es segura.
La partición raíz conserva exactamente el tamaño definido y probado dentro del
WIC. El espacio intermedio queda sin asignar deliberadamente: no se redimensiona
un rootfs en vivo durante el flasheo y los últimos 2 GiB permanecen reservados
para música.

Las particiones de arranque y rootfs se reemplazan. La partición musical solo se
preserva si es la partición 3, ext4 y tiene la etiqueta exacta `AURA_AUDIO`.

Las canciones nunca forman parte del WIC. Después de flashear se cargan sin
recompilar mediante `./load_audio.sh --source ./media`.

## Si falla la lectura después de grabar

No vuelva a escribir repetidamente la SD. Con la SD desmontada, puede comparar
sus primeros bytes con el WIC de la imagen actual sin modificar sus particiones:

```bash
./flash_sd.sh --device /dev/mmcblk0 --verify-only
./flash_sd.sh --device /dev/mmcblk0 --verify-only --direct-read
```

La segunda variante usa E/S directa para evitar la caché y lectura anticipada.
Si aparece `Invalid argument`, el lector podría no soportar este modo; no se
reintenta ni se cambia de modo automáticamente. Ambas verificaciones comparan
el SHA-256 de exactamente el tamaño del WIC usando bloques de 4 MiB, incluido un
último bloque parcial. Se rechazan errores de E/S, lecturas cortas y hashes distintos.

`--verify-only` no desmonta, graba, formatea ni restaura `AURA_AUDIO`. Una
comparación correcta no equivale a un flasheo completo: si un intento anterior
falló antes de restaurar p3, todavía falta esa etapa. Si fallan ambas lecturas,
pruebe otro lector/adaptador o tarjeta; no se pueden corregir sectores ilegibles
ignorando los errores del dispositivo. Los audios pueden seguir presentes aunque
falte p3: no formatee esa zona.

Las pruebas del lector usan archivos temporales y errores simulados, sin SD:

```bash
bash scripts/test-flash-readback.sh
```
