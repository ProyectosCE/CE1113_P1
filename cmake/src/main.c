#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdriver.h>




int main(){

    playSound("/home/musica/tiki.mp3");
    sleep(5);

    pauseSound();
    sleep(5);

    return 0;

}