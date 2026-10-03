# Asistente para crear recetas

Modo interactivo:

```bash
./scripts/create-recipe.sh
```

Modo automatizable:

```bash
./scripts/create-recipe.sh --category test --name telemetria --version 1.0.0
```

Las categorías permitidas son `lib`, `test`, `hw` y `auraapp`. Para
`telemetria`, el script crea:

```text
meta-ce1113/
├── recipes-test/telemetria/
│   ├── files/.gitkeep
│   └── telemetria_1.0.0.bb
└── recipes-core/packagegroups/packagegroup-ce1113-test.bb
```

La receta inicial usa `ALLOW_EMPTY`, de modo que puede validarse antes de añadir
código. El script registra el paquete en el grupo del área, no modifica la
imagen ni `layer.conf`, rechaza categorías/nombres/versiones inválidos y nunca
sobrescribe un directorio existente.

Después deben añadirse manualmente `SRC_URI`, fuentes, dependencias, clases y
`do_install`. Finalmente:

```bash
./scripts/check-repository.sh
./build.sh --no-ui
```

Este asistente crea recetas propias. Para modificar BusyBox, kernel, bootfiles u
otra receta externa, cree el `.bbappend` en la categoría adecuada conservando el
nombre base externo: `nombre.bbappend`, `nombre_versión.bbappend` o
`nombre_%.bbappend`.
