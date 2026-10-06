SUMMARY = "Hardware, audio y conectividad de CE1113"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    audio-storage \
    ce1113-audio-config \
    udev-rules-rpi \
    packagegroup-base-alsa \
    wifi-config \
    linux-firmware-rpidistro-bcm43455 \
    brcmfmac-firmware-fix \
    kernel-module-brcmfmac \
    kernel-module-brcmfmac-wcc \
    wpa-supplicant \
    wireless-regdb \
    systemd-analyze \
"
