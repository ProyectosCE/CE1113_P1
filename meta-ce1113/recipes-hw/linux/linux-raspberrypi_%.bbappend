FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://brcmfmac.cfg \
"

# El jack analógico se materializa mediante este módulo durante el arranque.
KERNEL_MODULE_AUTOLOAD:append:raspberrypi4 = " snd-bcm2835"

do_configure:append() {
    cat ${WORKDIR}/brcmfmac.cfg >> ${B}/.config
}
