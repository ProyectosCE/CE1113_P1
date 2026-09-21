# Instituto Tecnológico de Costa Rica

## Proyecto I

### Sistema embebido a la medida para un robot aspiradora autónomo con reproducción de audio y control remoto

## Sistemas Empotrados (CE1113)

---

### Estudiantes

- José Bernardo Barquero Bonilla (2023150476)
- Jose Eduardo Campos Salazar (2023135620)
- Jimmy Feng Feng (2023060347)
- Alexander Montero Vargas (2023166058)

### Profesor

- Dr.-Ing Jeferson González Gómez <jgonzalez@itcr.ac.cr>

---

### Objetivo

Mediante el desarrollo de este proyecto, cada grupo de trabajo aplicará los conceptos y herramientas de software y hardware vistos en el curso en el diseño de un sistema embebido a la medida que controla un robot aspiradora autónomo. El sistema deberá ser capaz de navegar de forma autónoma evitando obstáculos, reproducir audio (archivos MP3), y además poder ser operado de forma remota a través de un servidor web o aplicación móvil mediante conectividad WiFi/Bluetooth.


##intrucciones

antes del make:

cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm_toolchain.cmake    -DCMAKE_INSTALL_PREFIX:PATH=$HOME/build/usr

para configuracion de audio:


mpg123 -o alsa -a plughw:2,0 tu_archivo.mp3 

para segundo plano:


esto es lo que hace la funcion de cargar 

mkfifo /tmp/mpg123.cmd

mpg123 -R \
    -o alsa \
    -a plughw:2,0 \
    < /tmp/mpg123.cmd \
    > /tmp/mpg123.log 2>&1 &

echo $! > /tmp/mpg123.pid

exec 3>/tmp/mpg123.cmd

echo "LOAD cancion.mp3" >&3

