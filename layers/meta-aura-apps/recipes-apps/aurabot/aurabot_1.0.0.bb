SUMMARY = "Aplicación y bibliotecas de control de AuraBot"
DESCRIPTION = "Software C de AuraBot compilado dentro del entorno reproducible de Yocto"
HOMEPAGE = "https://github.com/"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://CMakeLists.txt \
    file://src \
    file://libaudio \
    file://libgpio \
    file://libaurabot_hw \
"

S = "${WORKDIR}"

inherit cmake

EXTRA_OECMAKE = "-DCMAKE_BUILD_TYPE=Release"

FILES:${PN}-dev += "${includedir}"
