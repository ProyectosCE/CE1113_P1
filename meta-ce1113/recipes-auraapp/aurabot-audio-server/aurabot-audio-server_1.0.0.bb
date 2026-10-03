SUMMARY = "Servidor persistente de audio de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "audio"
RDEPENDS:${PN} = "audio ce1113-audio-config"
SRC_URI = "file://CMakeLists.txt file://aurabot-audio-server.c file://aurabot-audio.init file://aurabot-audio-supervisor"
S = "${WORKDIR}"

inherit cmake update-rc.d
INITSCRIPT_NAME = "aurabot-audio"
INITSCRIPT_PARAMS = "defaults 70"

do_install:append() {
    install -d ${D}${sysconfdir}/init.d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/aurabot-audio.init ${D}${sysconfdir}/init.d/aurabot-audio
    install -m 0755 ${WORKDIR}/aurabot-audio-supervisor ${D}${sbindir}/aurabot-audio-supervisor
}

FILES:${PN} += "${sysconfdir}/init.d/aurabot-audio ${sbindir}/aurabot-audio-supervisor"
