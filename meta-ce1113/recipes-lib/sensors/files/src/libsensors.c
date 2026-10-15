#include <libgpio.h>
#include <libsensors.h>

int sensorReadDigital(int pin, int active_low)
{
    int value;
    if (pinMode(pin, "in") != 0 || (value = digitalRead(pin)) < 0) return -1;
    return active_low ? !value : value;
}
