# Imágenes Yocto importantes

| Imagen | Uso |
| --- | --- |
| `ce1113-p1` | Producto: imagen principal ampliada por los módulos seleccionados. |
| `core-image-minimal` | Diagnóstico base de Poky, sin software CE1113. |
| `rpi-test-image` | Diagnóstico del BSP Raspberry Pi; solo `raspberrypi4`. |

`ce1113-p1` parte de `core-image-minimal` y genera `ext4`, `tar.bz2` y
`wic.bz2`. `meta-aura-apps` añade `aurabot`, `meta-operaciones` añade `app-operaciones`,
`meta-red` añade Wi-Fi/firmware Broadcom y `meta-server` añade web/CGI.

Artefactos importantes:

- `.manifest`: lista exacta de paquetes, usada por `check_image.sh`.
- `.wic.bz2`: disco arrancable completo, usado por `flash_sd.sh`.
- `.ext4`: rootfs aislado; no basta para arrancar una SD.
- `.tar.bz2`: extracción manual del rootfs.

Se generan en `<poky>/build-<máquina>/tmp/deploy/images/<máquina>/` y nunca se
suben a Git.
