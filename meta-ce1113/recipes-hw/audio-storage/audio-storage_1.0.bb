SUMMARY = "Montaje persistente de la partición musical AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://media-audio.mount"

inherit systemd
SYSTEMD_SERVICE:${PN} = "media-audio.mount"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    install -d ${D}${systemd_system_unitdir} ${D}/media/audio
    install -m 0644 ${WORKDIR}/media-audio.mount \
        ${D}${systemd_system_unitdir}/media-audio.mount
}

FILES:${PN} += "${systemd_system_unitdir}/media-audio.mount /media/audio"
