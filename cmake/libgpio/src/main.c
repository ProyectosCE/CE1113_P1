#include "libgpio.h"
#include <stdio.h>

int main(){

    //gio de salida, 12 porque es el que tiene pwm0


    setPWM(1, 1, 50);


    setPWM(0, 1, 50);


    return 0;

}