#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdriver.h>
#include <libgpio.h>
#include <libaudio.h>



int main(){

    pinMode(0, "out");

    digitalWrite(0, 1);


    
   IniciarSonido("plughw:2,0");

   sleep(1);

   CargarSonido("/home/musica/lean.mp3");
    
   sleep(5);

   Pausar();


}