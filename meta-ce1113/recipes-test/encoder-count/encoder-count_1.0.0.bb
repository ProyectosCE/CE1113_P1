SUMMARY = "Aplicación para contar los ticks acumulados del encoder"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "libgpio"
SRC_URI = " \
    file://CMakeLists.txt \
    file://encoder_count.c \
"
S = "${WORKDIR}"

inherit cmake
