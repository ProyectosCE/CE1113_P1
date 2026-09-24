#ifndef LIBGPIO_H
#define LIBGPIO_H

int pinMode(int pin, const char *mode);

int digitalWrite(int pin, int value);

int digitalRead(int pin);

int setPWM(int pin, int freq, int duty_percent);

#endif