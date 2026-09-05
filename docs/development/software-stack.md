# Stack de software y entorno de desarrollo

Este documento resume las principales herramientas y tecnologías seleccionadas para el desarrollo del software del proyecto.

El objetivo es que todos los integrantes del equipo conozcan qué se utilizará, para qué se utilizará y cómo se relacionan las diferentes partes del sistema.

## Arquitectura objetivo

El sistema se ejecutará sobre una Raspberry Pi 4 Model B utilizando una distribución Linux mínima generada con Yocto y arquitectura AArch64 (ARM64).

La arquitectura general del software será:

```text
Web responsive
      |
Python + Flask
      |
    ctypes
      |
libaurabot.so
      |
Software de control en C
      |
Linux / Hardware
```

El acceso al hardware deberá realizarse exclusivamente mediante la biblioteca dinámica propia del proyecto.

---

## C17

El software principal del robot será desarrollado utilizando C17.

C se utilizará principalmente para:

* Control de motores.
* Lectura de sensores.
* Control de LEDs.
* Manejo de audio.
* Navegación autónoma.
* Mapeo y estado interno del robot.
* Biblioteca dinámica de acceso al hardware.

Se seleccionó C por su bajo nivel de abstracción, eficiencia y amplio uso en sistemas embebidos y Linux.

---

## CMake

Se utilizará CMake como sistema de construcción del código desarrollado en C.

CMake permitirá:

* Compilar la biblioteca dinámica del proyecto.
* Generar ejecutables auxiliares.
* Mantener una estructura organizada del proyecto.
* Compilar tanto para la computadora de desarrollo como para la arquitectura ARM64.
* Integrar posteriormente el proyecto con Yocto.

La biblioteca principal del sistema será generada como una biblioteca compartida, por ejemplo:

```text
libaurabot.so
```

---

## Cross-compilation y AArch64

La Raspberry Pi 4 utiliza una arquitectura ARM compatible con AArch64.

El software destinado a la Raspberry Pi será compilado desde las computadoras de desarrollo utilizando un toolchain de compilación cruzada.

Durante las primeras etapas se podrá utilizar herramientas ARM64 genéricas para experimentación. Una vez integrado el proyecto con Yocto, el entorno de compilación proporcionado por Yocto será la referencia para generar los binarios destinados al sistema final.

El objetivo es que el flujo sea:

```text
PC x86_64
    |
Cross-toolchain
    |
Binario AArch64
    |
Raspberry Pi 4
```

---

## Yocto Linux

Se utilizará Yocto para generar el sistema operativo Linux del robot.

Yocto permitirá construir una imagen personalizada que contenga únicamente los componentes necesarios, entre ellos:

* Kernel de Linux.
* systemd.
* Aplicación del robot.
* Biblioteca dinámica.
* Python.
* Flask.
* Dependencias necesarias para hardware y audio.
* Archivos de configuración.

La capa propia del proyecto se mantendrá dentro de:

```text
yocto/meta-aurabot/
```

El objetivo final es que la imagen completa pueda generarse mediante BitBake sin configuraciones manuales posteriores sobre la Raspberry Pi.

---

## Python y Flask

La interfaz remota utilizará Python 3 junto con Flask.

Flask será responsable principalmente de:

* Servir la interfaz web.
* Recibir comandos del usuario.
* Exponer la API del robot.
* Consultar el estado del sistema.
* Gestionar el control remoto.

Python no accederá directamente al hardware.

---

## ctypes

Se utilizará el módulo estándar de Python ctypes para comunicar Flask con la biblioteca desarrollada en C.

La comunicación será:

```text
Flask
  |
Python
  |
ctypes
  |
libaurabot.so
```

De esta manera, el servidor podrá utilizar las funciones de la biblioteca C sin implementar directamente acceso a GPIO, motores, sensores u otros dispositivos.

---

## Interfaz web responsive

El control remoto se implementará inicialmente como una aplicación web responsive utilizando:

* HTML.
* CSS.
* JavaScript.

La interfaz deberá poder utilizarse desde computadoras, teléfonos y tablets mediante un navegador web.

Para la comunicación con Flask se podrán utilizar:

* HTTP/REST para comandos y configuración.
* WebSocket para información que requiera actualización frecuente, como sensores, estado del robot y mapa.

