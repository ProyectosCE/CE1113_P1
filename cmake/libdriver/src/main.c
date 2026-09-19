#include <stdio.h>
#include <libdriver.h>



int main(){

    struct Motor motorA = {13 ,12 ,0};

    SetMotor(&motorA);

    SetMoDir(&motorA, 'i');

    sleep(5);

    SetMoDir(&motorA, 'd');

    sleep(5);

    ApagarMotor(&motorA);

    return 0;



}