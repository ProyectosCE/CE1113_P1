#ifndef LIBGPIO_H
#define LIBGPIO_H

typedef struct {
    int pin;
    int freq;
    int duty;
} PWMData;



int pinMode(int pin, const char *mode);

int digitalWrite(int pin, int value);

int digitalRead(int pin);

int setPWM(int pin, int freq, int duty_percent);

void *pwmThread(void *arg);

#endif
