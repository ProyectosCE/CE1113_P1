SUMMARY = "Biblioteca GPIO compartida de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://CMakeLists.txt file://include/libgpio.h file://src/libgpio.c"
S = "${WORKDIR}"

inherit cmake

FILES:${PN}-dev += "${includedir}"
