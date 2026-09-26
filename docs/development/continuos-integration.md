# Integración continua

`.github/workflows/validation.yml` se ejecuta en cada push y pull request hacia
`develop` o `main`. Mantiene dos trabajos independientes:

1. `repository-and-yocto` ejecuta `scripts/validate-repository.sh`. Comprueba la
   estructura, nombres, archivos generados, capa principal, dependencias entre
   capas, ubicación/formato de recetas, uso de `ce1113-p1.bbappend`, ubicación
   de C/CMake y sintaxis de shell.
2. `native-cmake-build` configura, compila e instala en staging las mismas fuentes
   que consume la receta `aurabot`. Detecta roturas de C/CMake sin ejecutar el
   costoso build completo de Yocto.

La validación local equivalente es:

```bash
./scripts/validate-repository.sh
cmake -S layers/meta-aura-apps/recipes-apps/aurabot/files -B build-native
cmake --build build-native --parallel
```

El build BitBake completo depende del árbol externo de Poky, descargas y caché;
por ello no se ejecuta en el runner genérico. Antes de integrar una entrega se
debe ejecutar localmente `build.sh`, `bitbake-layers show-appends` y
`check_image.sh`, como describe la guía de extensión.
