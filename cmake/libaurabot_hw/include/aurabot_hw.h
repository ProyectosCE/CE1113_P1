#ifndef LIBDRIVER_H
#define LIBDRIVER_H


//modo = in o out


struct Motor{
    int Pin1;
    int Pin2;
    int pwm;

};


int SetMotor(struct Motor *m);


//apaga el motor pues
int ApagarMotor(struct Motor *m);


//direccion de motor recibe "i" para izquierda y "d" para derecha
int SetMoDir(struct Motor *m, char dir);

//pwm del motor
int SetMoPwm(struct Motor *motor, int duty_percent);




#endif
