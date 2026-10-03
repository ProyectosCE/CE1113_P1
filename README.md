# Instituto Tecnológico de Costa Rica

## Proyecto I

### Sistema embebido a la medida para un robot aspiradora autónomo con reproducción de audio y control remoto

## Sistemas Empotrados (CE1113)

---

### Estudiantes

- José Bernardo Barquero Bonilla (2023150476)
- Jose Eduardo Campos Salazar (2023135620)
- Jimmy Feng Feng (2023060347)
- Alexander Montero Vargas (2023166058)

### Profesor

- Dr.-Ing Jeferson González Gómez <jgonzalez@itcr.ac.cr>

---

### Objetivo

Mediante el desarrollo de este proyecto, cada grupo de trabajo aplicará los conceptos y herramientas de software y hardware vistos en el curso en el diseño de un sistema embebido a la medida que controla un robot aspiradora autónomo. El sistema deberá ser capaz de navegar de forma autónoma evitando obstáculos, reproducir audio (archivos MP3), y además poder ser operado de forma remota a través de un servidor web o aplicación móvil mediante conectividad WiFi/Bluetooth.


## Desarrollo

La imagen principal es `ce1113-p1`. Todas las recetas activas están dentro de
`meta-ce1113`, organizadas en `recipes-lib`, `recipes-test`, `recipes-hw` y
`recipes-auraapp`. La imagen instala un único `packagegroup-ce1113`; los grupos
por área mantienen la composición sin modificar la receta de imagen.

Consulte el [índice de documentación](docs/README.md), en particular:

- [estructura del repositorio](docs/development/repository-structure.md);
- [imágenes disponibles](docs/yocto/images.md);
- [scripts de compilación, comprobación y grabación](docs/development/scripts.md);
- [guía para añadir funcionalidades](docs/yocto/extending.md).
- [integración y resultados de audio](docs/architecture/audio.md).

## Notas de audio

El producto actual utiliza el jack de 3.5 mm y PWM por software. La capa
experimental `layers/meta-pwm`, correspondiente al PWM por hardware, se
conserva fuera del build y `build.sh` no la añade a `BBLAYERS`.

Construir la imagen de prueba:

```sh
./build.sh --no-ui --machine raspberrypi4 --image ce1113-p1
```

Después de arrancar la Raspberry Pi:

```sh
/etc/init.d/aurabot-audio status
/etc/init.d/webapp-httpd status
aplay -l
```

La página web presenta controles **Play**, **Pausa** y **Stop**. Play reproduce
`/usr/share/aurabot/audio/test.mp3`. El servicio intenta encontrar la tarjeta
ALSA cuyo identificador contiene `Headphones`; puede sobrescribirse creando
`/etc/default/aurabot-audio` con, por ejemplo:

```sh
AURABOT_AUDIO_DEVICE="plughw:Headphones,0"
```

Después se aplica el cambio con:

```sh
/etc/init.d/aurabot-audio restart
```
