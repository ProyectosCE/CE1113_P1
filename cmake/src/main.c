#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdriver.h>




int main(){

    struct Motor motorA = {12 ,13 ,0};

    SetMotor(&motorA);

    SetMoDir(&motorA, 'd');

    sleep(5);

    ApagarMotor(&motorA);

    sleep(5);

    SetMoDir(&motorA, 'i');

    sleep(5);


    return 0;



}