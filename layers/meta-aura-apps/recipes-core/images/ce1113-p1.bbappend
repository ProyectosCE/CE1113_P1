# AuraBot se instala desde su capa de aplicaciones sin modificar meta-ce1113.
IMAGE_INSTALL:append = " aurabot"

# Se mantiene explícito para que el propósito de la imagen sea visible. La
# receta audio-mp3 es la responsable de declarar todos los paquetes ALSA/MP3.
IMAGE_INSTALL:append = " audio-mp3"

# La imagen de Raspberry usada durante las primeras pruebas funcionales se
# basaba en core-image-base/rpi-test-image. ce1113-p1 hereda la variante
# minimal y, por ello, no incorpora automáticamente el soporte completo de la
# máquina ni el grupo ALSA. Se restauran aquí sin modificar meta-ce1113.
IMAGE_INSTALL:append:raspberrypi4 = " \
    packagegroup-machine-base \
    packagegroup-base-alsa \
"

IMAGE_INSTALL:append = " hola"
