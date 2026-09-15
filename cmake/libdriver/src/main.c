#include "libgpio.h"
#include <stdio.h>

int main(){

    //gio de salida, 12 porque es el que tiene pwm0
    pinMode(12, "out");
    pinMode(13, "out");

    //gpio de entrada

    pinMode(17, "in");

    //escribir al 13 un 1
    digitalWrite(13, 1);

    //establecer blink al pwm0 = 12, frecuencia 1khz y duracion 5 segundos
    int valor_leido = digitalRead(17);
    
    printf("valor leido en el pin 17: %d", valor_leido);

    return 0;

}