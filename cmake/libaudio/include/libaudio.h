#ifndef LIBAUDIO_H
#define LIBAUDIO_H

int IniciarSonido(const char *audio_device);

int CargarSonido(const char *archivo);

int Pausar(void);

int Stop(void);

#endif