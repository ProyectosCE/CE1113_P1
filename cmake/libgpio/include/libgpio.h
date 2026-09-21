#ifndef LIBGPIO_H
#define LIBGPIO_H


//modo = in o out




int pinMode(int pin, const char *MODE);


int digitalWrite(int pin, int value);

int digitalRead(int pin);
//pin solo 0 o 1

int blink(int pin, int freq, int duration);

int IniciarSonido(const char *audio_device);

static int EnviarComando(const char *comando);

int CargarSonido(const char *archivo);


int Pausar(void);
int Stop(void);

#endif
