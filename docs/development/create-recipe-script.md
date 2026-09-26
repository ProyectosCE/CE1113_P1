# Asistente para crear recetas

```bash
./scripts/create-recipe.sh
```

El script detecta dinámicamente los `layers/meta-*` válidos. Permite seleccionar
uno o crear un layer nuevo y después solicita el nombre de la receta.

Para `mi-servicio` genera:

```text
layers/meta-<layer>/
├── recipes-apps/mi-servicio/
│   ├── files/.gitkeep
│   └── mi-servicio_1.0.0.bb
└── recipes-core/images/<imagen-principal>.bbappend
```

La receta inicial usa `ALLOW_EMPTY`, por lo que es válida antes de añadir código.
Después se agregan manualmente `SRC_URI`, archivos, dependencias, clases y tareas
de instalación. El asistente nunca sobrescribe una receta existente ni duplica
el paquete en el `.bbappend`.

El nombre del `.bbappend` no está fijado en el script: se detecta leyendo la única
receta `.bb` presente en `meta-ce1113/recipes-core/images`.
