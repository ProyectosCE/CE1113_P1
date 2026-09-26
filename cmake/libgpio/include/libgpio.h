#ifndef LIBGPIO_H
#define LIBGPIO_H

int pinMode(int pin, const char *mode);

int digitalWrite(int pin, int value);

int digitalRead(int pin);

int gpioRelease(int pin);

/* Compatibility: pin is a PWM channel, not a BCM GPIO number. */
int setPWM(int pin, int freq, int duty_percent);

#endif