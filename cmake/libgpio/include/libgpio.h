#ifndef LIBGPIO_H
#define LIBGPIO_H


//modo = in o out

int pinMode(int pin, const char *MODE);


int digitalWrite(int pin, int value);

int digitalRead(int pin);
//pin solo 0 o 1

int blink(int pin, int freq, int duration);

#endif
