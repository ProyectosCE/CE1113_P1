# Imágenes Yocto importantes

| Imagen | Uso |
| --- | --- |
| `ce1113-p1` | Producto final compuesto por `packagegroup-ce1113`. |
| `core-image-minimal` | Diagnóstico base de Poky, sin software CE1113. |
| `rpi-test-image` | Diagnóstico del BSP Raspberry Pi; solo `raspberrypi4`. |

`ce1113-p1` parte de `core-image-minimal` y genera `ext4`, `tar.bz2` y
`wic.bz2`. Los packagegroups `hw`, `webapp`, `api` y `auraapp` incorporan los
mismos componentes que antes estaban distribuidos entre capas funcionales.

Artefactos importantes:

- `.manifest`: lista exacta de paquetes, usada por `check_image.sh`.
- `.wic.bz2`: disco arrancable completo, usado por `flash_sd.sh`.
- `.ext4`: rootfs aislado; no basta para arrancar una SD.
- `.tar.bz2`: extracción manual del rootfs.

Se generan en `<poky>/build-<máquina>/tmp/deploy/images/<máquina>/` y nunca se
suben a Git.
