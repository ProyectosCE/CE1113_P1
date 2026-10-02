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
6. elimina firmas y los primeros 10 MiB anteriores;
7. escribe la imagen completa y sincroniza los datos.

La operación destruye la tarjeta seleccionada. Confirme siempre modelo y tamaño.
