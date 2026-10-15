# Arquitectura de audio

El audio se divide entre activación física, biblioteca dinámica, servidor y
almacenamiento persistente:

```text
recipes-hw/audio-config
  └─ habilita snd-bcm2835 y la salida analógica
recipes-lib/audio
  └─ libaudio.so: controla mpg123 y volumen ALSA
recipes-auraapp/aurabot-audio-server
  └─ aurabot-audio.service y FIFO /run/aurabot-audio/control
partición 3 AURA_AUDIO (ext4)
  └─ canciones y playlist montadas en /media/audio
receta aurabot-audio-server
  └─ avisos internos instalados en /usr/share/aurabot/sounds
```

La imagen no contiene las canciones del usuario. Los cuatro avisos del robot
(`startup`, modo manual, modo automático y obstáculo) sí se incorporan desde
la carpeta `sonidos/` mediante la receta del servidor; no se copian a la
partición `AURA_AUDIO`. `flash_sd.sh` crea una partición ext4 de
2 GiB al final de la SD durante el primer flash y conserva sus bloques en los
flashes siguientes. `audio-storage` la monta por etiqueta en `/media/audio`
antes de iniciar el servidor.

## Biblioteca y servidor

La receta `recipes-lib/audio/audio_1.0.0.bb` genera `libaudio.so`: inicia
`mpg123`, carga MP3 o playlists, pausa, detiene y ajusta volumen. El servidor se
compila en otra receta con `DEPENDS = "audio"`; no duplica la implementación.

`aurabot-audio.service` mantiene el servidor activo con
`Restart=on-failure`. El PCM se puede fijar mediante
`AURABOT_AUDIO_DEVICE`; si no existe, el servidor busca la tarjeta
`Headphones` y finalmente usa `default`.

Comandos actuales del FIFO:

```text
PLAY
PAUSE
STOP
VOLUME 0..100
DEVICE <número-de-tarjeta>
TRACK <índice-de-playlist>
EVENT boot|manual|autonomous|obstacle
```

`TRACK` carga la canción indicada por su posición (desde cero) en
`/media/audio/playlist.txt`. La CGI expone esa lista a la web y solo envía el
índice, por lo que una petición HTTP no puede inyectar una ruta arbitraria.
`PLAY` se conserva para la prueba histórica con `/media/audio/test.mp3`.

`EVENT` solo acepta esos cuatro nombres y resuelve rutas internas fijas. El
servidor conserva el fotograma actual de la canción, reproduce los eventos en
orden y luego vuelve a cargar la canción con `JUMP` para continuarla. Si la
canción estaba pausada, se mantiene pausada al terminar el aviso. Los eventos
de obstáculo se deduplican mientras haya uno reproduciéndose o esperando en la
cola, para que detecciones consecutivas no repitan la alerta.

La web presenta un volumen lógico de 0 a 100 y lo convierte al rango útil del
mezclador: 0% web equivale a 50% ALSA y 100% web equivale a 100% ALSA.

Prueba básica:

```bash
systemctl status media-audio.mount
mount | grep /media/audio
systemctl status aurabot-audio.service
printf 'TRACK 0\n' > /run/aurabot-audio/control
printf 'VOLUME 60\n' > /run/aurabot-audio/control
printf 'STOP\n' > /run/aurabot-audio/control
```

| Componente | Ruta |
|---|---|
| Activación física | `meta-ce1113/recipes-hw/audio-config/` |
| Montaje persistente | `meta-ce1113/recipes-hw/audio-storage/` |
| Biblioteca | `meta-ce1113/recipes-lib/audio/` |
| Servidor | `meta-ce1113/recipes-auraapp/aurabot-audio-server/` |
| CGI y web | `meta-ce1113/recipes-auraapp/webapp/` |
