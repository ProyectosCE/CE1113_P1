# Este archivo amplía ce1113-p1.bb sin duplicar la imagen principal.
# Añadimos la configuracion de red y Wi-Fi a la imagen

IMAGE_INSTALL:append = " \
    wifi-config \
    linux-firmware-rpidistro-bcm43455 \
    brcmfmac-firmware-fix \
    kernel-module-brcmfmac \
    kernel-module-brcmfmac-wcc \
    wpa-supplicant \
    wireless-regdb \
"
