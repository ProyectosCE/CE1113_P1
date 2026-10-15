#include <libgpio.h>
#include <libleds.h>

int ledSet(int pin, int active_high, int enabled)
{
    if (pinMode(pin, "out") != 0) return -1;
    return digitalWrite(pin, active_high ? !!enabled : !enabled);
}

int ledGet(int pin, int active_high)
{
    int value;
    if (pinMode(pin, "in") != 0 || (value = digitalRead(pin)) < 0) return -1;
    return active_high ? value : !value;
}
