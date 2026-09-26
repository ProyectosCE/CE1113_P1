#ifndef LIBPWM_H
#define LIBPWM_H

/*
 * Configure a PWM channel
 *
 * channel: PWM channel (0 or 1), not a BCM GPIO number
 * freq: frequency in Hz
 * duty_percent: duty cycle from 0 to 100
 *
 * Returns 0 on success and -1 on error
 */
int pwm_set(int channel, int freq, int duty_percent);

/*
 * Disable and release a channel exported by this process
 * Channels exported by other processes are not unexported
 */
int pwm_release(int channel);

#endif