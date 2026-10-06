SUMMARY = "Configuracion automatica WiFi Raspberry Pi"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://wpa_supplicant.conf \
    file://aurabot-wifi.service \
    file://aurabot-wifi-dhcp.service \
"

RDEPENDS:${PN} += " \
    wpa-supplicant \
    wpa-supplicant-cli \
    busybox \
"

S = "${WORKDIR}"

inherit systemd
SYSTEMD_SERVICE:${PN} = "aurabot-wifi.service aurabot-wifi-dhcp.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    install -d ${D}${sysconfdir}/wpa_supplicant
    install -d ${D}${systemd_system_unitdir}

    install -m 0600 \
        ${WORKDIR}/wpa_supplicant.conf \
        ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant.conf

    install -m 0644 ${WORKDIR}/aurabot-wifi.service \
        ${D}${systemd_system_unitdir}/aurabot-wifi.service
    install -m 0644 ${WORKDIR}/aurabot-wifi-dhcp.service \
        ${D}${systemd_system_unitdir}/aurabot-wifi-dhcp.service
}

FILES:${PN} += " \
    ${sysconfdir}/wpa_supplicant \
    ${systemd_system_unitdir}/aurabot-wifi.service \
    ${systemd_system_unitdir}/aurabot-wifi-dhcp.service \
"
