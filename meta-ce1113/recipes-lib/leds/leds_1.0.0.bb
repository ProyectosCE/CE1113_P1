SUMMARY = "Biblioteca compartida de LEDs de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "libgpio"
SRC_URI = "file://CMakeLists.txt file://include/libleds.h file://src/libleds.c"
S = "${WORKDIR}"
inherit cmake
FILES:${PN}-dev += "${includedir}"
