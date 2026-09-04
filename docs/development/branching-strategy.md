# Estrategia de ramas

## 1. Objetivo

Este documento define el flujo de ramas utilizado en el repositorio.

El objetivo es mantener un proceso de desarrollo simple, ordenado y consistente para todos los integrantes del equipo.

---

## 2. Flujo principal

El repositorio utiliza la siguiente estructura general:

```text
main
  ↑
develop
  ↑
ramas de trabajo
```

### `main`

Es la rama estable y final del proyecto.

* No recibe commits directamente.
* Los cambios llegan únicamente mediante Pull Request.
* Los Pull Requests hacia `main` requieren al menos 1 reviewer.
* Se utilizará principalmente para integrar la versión final del proyecto desde `develop`.

### `develop`

Es la rama principal de integración durante el desarrollo.

* No recibe commits directamente.
* Recibe cambios mediante Pull Requests desde las ramas de trabajo.
* Las nuevas ramas de trabajo deben crearse normalmente desde `develop`.

---

## 3. Ramas de trabajo

Las ramas de trabajo deben seguir el formato:

```text
tipo/descripcion-corta
```

Los nombres deben escribirse en minúsculas y utilizar guiones (`-`) para separar palabras.

Ejemplos:

```text
feature/control-motores
fix/error-lectura-sensor
docs/documentacion-inicial
```

### Tipos permitidos

| Prefijo     | Uso                                                             |
| ----------- | --------------------------------------------------------------- |
| `feature/`  | Desarrollo de una funcionalidad nueva                           |
| `fix/`      | Corrección de errores o bugs                                    |
| `task/`     | Configuración, mantenimiento u otras tareas técnicas            |
| `docs/`     | Cambios exclusivamente de documentación                         |
| `test/`     | Creación o modificación de pruebas                              |
| `refactor/` | Reorganización de código sin cambiar su comportamiento esperado |

---

## 4. Flujo de trabajo

El flujo recomendado para una tarea es:

```text
develop
   │
   └── rama de trabajo
          │
          ├── commits
          │
          └── Pull Request
                  │
                  ▼
               develop
```

Antes de abrir un Pull Request, la rama debe actualizarse con los cambios recientes de su rama de origen.

Para una rama creada directamente desde `develop`, por ejemplo:

```bash
git checkout develop
git pull

git checkout feature/control-motores
git merge develop
```

No se requiere utilizar `rebase` como parte obligatoria del flujo.

---

## 5. Ramas hijas

Cuando sea necesario, una rama de trabajo puede tener ramas adicionales.

Ejemplo:

```text
develop
  └── feature/control-motores
        └── test/pruebas-motores
```

En estos casos, la rama hija debe hacer su Pull Request hacia la rama de la cual fue creada:

```text
test/pruebas-motores
        ↓
feature/control-motores
        ↓
develop
```

Las ramas hijas deben utilizarse únicamente cuando ayuden a dividir claramente el trabajo.

---

## 6. Eliminación de ramas

Las ramas de trabajo deben eliminarse después de que su Pull Request haya sido integrado correctamente.

Las ramas permanentes del repositorio son:

```text
main
develop
```

Las ramas `feature/`, `fix/`, `task/`, `docs/`, `test/` y `refactor/` son temporales y no deben reutilizarse para tareas diferentes.

---

## 7. Reglas generales

1. No realizar commits directamente sobre `main` o `develop`.
2. Crear normalmente las ramas de trabajo desde `develop`.
3. Utilizar el formato `tipo/descripcion-corta`.
4. Utilizar nombres en minúsculas y separar palabras con guiones.
5. Utilizar únicamente los prefijos definidos en este documento.
6. Las ramas hijas deben integrarse primero en su rama padre.
7. Actualizar la rama con los cambios recientes de su rama de origen antes de abrir el Pull Request.
8. Eliminar las ramas de trabajo después de integrarlas.
9. Evitar nombres genéricos como `feature/cambios` o `task/arreglos`.
10. Mantener el flujo simple y evitar reglas adicionales que no aporten valor al proyecto.
