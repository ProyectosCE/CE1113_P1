# Estructura del repositorio

```text
CE1113_P1/
├── .github/workflows/validation.yml
├── docs/{architecture,development,yocto}/
├── meta-ce1113/                    # única capa activa del producto
│   ├── conf/layer.conf
│   ├── recipes-core/               # imagen y packagegroups
│   ├── recipes-hw/                 # hardware, audio y red
│   ├── recipes-webapp/             # HTTP, CGI y frontend
│   ├── recipes-api/                # API y operaciones compartidas
│   └── recipes-auraapp/            # aplicaciones AuraBot
├── layers/meta-pwm/                # referencia experimental, fuera del build
├── scripts/
│   ├── check-repository.sh         # entrada estable local/CI
│   ├── validate-repository.sh      # implementación de las reglas
│   ├── create-recipe.sh
│   └── lib/image-common.sh         # selección e integridad de artefactos
├── build.sh
├── check_image.sh
└── flash_sd.sh
```

`meta-ce1113` contiene toda la metadata activa. La imagen `ce1113-p1` instala
únicamente `packagegroup-ce1113`; este compone los grupos `hw`, `webapp`, `api`
y `auraapp`. Añadir una aplicación requiere modificar solo el packagegroup de
su área, nunca la receta de imagen ni `layer.conf`.

## Categorías

| Categoría | Responsabilidad |
|---|---|
| `recipes-hw` | Audio, kernel, bootfiles, firmware, BusyBox y conectividad |
| `recipes-webapp` | Archivos web, CGI y servicio HTTP |
| `recipes-api` | Operaciones y API reutilizable |
| `recipes-auraapp` | Ejecutables y servicios propios de AuraBot |

Una receta normal se registra en el packagegroup de su área. Una receta de
configuración puede ser dependencia de la receta que la consume si no representa
una funcionalidad seleccionable. Los `.bbappend` se reservan para recetas
externas de Poky o `meta-raspberrypi`.

El código instalado vive en `files/` junto a su receta, permitiendo que BitBake
calcule hashes reproducibles. No se admite C/CMake suelto en la raíz. Los
directorios `build-*`, `downloads` y `sstate-cache` son resultados locales.

## Reglas comprobadas

- Existe una sola imagen: `ce1113-p1` para `raspberrypi4`.
- Todas las recetas usan minúsculas y nombres permitidos.
- No existen `.bbappend` de la imagen; la composición usa packagegroups.
- Los cuatro packagegroups de área forman el producto.
- Solo `meta-ce1113` se activa; `meta-pwm` permanece excluida.
- El código C/CMake se encuentra dentro de `files/` de una receta.
- No se versionan builds, cachés, logs ni archivos mayores de 10 MiB.
- Build y flash invocan la comprobación de integridad.

Ejecute `./scripts/check-repository.sh` antes de abrir un PR.
