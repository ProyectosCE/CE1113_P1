SUMMARY = "Aplicación de prueba para el módulo de operaciones"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

# Como es un ejemplo, solo crearemos un script de bash vacío
do_install() {
    install -d ${D}${bindir}
    echo "#!/bin/sh" > ${D}${bindir}/mi_operacion
    echo "echo 'Operaciones funcionando'" >> ${D}${bindir}/mi_operacion
    chmod 0755 ${D}${bindir}/mi_operacion
}
