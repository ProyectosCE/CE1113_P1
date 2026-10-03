SUMMARY = "Montaje persistente de la partición musical AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://audio-storage.init"

inherit update-rc.d
INITSCRIPT_NAME = "audio-storage"
INITSCRIPT_PARAMS = "defaults 20"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/audio-storage.init ${D}${sysconfdir}/init.d/audio-storage
}

FILES:${PN} += "${sysconfdir}/init.d/audio-storage"
