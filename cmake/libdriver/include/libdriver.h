#ifndef LIBDRIVER_H
#define LIBDRIVER_H


//modo = in o out


struct Motor{
    int Pin1;
    int Pin2;
    int pwm;

};

int pinMode(int pin, const char *MODE);


int digitalWrite(int pin, int value);

int digitalRead(int pin);
//pin solo 0 o 1

int blink(int pin, int freq, int duration);

#endif
