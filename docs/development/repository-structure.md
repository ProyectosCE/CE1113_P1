# Estructura del repositorio

## 1. Objetivo

Este documento describe la estructura base del repositorio y establece de forma general dónde debe colocarse cada tipo de archivo.

La estructura podrá evolucionar conforme avance el proyecto, evitando agregar carpetas o configuraciones que todavía no sean necesarias.

---

## 2. Estructura base

```text
CE1113_P1/
│
├── .github/
│   └── workflows/
│
├── docs/
│   ├── README.md
│   └── development/
│
├── src/
│   ├── main.c
│   ├── include/
│   └── core/
│
├── tests/
│   ├── unit/
│   └── integration/
│
├── yocto/
│   ├── emuARM/
│   └── meta-aurabot/
│
├── .gitignore
├── LICENSE
└── README.md
```

---

## 3. Descripción de directorios

### `.github/`

Contiene configuraciones relacionadas con GitHub.

```text
.github/
└── workflows/
```

La carpeta `workflows/` será utilizada para los flujos de GitHub Actions que se definan posteriormente para compilación, pruebas o validaciones automáticas.

---

### `docs/`

Contiene la documentación del proyecto.

```text
docs/
├── README.md
└── development/
```

* `README.md`: sirve como punto de entrada a la documentación.
* `development/`: contiene documentación relacionada con el proceso de desarrollo, estándares y organización del repositorio.

Ejemplos futuros:

```text
docs/development/
├── repository-structure.md
├── commit-convention.md
└── pull-request-guidelines.md
```

Solo se agregarán documentos cuando sean necesarios para el proyecto.

---

### `src/`

Contiene el código fuente principal del proyecto desarrollado en C y/o C++.

```text
src/
├── main.c
├── include/
└── core/
```

#### `main.c`

Contiene el punto de entrada principal de la aplicación.

#### `include/`

Contiene los archivos de cabecera del proyecto, principalmente archivos `.h` o `.hpp`.

Estos archivos pueden definir:

* Interfaces.
* Funciones.
* Estructuras de datos.
* Constantes.
* Tipos compartidos entre diferentes módulos.

#### `core/`

Contiene la lógica principal del robot.

Aquí se ubicarán componentes relacionados con comportamiento, navegación, control y otras funciones principales que no dependan directamente de una implementación específica de hardware.

La organización interna de `core/` podrá cambiar conforme se defina la arquitectura del software.

---

### `tests/`

Contiene las pruebas automatizadas del proyecto.

```text
tests/
├── unit/
└── integration/
```

#### `unit/`

Contiene pruebas de componentes individuales, funciones o módulos de forma aislada.

#### `integration/`

Contiene pruebas que verifican la interacción entre varios componentes del sistema.

La herramienta o framework utilizado para las pruebas se definirá posteriormente.

---

### `yocto/`

Agrupa todos los archivos relacionados con Yocto.

```text
yocto/
├── emuARM/
└── meta-aurabot/
```

Esto permite mantener los archivos específicos de Yocto separados del código principal de la aplicación.

#### `emuARM/`

Contiene configuraciones relacionadas con el entorno de compilación o emulación utilizado durante el desarrollo.

Los archivos generados, cachés y otros artefactos temporales de Yocto no deben almacenarse en Git.

#### `meta-aurabot/`

Contiene la capa personalizada de Yocto utilizada por el proyecto.

Dentro de esta carpeta se encontrarán configuraciones, recetas y demás metadatos específicos necesarios para integrar el proyecto dentro de una imagen Yocto.

Los detalles internos de la configuración de Yocto se documentarán por separado cuando sea necesario.

---

## 4. Archivos en la raíz

### `README.md`

Documento principal del repositorio.

Debe ofrecer una introducción general al proyecto y enlaces hacia documentación más detallada cuando sea necesario.

### `.gitignore`

Define los archivos y directorios que Git no debe rastrear, como:

* Archivos temporales.
* Cachés.
* Resultados de compilación.
* Archivos generados.
* Configuraciones locales de herramientas.

### `LICENSE`

Contiene la licencia utilizada por el proyecto.

---

## 5. Reglas generales de organización

Para mantener el repositorio ordenado, se seguirán estas reglas:

1. El código principal en C/C++ debe mantenerse dentro de `src/`.
2. Las pruebas deben mantenerse dentro de `tests/`.
3. La documentación debe mantenerse dentro de `docs/`.
4. Los archivos específicos de Yocto deben mantenerse dentro de `yocto/`.
5. Los archivos generados, temporales o de caché no deben subirse al repositorio.
6. No se deben crear nuevas carpetas sin que exista una necesidad concreta dentro del proyecto.
7. La estructura podrá ampliarse conforme se definan nuevos componentes del sistema.

---

## 6. Sistema de compilación

El proyecto utilizará CMake como sistema de compilación.

La ubicación y organización de los archivos `CMakeLists.txt` se definirá cuando la arquitectura del código esté suficientemente establecida.

Dependiendo de las necesidades del proyecto, podrá existir un archivo principal en la raíz y archivos adicionales dentro de los directorios que contengan código fuente.

---

## 7. Resumen

| Directorio            | Propósito                                |
| --------------------- | ---------------------------------------- |
| `.github/`            | Configuración y automatización de GitHub |
| `docs/`               | Documentación del proyecto               |
| `src/`                | Código fuente principal                  |
| `src/include/`        | Archivos de cabecera                     |
| `src/core/`           | Lógica principal del robot               |
| `tests/unit/`         | Pruebas unitarias                        |
| `tests/integration/`  | Pruebas de integración                   |
| `yocto/`              | Archivos relacionados con Yocto          |
| `yocto/emuARM/`       | Configuración del entorno Yocto          |
| `yocto/meta-aurabot/` | Capa personalizada del proyecto          |

Esta estructura se considera la base inicial del repositorio y podrá evolucionar conforme avance el desarrollo del proyecto.
