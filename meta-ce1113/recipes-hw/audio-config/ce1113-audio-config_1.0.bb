SUMMARY = "Configuración del audio analógico de CE1113"
DESCRIPTION = "Configura snd-bcm2835 y verifica la tarjeta Headphones al arrancar"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://snd-bcm2835.conf \
    file://aurabot-audio-kernel.init \
"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "aurabot-audio-kernel"
INITSCRIPT_PARAMS = "start 07 S ."

RDEPENDS:${PN}:append:raspberrypi4 = " kernel-module-snd-bcm2835"

do_install() {
    install -d ${D}${sysconfdir}/modprobe.d
    install -d ${D}${sysconfdir}/init.d
    install -m 0644 ${WORKDIR}/snd-bcm2835.conf \
        ${D}${sysconfdir}/modprobe.d/snd-bcm2835.conf
    install -m 0755 ${WORKDIR}/aurabot-audio-kernel.init \
        ${D}${sysconfdir}/init.d/aurabot-audio-kernel
}

FILES:${PN} += " \
    ${sysconfdir}/modprobe.d/snd-bcm2835.conf \
    ${sysconfdir}/init.d/aurabot-audio-kernel \
"
