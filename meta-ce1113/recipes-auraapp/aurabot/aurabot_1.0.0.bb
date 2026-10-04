SUMMARY = "Aplicación principal de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "pwm libgpio"
SRC_URI = "file://CMakeLists.txt file://src/main.c file://aurabot.init file://aurabot-supervisor"
S = "${WORKDIR}"

inherit cmake update-rc.d
INITSCRIPT_NAME = "aurabot"
INITSCRIPT_PARAMS = "defaults 70"

do_install:append() {
    install -d ${D}${sysconfdir}/init.d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/aurabot.init ${D}${sysconfdir}/init.d/aurabot
    install -m 0755 ${WORKDIR}/aurabot-supervisor ${D}${sbindir}/aurabot-supervisor
}

FILES:${PN} += "${sysconfdir}/init.d/aurabot ${sbindir}/aurabot-supervisor"
