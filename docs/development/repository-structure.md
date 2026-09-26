# Estructura actual del repositorio

```text
CE1113_P1/
├── .github/workflows/validation.yml
├── docs/{architecture,development,yocto}/
├── meta-ce1113/                    # capa original: solo imagen base
│   ├── conf/layer.conf
│   └── recipes-core/images/ce1113-p1.bb
├── layers/                         # capas funcionales opcionales
│   ├── meta-aura-apps/             # receta y fuentes de AuraBot
│   ├── meta-operaciones/
│   ├── meta-red/
│   └── meta-server/
├── scripts/validate-repository.sh
├── build.sh
├── check_image.sh
└── flash_sd.sh
```

`meta-ce1113` conserva su estructura original y define únicamente la imagen base.
Toda construcción del producto debe cargarla. Todas las recetas de aplicaciones
y subsistemas viven en `layers/meta-*`; `meta-aura-apps` contiene AuraBot. Estas capas
declaran dependencia de `ce1113`, guardan recetas bajo
`recipes-<categoría>/<receta>/` y amplían la imagen mediante
`recipes-core/images/ce1113-p1.bbappend`.

El código instalado en el dispositivo vive en `files/` junto a su receta. Así
BitBake calcula hashes y reconstruye reproduciblemente. No se admite C/CMake
suelto en la raíz. `build-*`, `downloads` y `sstate-cache` son resultados locales
y no se versionan; `rpi4/` es solo un espacio vacío heredado.

## Reglas comprobadas por CI

- Todas las recetas usan minúsculas y guiones.
- Una receta es `.bb`; una modificación de receta ajena es `.bbappend`.
- No se duplica `ce1113-p1.bb`: los módulos usan su `.bbappend`.
- Cada capa posee `layer.conf`, colección única, soporte `scarthgap` y, si es
  modular, dependencia de `ce1113`.
- No se versionan builds, cachés, logs ni archivos mayores de 10 MiB.
- Los scripts pasan validación sintáctica.

Ejecute `./scripts/validate-repository.sh` antes de abrir un PR.
