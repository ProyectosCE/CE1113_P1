SUMMARY = "Soporte de reproducción MP3 para AuraBot"
DESCRIPTION = "Paquete de integración que instala ALSA y mpg123 para las aplicaciones de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://test.mp3 \
    file://snd-bcm2835.conf \
    file://aurabot-audio-kernel.init \
"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "aurabot-audio-kernel"
INITSCRIPT_PARAMS = "start 07 S ."

# La implementación C existente ejecuta mpg123 en modo remoto y, por tanto,
# estas son dependencias de ejecución y no dependencias de compilación.
RDEPENDS:${PN} = " \
    alsa-utils \
    alsa-utils-aplay \
    alsa-utils-amixer \
    alsa-utils-alsamixer \
    mpg123 \
"

# dtparam=audio=on necesita este módulo para materializar la tarjeta analógica
# en /proc/asound. HDMI puede aparecer aunque este paquete no esté instalado.
RDEPENDS:${PN}:append:raspberrypi4 = " kernel-module-snd-bcm2835"

do_install() {
    install -d ${D}${datadir}/aurabot/audio
    install -d ${D}${sysconfdir}/modprobe.d
    install -d ${D}${sysconfdir}/init.d
    install -m 0644 ${WORKDIR}/test.mp3 \
        ${D}${datadir}/aurabot/audio/test.mp3
    install -m 0644 ${WORKDIR}/snd-bcm2835.conf \
        ${D}${sysconfdir}/modprobe.d/snd-bcm2835.conf
    install -m 0755 ${WORKDIR}/aurabot-audio-kernel.init \
        ${D}${sysconfdir}/init.d/aurabot-audio-kernel
}

FILES:${PN} += " \
    ${datadir}/aurabot/audio/test.mp3 \
    ${sysconfdir}/modprobe.d/snd-bcm2835.conf \
    ${sysconfdir}/init.d/aurabot-audio-kernel \
"
