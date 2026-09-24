#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/libgpio.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define FIFO "/tmp/mpg123.cmd"
#define PID_FILE "/tmp/mpg123.pid"
#define PATH "/sys/class/gpio"
#define base 512

 
static pid_t g_pid = -1;
static int   g_fd  = -1;
static char  g_fifo_path[256];





//modo = in o out

int pinMode(int pin, const char *MODE){ 
    char comando_export[50];
    char comando_set_mode[50];
    
    snprintf(comando_export, sizeof(comando_export), "echo %d > %s/export", pin + base, PATH);
    snprintf(comando_set_mode, sizeof(comando_set_mode), "echo %s > %s/gpio%d/direction",MODE ,PATH, pin+base);
    system(comando_export); //exporta un pin
    system(comando_set_mode); //setea el modo
    return 0;
}



int digitalWrite(int pin, int value){
    char comando_set_value[50];

    if(value != 0 && value != 1){
        return -1;
    }
    snprintf(comando_set_value, sizeof(comando_set_value), "echo %d > %s/gpio%d/value", value, PATH, pin+base);
    int resultado = system(comando_set_value);
    return resultado;
}


int digitalRead(int pin){
    char path[50];
    int value;

    snprintf(path, sizeof(path), "%s/gpio%d/value", PATH, pin+base);

    FILE *file =fopen(path, "r");

    if(file == NULL){
        return -1;
    }

    fscanf(file, "%d", &value);
    fclose(file);

    return value;

    
}

//pin solo 0 o 1

int setPWM(int pin, int freq, int duty_percent) {
    if (freq <= 0) return -1;
    if (duty_percent < 0) duty_percent = 0;
    if (duty_percent > 100) duty_percent = 100;

    int periodo = 1000000 / freq;
    int duty = (periodo * duty_percent) / 100;

    char comando_pwm_set[100];
    char comando_pwm_period[100];
    char comando_pwm_duty[100];
    char comando_pwm_en[100];

    snprintf(comando_pwm_set, sizeof(comando_pwm_set), "echo %d > /sys/class/pwm/pwmchip%d/export", pin, 0);
    snprintf(comando_pwm_period, sizeof(comando_pwm_period), "echo %d > /sys/class/pwm/pwmchip%d/pwm%d/period", periodo, 0, pin);
    snprintf(comando_pwm_duty, sizeof(comando_pwm_duty), "echo %d > /sys/class/pwm/pwmchip%d/pwm%d/duty_cycle", duty, 0, pin);
    snprintf(comando_pwm_en, sizeof(comando_pwm_en), "echo %d > /sys/class/pwm/pwmchip%d/pwm%d/enable", (duty_percent > 0) ? 1 : 0, 0, pin);
                                    
    system(comando_pwm_set);
    system(comando_pwm_period);
    system(comando_pwm_duty);
    system(comando_pwm_en);

    return 0;
}





//falta meter mas comandos investigar los comandos remotos para mpg123

//ademas de poder cargar una playlist en vez de una cancion



