SUMMARY = "Servidor persistente de audio de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "audio"
RDEPENDS:${PN} = "audio ce1113-audio-config"
SRC_URI = " \
    file://CMakeLists.txt \
    file://aurabot-audio-server.c \
    file://aurabot-audio.service \
"
S = "${WORKDIR}"

inherit cmake systemd
SYSTEMD_SERVICE:${PN} = "aurabot-audio.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/aurabot-audio.service \
        ${D}${systemd_system_unitdir}/aurabot-audio.service
}

FILES:${PN} += "${systemd_system_unitdir}/aurabot-audio.service"
