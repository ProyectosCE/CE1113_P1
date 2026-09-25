#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <aurabot_hw.h>
#include <libgpio.h>


int main(){

    aurabot_hw_init();

    pinMode(0, "out");
    digitalWrite(0, 1);

   sleep(1);

   aurabot_audio_play_file("/home/musica/lean.mp3");
    
   sleep(5);

   aurabot_audio_pause();

   aurabot_hw_shutdown();


}