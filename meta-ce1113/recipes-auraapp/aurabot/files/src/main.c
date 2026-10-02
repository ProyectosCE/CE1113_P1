#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <aurabot_hw.h>
#include <libgpio.h>


int main(){

    int pin = 17; // GPIO pin number
    int freq = 2; // Frequency in Hz
    int duty = 50; // Duty cycle in percentage

    setPWM(pin, freq, duty);  // Motor 1: 100 Hz, 50 %

    while(1){
        sleep(1);
    }


}
