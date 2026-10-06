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
  └─ media-audio.mount la monta en /media/audio
```

La imagen ya no contiene canciones. `flash_sd.sh` crea una partición ext4 de
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
```

`TRACK` carga la canción indicada por su posición (desde cero) en
`/media/audio/playlist.txt`. La CGI expone esa lista a la web y solo envía el
índice, por lo que una petición HTTP no puede inyectar una ruta arbitraria.
`PLAY` se conserva para la prueba histórica con `/media/audio/test.mp3`.

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
