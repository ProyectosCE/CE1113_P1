#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libpwm.h>

int main(){

    int pin = 17; // GPIO pin number
    int freq = 2; // Frequency in Hz
    int duty = 50; // Duty cycle in percentage

    if (setPWM(pin, freq, duty) != 0) {
        perror("aurabot: no se pudo iniciar PWM en GPIO17");
        return EXIT_FAILURE;
    }

    printf("AuraBot: PWM activo en GPIO%d (%d Hz, %d%%).\n",
           pin, freq, duty);
    fflush(stdout);

    while(1){
        sleep(1);
    }
}
