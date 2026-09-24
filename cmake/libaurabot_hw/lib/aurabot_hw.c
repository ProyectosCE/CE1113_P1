#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgpio.h>
#include "../include/aurabot_hw.h"
#define FREQ_MOTOR_HZ 1000

aurabot_hw_status_t aurabot_hw_init(void)
{
    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_hw_shutdown(void)
{
    return AURABOT_HW_OK;
}

int SetMotor(struct Motor *m){

    int pin1 =  m->Pin1;
    int pin2 =  m->Pin2;

    pinMode(pin1, "out");
    pinMode(pin2, "out");
    return 0;

}


//apaga el motor pues
int ApagarMotor(struct Motor *m){
    int pin1 =  m->Pin1;
    int pin2 =  m->Pin2;
    digitalWrite(pin1, 0);
    digitalWrite(pin2, 0);

    return 0;

}



//direccion de motor recibe "i" para izquierda y "d" para derecha
int SetMoDir(struct Motor *m, char dir){

    int pin1 =  m->Pin1;
    int pin2 =  m->Pin2;

    int value1;
    int value2;

    if(dir == 'i'){
        value1 = 0;
        value2 = 1;

    }else if(dir == 'd'){
        value1 = 1;
        value2 = 0;
    }else{
        value1 = 0;
        value2 = 0;
    }

    digitalWrite(pin1, value1);
    digitalWrite(pin2, value2);

    return 0;

}



//pwm del motor
int SetMoPwm(struct Motor *motor, int duty_percent) {
    if (motor == NULL) return -1;
    return setPWM(motor->pwm, FREQ_MOTOR_HZ, duty_percent);
}


