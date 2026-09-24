#ifndef LIBAUDIO_H
#define LIBAUDIO_H


//modo = in o out



int IniciarSonido(const char *audio_device);

static int EnviarComando(const char *comando);

int CargarSonido(const char *archivo);


int Pausar(void);
int Stop(void);

#endif
