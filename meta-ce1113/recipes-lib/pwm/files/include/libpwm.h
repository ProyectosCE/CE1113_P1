#ifndef LIBPWM_H
#define LIBPWM_H

int setPWM(int pin, int frequency_hz, int duty_percent);
int stopPWM(int pin);

#endif
