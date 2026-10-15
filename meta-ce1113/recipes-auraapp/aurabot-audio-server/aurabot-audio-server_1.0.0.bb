SUMMARY = "Servidor persistente de audio de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "audio"
RDEPENDS:${PN} = "audio ce1113-audio-config"
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../sonidos:"
SRC_URI = " \
    file://CMakeLists.txt \
    file://aurabot-audio-server.c \
    file://aurabot-audio.service \
    file://startup.mp3 \
    file://modoManual.mp3 \
    file://modoAuto.mp3 \
    file://obstaculo.mp3 \
"
S = "${WORKDIR}"

inherit cmake systemd
SYSTEMD_SERVICE:${PN} = "aurabot-audio.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir} ${D}${datadir}/aurabot/sounds
    install -m 0644 ${WORKDIR}/aurabot-audio.service \
        ${D}${systemd_system_unitdir}/aurabot-audio.service
    install -m 0644 ${WORKDIR}/startup.mp3 \
        ${D}${datadir}/aurabot/sounds/startup.mp3
    install -m 0644 ${WORKDIR}/modoManual.mp3 \
        ${D}${datadir}/aurabot/sounds/modoManual.mp3
    install -m 0644 ${WORKDIR}/modoAuto.mp3 \
        ${D}${datadir}/aurabot/sounds/modoAuto.mp3
    install -m 0644 ${WORKDIR}/obstaculo.mp3 \
        ${D}${datadir}/aurabot/sounds/obstaculo.mp3
}

FILES:${PN} += "${systemd_system_unitdir}/aurabot-audio.service \
    ${datadir}/aurabot/sounds"
