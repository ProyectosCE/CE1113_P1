SUMMARY = "Aplicación hola para CE1113"
DESCRIPTION = "Receta base; agregue manualmente fuentes, dependencias e instalación"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Añada manualmente los archivos específicos, por ejemplo:
# SRC_URI = "file://archivo.c file://CMakeLists.txt"
SRC_URI = ""
S = "${WORKDIR}"

# Permite validar la receta antes de añadir contenido instalable.
ALLOW_EMPTY:${PN} = "1"
