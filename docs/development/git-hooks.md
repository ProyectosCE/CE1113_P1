# Git Hooks

## 1. Objetivo

Este documento describe los Git hooks utilizados en el repositorio, cómo activarlos y qué validaciones realizan.

Los hooks definidos actualmente son:

```text
commit-msg
pre-commit
```

Ambos están almacenados dentro de:

```text
.githooks/
```

y se aplican únicamente a este repositorio.

---

## 2. Activación de los hooks

Git utiliza por defecto la carpeta interna `.git/hooks/`. Como los hooks del proyecto están versionados dentro de `.githooks/`, cada integrante debe configurar Git una sola vez para que utilice esa carpeta.

Desde la raíz del repositorio:

```bash
git config core.hooksPath .githooks
```

Ejemplo:

```text
CE1113_P1/
├── .git/
├── .githooks/
├── src/
├── tests/
└── ...
```

El comando debe ejecutarse dentro de `CE1113_P1`.

No se debe utilizar `--global`, ya que se quiere aplicar esta configuración únicamente a este repositorio.

Para verificar la configuración:

```bash
git config --get core.hooksPath
```

El resultado esperado es:

```text
.githooks
```

---

## 3. Hook `commit-msg`

El hook `commit-msg` se ejecuta automáticamente cuando Git intenta crear un commit.

Su objetivo es verificar que el mensaje cumpla con el estándar definido por el proyecto.

Formatos permitidos:

```text
[TAG] Descripción breve
```

```text
[TAG][TAG] Descripción breve
```

Se permite un máximo de dos tags.

Tags válidos:

```text
ADD
UPDATE
FIX
DEL
DOCS
TEST
REFACTOR
```

Ejemplos válidos:

```text
[ADD] Agrega módulo de control de motores
[FIX] Corrige lectura del sensor frontal
[UPDATE][TEST] Actualiza control de motores y sus pruebas
```

Si el mensaje no cumple el formato, el commit se cancela y se muestra un mensaje indicando el problema.

El hook también permite mensajes automáticos de merge generados por Git, por ejemplo:

```text
Merge branch 'develop' into feature/control-motores
```

Esto evita que el estándar de commits interfiera con operaciones normales de merge.

---

## 4. Hook `pre-commit`

El hook `pre-commit` se ejecuta antes de crear el commit.

Su objetivo es detectar errores simples antes de que los archivos sean agregados al historial del repositorio.

Actualmente realiza las siguientes validaciones:

### Archivos temporales o generados

Detecta archivos que normalmente no deberían formar parte del repositorio, por ejemplo:

```text
*.log
*.tmp
*.bak
*.swp
*.swo
__pycache__/
.cache/
cache/
```

La primera protección contra estos archivos sigue siendo `.gitignore`. El hook funciona como una validación adicional.

### Archivos demasiado grandes

El hook detecta archivos mayores al límite definido actualmente de:

```text
10 MiB
```

Esto ayuda a evitar commits accidentales de binarios, imágenes, archivos de caché u otros artefactos pesados.

### Nombres de archivos y carpetas

Se realizan validaciones básicas sobre los nombres de los archivos incluidos en el commit.

Se permite utilizar:

```text
Letras
Números
.
_
-
/
```

No se permiten espacios ni caracteres especiales fuera de este conjunto.

El hook no intenta comprobar automáticamente que los nombres estén escritos en inglés. Esa regla sigue siendo responsabilidad del desarrollador según el estándar documentado del proyecto.

---

## 5. Flujo normal

Una vez activados los hooks, no es necesario ejecutar ningún comando adicional.

El flujo normal continúa siendo:

```bash
git add .
git commit -m "[ADD] Agrega control de motores"
```

Git ejecutará automáticamente:

```text
pre-commit
    ↓
commit-msg
    ↓
commit
```

Si ambas validaciones pasan, el commit se crea normalmente.

Si alguna falla, Git cancela el commit y muestra el motivo para que el desarrollador pueda corregirlo.
