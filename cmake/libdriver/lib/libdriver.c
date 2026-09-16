#include <libgpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmake/libgpio/include/libgpio.h>



struct Motor{

    int Pin1;
    int Pin2;
    int pwm;


};



//apaga el motor pues
int ApagarMotor(struct Motor *motor){
    


}



//direccion de motor
int SetMoDir(struct Motor *motor){



}



//pwm del motor
int SetMoPwm(struct Motor *motor){



}


