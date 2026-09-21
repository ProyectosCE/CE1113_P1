#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdriver.h>
#include <libgpio.h>



int main(){
 
   IniciarSonido("plughw:2,0");

   sleep(1);

   CargarSonido("/home/musica/lean.mp3");
    
   sleep(5);

   Pausar();


}