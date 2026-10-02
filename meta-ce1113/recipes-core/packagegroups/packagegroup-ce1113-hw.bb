SUMMARY = "Hardware, audio y conectividad de CE1113"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    audio-mp3 \
    packagegroup-machine-base \
    packagegroup-base-alsa \
    wifi-config \
    linux-firmware-rpidistro-bcm43455 \
    brcmfmac-firmware-fix \
    kernel-module-brcmfmac \
    kernel-module-brcmfmac-wcc \
    wpa-supplicant \
    wireless-regdb \
"
