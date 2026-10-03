SUMMARY = "Biblioteca de audio compartida de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://CMakeLists.txt file://include/libaudio.h file://src/libaudio.c"
S = "${WORKDIR}"

inherit cmake

RDEPENDS:${PN} = "mpg123 alsa-utils-amixer"
FILES:${PN}-dev += "${includedir}"
