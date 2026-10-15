SUMMARY = "API compartida de control de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://CMakeLists.txt;subdir=source \
    file://include/aurabot.h;subdir=source \
    file://include/aurabot_protocol.h;subdir=source \
    file://src/aurabot.c;subdir=source \
    file://tools/aurabotctl.c;subdir=source \
"
S = "${WORKDIR}/source"

# do_unpack reemplaza fuentes fuera de fakeroot. No registrar esas rutas en
# pseudo: do_package puede haber creado enlaces duros a las fuentes de debug.
# Mantener image/ y package/ fuera de esta exclusión para rastrear su instalación.
PSEUDO_IGNORE_PATHS:append = ",${S}/"

inherit cmake

FILES:${PN}-dev += "${includedir}"
