#ifndef LIBPWM_H
#define LIBPWM_H

/* pin represents a PWM channel, not a BCM GPIO number. */
int pwm_set(int pin, int freq, int duty_percent);

#endif