No se contempla inicialmente desarrollar una aplicación móvil nativa.

---

## systemd

La aplicación principal será ejecutada como un servicio de systemd.

systemd permitirá:

* Iniciar automáticamente el sistema de control durante el arranque.
* Reiniciar el servicio en caso de fallo.
* Administrar el estado del proceso.
* Integrar los mensajes de la aplicación con el journal del sistema.

La configuración final incluirá una unidad propia para AuraBot.

---

## Logging y journalctl

El código C contará con una interfaz sencilla de logging con niveles similares a:

```text
DEBUG
INFO
WARN
ERROR
```

Los mensajes podrán escribirse en la salida estándar y ser recopilados por systemd.

En el sistema embebido se podrán consultar mediante:

```bash
journalctl -u <servicio>
```

y seguir en tiempo real mediante:

```bash
journalctl -u <servicio> -f
```

Esto facilitará el diagnóstico de problemas durante la integración con la Raspberry Pi.

---

## GDB y gdbserver

Se utilizará GDB para depurar el software escrito en C.

Durante el desarrollo local se podrá ejecutar directamente GDB sobre los programas correspondientes.

Para depuración remota sobre la Raspberry Pi se utilizará:

```text
GDB en PC
    |
Red
    |
gdbserver
    |
Raspberry Pi
```

Esto permitirá establecer breakpoints, inspeccionar variables, analizar fallos y obtener backtraces sin necesitar depurar directamente desde la placa.

---

## QEMU

Debido a que solo se dispone de una única Raspberry Pi 4, se utilizará QEMU como entorno ARM64 emulado.

Inicialmente se utilizará un Linux ARM64 genérico en lugar de intentar emular completamente una Raspberry Pi 4.

QEMU permitirá comprobar aspectos como:

* Ejecución de binarios AArch64.
* Carga de `libaurabot.so`.
* Dependencias dinámicas.
* Integración C/Python mediante ctypes.
* Ejecución de Flask.
* Configuración del sistema.
* Comportamiento general del software ARM64.

QEMU no sustituye las pruebas sobre hardware real. GPIO, PWM, motores, sensores, audio físico y comportamiento temporal deberán verificarse posteriormente sobre la Raspberry Pi.

---

## Docker

Docker se utilizará principalmente para proporcionar un entorno compartido de QEMU.

La idea general será:

```text
Windows
   |
  WSL
   |
 Docker
   |
 QEMU
   |
Linux ARM64
```

Esto permitirá que todos los integrantes utilicen una configuración similar sin instalar y configurar QEMU manualmente.

Docker se utilizará como herramienta de desarrollo en el host y no formará parte de la imagen final de la Raspberry Pi.

---

## Entornos de ejecución

Durante el desarrollo existirán tres entornos principales:

| Entorno             | Uso principal                                  |
| ------------------- | ---------------------------------------------- |
| PC / WSL            | Desarrollo general del software                |
| Docker + QEMU ARM64 | Verificación del software compilado para ARM64 |
| Raspberry Pi 4      | Integración y validación con hardware real     |

QEMU complementará el acceso limitado a la Raspberry Pi, pero no reemplazará las pruebas físicas.

---

## Resumen de herramientas

| Tecnología              | Uso en el proyecto                                |
| ----------------------- | ------------------------------------------------- |
| **C17**                 | Software de control y biblioteca dinámica         |
| **CMake**               | Construcción del software C                       |
| **AArch64**             | Arquitectura objetivo de la Raspberry Pi 4        |
| **Cross-toolchain**     | Compilación desde el host para ARM64              |
| **Yocto**               | Generación del sistema Linux personalizado        |
| **Python 3**            | Capa de aplicación del servidor                   |
| **Flask**               | Servidor web y API remota                         |
| **ctypes**              | Comunicación Python ↔ biblioteca C                |
| **HTML/CSS/JavaScript** | Interfaz web responsive                           |
| **systemd**             | Inicio y administración del servicio              |
| **Logging**             | Registro de eventos y errores                     |
| **journalctl**          | Consulta de logs en el target                     |
| **GDB**                 | Depuración del software C                         |
| **gdbserver**           | Depuración remota sobre Raspberry Pi              |
| **QEMU ARM64**          | Ejecución y verificación en entorno ARM64 emulado |
| **Docker**              | Distribución reproducible del entorno QEMU        |
