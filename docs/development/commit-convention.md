# Estándar de commits

## 1. Objetivo

Este documento define la convención utilizada para escribir los mensajes de commit en el repositorio.

El objetivo es mantener un historial de Git claro, consistente y fácil de entender para todos los integrantes del equipo.

---

## 2. Formato general

Todos los commits deben seguir la siguiente estructura:

```text
[TAG] Breve descripción
```

También se permite utilizar un máximo de dos tags cuando el cambio pertenezca claramente a más de una categoría:

```text
[TAG][TAG] Breve descripción
```

Ejemplo:

```text
[UPDATE][TEST] Actualiza las pruebas unitarias del movimiento de motores
```

La descripción breve es obligatoria y debe escribirse en español.

---

## 3. Tags disponibles

### `[ADD]`

Se utiliza cuando se agrega contenido nuevo al repositorio, como:

* Nuevos archivos.
* Nuevas funcionalidades.
* Nuevos módulos o componentes.

Ejemplo:

```text
[ADD] Agrega módulo para lectura del sensor ultrasónico
```

---

### `[UPDATE]`

Se utiliza cuando se modifica o amplía una funcionalidad existente sin tratarse de una corrección de error.

Ejemplo:

```text
[UPDATE] Mejora la lógica de detección de obstáculos
```

---

### `[FIX]`

Se utiliza cuando se corrige un error o comportamiento incorrecto existente.

Ejemplo:

```text
[FIX] Corrige cálculo de distancia del sensor frontal
```

---

### `[DEL]`

Se utiliza cuando se eliminan archivos, configuraciones o componentes que ya no son necesarios.

Ejemplo:

```text
[DEL] Elimina archivos temporales del entorno de pruebas
```

---

### `[DOCS]`

Se utiliza para cambios relacionados con documentación.

Incluye, por ejemplo:

* Archivos Markdown.
* README.
* Comentarios relevantes dentro del código.

Ejemplo:

```text
[DOCS] Documenta la estructura base del repositorio
```

---

### `[TEST]`

Se utiliza cuando el cambio está relacionado con pruebas.

Ejemplo:

```text
[TEST] Agrega pruebas unitarias para la lógica de navegación
```

---

### `[REFACTOR]`

Se utiliza cuando se reorganiza o mejora internamente el código sin modificar su comportamiento esperado.

Ejemplo:

```text
[REFACTOR] Simplifica la lógica de control de movimiento
```

---

## 4. Uso de múltiples tags

Un commit puede utilizar hasta dos tags.

Esto debe hacerse únicamente cuando ambos representen claramente el cambio realizado.

Ejemplos:

```text
[ADD][TEST] Agrega módulo de navegación y sus pruebas unitarias
```

```text
[FIX][DOCS] Corrige configuración de compilación y actualiza su documentación
```

Si un commit requiere más de dos tags, se recomienda revisar si los cambios deberían dividirse en commits separados.

---

## 5. Descripción breve

La descripción del commit debe:

* Estar escrita en español.
* Ser corta y clara.
* Explicar qué se modificó.
* Evitar mensajes genéricos.

Ejemplos recomendados:

```text
[ADD] Agrega interfaz para control de motores
[FIX] Corrige lectura incorrecta del sensor izquierdo
[UPDATE] Mejora el manejo de estados del robot
```

Ejemplos que deben evitarse:

```text
[UPDATE] Cambios
[FIX] Arreglo
[ADD] Cosas nuevas
```

---

## 6. Descripción extendida

El cuerpo o descripción extendida del commit es opcional.

Puede utilizarse cuando sea necesario agregar contexto adicional sobre:

* La razón del cambio.
* Decisiones técnicas importantes.
* Limitaciones conocidas.
* Información útil para otros integrantes del equipo.

Para cambios pequeños y suficientemente claros, el mensaje corto es suficiente.

---

## 7. Resumen

El formato obligatorio es:

```text
[TAG] Descripción breve
```

o:

```text
[TAG][TAG] Descripción breve
```

Los tags permitidos son:

```text
[ADD]
[UPDATE]
[FIX]
[DEL]
[DOCS]
[TEST]
[REFACTOR]
```

Cada commit debe representar un cambio lógico y mantener un mensaje claro para facilitar la revisión y el seguimiento del historial del proyecto.
