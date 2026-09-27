# dtparam=audio=on debería añadir estos parámetros mediante Device Tree. Se
# declaran también en cmdline para que el resultado no dependa del firmware.
CMDLINE:append:raspberrypi4 = " snd_bcm2835.enable_headphones=1 snd_bcm2835.enable_hdmi=0"
