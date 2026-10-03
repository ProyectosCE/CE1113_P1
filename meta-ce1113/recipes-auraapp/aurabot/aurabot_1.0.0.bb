SUMMARY = "Aplicación principal de AuraBot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "pwm"
SRC_URI = "file://CMakeLists.txt file://src/main.c"
S = "${WORKDIR}"

inherit cmake
