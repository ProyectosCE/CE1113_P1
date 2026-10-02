SUMMARY = "Soporte de reproducción MP3 para AuraBot"
DESCRIPTION = "Paquete de integración que instala ALSA y mpg123 para las aplicaciones de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://test.mp3 \
"

S = "${WORKDIR}"

# La implementación C existente ejecuta mpg123 en modo remoto y, por tanto,
# estas son dependencias de ejecución y no dependencias de compilación.
RDEPENDS:${PN} = " \
    alsa-utils \
    alsa-utils-aplay \
    alsa-utils-amixer \
    alsa-utils-alsamixer \
    mpg123 \
    ce1113-audio-config \
"

do_install() {
    install -d ${D}${datadir}/aurabot/audio
    install -m 0644 ${WORKDIR}/test.mp3 \
        ${D}${datadir}/aurabot/audio/test.mp3
}

FILES:${PN} += " \
    ${datadir}/aurabot/audio/test.mp3 \
"
