#ifndef LIBAUDIO_H
#define LIBAUDIO_H

int IniciarSonido(const char *audio_device);

int CargarSonido(const char *archivo);

int SaltarAFrame(long frame);

int ProcesarRespuestasAudio(void);

int DescriptorRespuestasAudio(void);

long FrameActualAudio(void);

unsigned long GeneracionIniciadaAudio(void);

unsigned long GeneracionSolicitadaAudio(void);

unsigned long GeneracionFinalizadaAudio(void);

int Pausar(void);

int Stop(void);

int CargarListaReproduccion(const char *playlist_path);

int AjustarVolumen(const char *audio_device, int volume_percent);

int FinalizarSonido(void);

#endif
