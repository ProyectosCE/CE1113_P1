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

La imagen principal es `ce1113-p1` y su definición permanece aislada en
`meta-ce1113`. El código C/CMake se compila mediante la receta `aurabot` de
`layers/meta-aura-apps`.

Consulte el [índice de documentación](docs/README.md), en particular:

- [estructura del repositorio](docs/development/repository-structure.md);
- [imágenes disponibles](docs/yocto/images.md);
- [scripts de compilación, comprobación y grabación](docs/development/scripts.md);
- [guía para añadir funcionalidades](docs/yocto/extending.md).
- [integración y resultados de audio](docs/architecture/audio.md).

## Notas de audio

Durante la prueba del jack de 3.5 mm, `build.sh` excluye `meta-pwm` de la
selección `all`, porque el audio analógico y los motores usan el mismo bloque
PWM. Para volver a incluir esa capa se debe usar `--enable-pwm`, sabiendo que el
jack analógico dejará de estar disponible.

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
