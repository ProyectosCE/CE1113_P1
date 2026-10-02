SUMMARY = "Configuracion automatica WiFi Raspberry Pi"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"


SRC_URI = " \
    file://wpa_supplicant.conf \
    file://wifi-init.sh \
"


RDEPENDS:${PN} += " \
    wpa-supplicant \
    busybox \
"


S = "${WORKDIR}"


inherit update-rc.d


INITSCRIPT_NAME = "wifi-init"
INITSCRIPT_PARAMS = "defaults 99"


do_install() {

    install -d ${D}${sysconfdir}/wpa_supplicant
    install -d ${D}${sysconfdir}/init.d


    install -m 0600 \
    ${WORKDIR}/wpa_supplicant.conf \
    ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant.conf


    install -m 0755 \
    ${WORKDIR}/wifi-init.sh \
    ${D}${sysconfdir}/init.d/wifi-init
}


FILES:${PN} += " \
${sysconfdir}/wpa_supplicant \
${sysconfdir}/init.d \
"