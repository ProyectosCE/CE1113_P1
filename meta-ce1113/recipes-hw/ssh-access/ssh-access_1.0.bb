SUMMARY = "Acceso SSH temporal para desarrollo de CE1113"
DESCRIPTION = "Instala Dropbear y habilita su servicio SysV al arrancar."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dropbear ya proporciona y registra su script SysV. Esta receta funciona como
# módulo removible para no acoplar SSH a la receta de imagen principal.
ALLOW_EMPTY:${PN} = "1"
RDEPENDS:${PN} = "dropbear"

