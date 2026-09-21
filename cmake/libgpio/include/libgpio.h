#ifndef LIBGPIO_H
#define LIBGPIO_H


//modo = in o out

typedef struct {
    char *path;
    char *title;
    char *artist;
} Song;


struct Playlist {
    Song *songs;
    size_t count;
    size_t capacity;
    size_t current;
    int shuffled;
};



int pinMode(int pin, const char *MODE);


int digitalWrite(int pin, int value);

int digitalRead(int pin);
//pin solo 0 o 1

int blink(int pin, int freq, int duration);

int playSound(const char *file_path);


#endif
