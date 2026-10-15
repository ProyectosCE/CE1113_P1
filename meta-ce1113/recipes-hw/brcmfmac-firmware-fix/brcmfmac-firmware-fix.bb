SUMMARY = "Fix firmware aliases for Raspberry Pi BCM43455"
LICENSE = "MIT"

RDEPENDS:${PN} = "linux-firmware-rpidistro-bcm43455"

do_install() {
    install -d ${D}${nonarch_base_libdir}/firmware/brcm
    ln -sf brcmfmac43455-sdio.raspberrypi,4-model-b.bin \
        ${D}${nonarch_base_libdir}/firmware/brcm/brcmfmac43455-sdio.bin
}

FILES:${PN} += "${nonarch_base_libdir}/firmware/brcm/brcmfmac43455-sdio.bin"
