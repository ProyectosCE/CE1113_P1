SUMMARY = "Aplicación y bibliotecas de control de AuraBot"
DESCRIPTION = "Software C de AuraBot compilado dentro del entorno reproducible de Yocto"
HOMEPAGE = "https://github.com/"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://CMakeLists.txt;subdir=source \
    file://src;subdir=source \
    file://libaudio;subdir=source \
    file://libgpio;subdir=source \
    file://libaurabot_hw;subdir=source \
    file://aurabot-audio.init \
    file://aurabot-audio-supervisor \
"

S = "${WORKDIR}/source"

inherit cmake update-rc.d

INITSCRIPT_NAME = "aurabot-audio"
INITSCRIPT_PARAMS = "defaults 70"

EXTRA_OECMAKE = "-DCMAKE_BUILD_TYPE=Release"

# libaudio ejecuta mpg123 mediante execlp() en tiempo de ejecución.
RDEPENDS:${PN} += "audio-mp3"

FILES:${PN}-dev += "${includedir}"

do_install:append() {
    install -d ${D}${sysconfdir}/init.d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/aurabot-audio.init \
        ${D}${sysconfdir}/init.d/aurabot-audio
    install -m 0755 ${WORKDIR}/aurabot-audio-supervisor \
        ${D}${sbindir}/aurabot-audio-supervisor
}

FILES:${PN} += " \
    ${sysconfdir}/init.d/aurabot-audio \
    ${sbindir}/aurabot-audio-supervisor \
"
