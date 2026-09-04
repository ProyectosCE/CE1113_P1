# Convenciones de nombres y documentación

## 1. Objetivo

Este documento define el idioma y las convenciones generales utilizadas para nombrar archivos, carpetas y elementos de código, así como para escribir documentación dentro del proyecto.

El objetivo es mantener consistencia sin agregar reglas innecesarias al flujo de trabajo del equipo.

---

## 2. Nombres técnicos

Los nombres técnicos del proyecto deben escribirse en inglés.

Esto incluye:

* Archivos.
* Carpetas.
* Funciones.
* Variables.
* Constantes.
* Macros.
* `struct`.
* `enum`.
* Clases, si se utiliza C++.
* Nombres de pruebas.

Ejemplos:

```text
src/motor_control.c
src/include/sensor_manager.h
tests/unit/test_navigation.c
```

```c
int obstacle_distance;
bool is_motor_enabled;

void update_robot_state(void);
int read_front_sensor(void);
```

Se debe evitar mezclar idiomas dentro de los nombres.

Ejemplos que deben evitarse:

```c
int distanciaSensor;
void actualizar_robot_state(void);
```

---

## 3. Documentación

La documentación debe escribirse en español.

Esto incluye:

* Comentarios dentro del código.
* Documentación de funciones.
* Archivos Markdown (`.md`).
* Explicaciones técnicas del proyecto.

Ejemplo:

```c
// Actualiza el estado del robot según la información de los sensores.
update_robot_state();
```

Los nombres de los archivos Markdown pueden mantenerse en inglés:

```text
repository-structure.md
commit-convention.md
branching-strategy.md
```

pero su contenido debe estar escrito en español.

---

## 4. Regla general

La convención principal del proyecto es:

```text
Nombres técnicos  → Inglés
Documentación     → Español
```

Cuando una herramienta, biblioteca o estándar externo defina un nombre específico, se debe respetar el nombre original.

La prioridad es mantener consistencia y facilitar la lectura del código y la documentación para todos los integrantes del equipo.
