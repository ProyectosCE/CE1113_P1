SUMMARY = "Configuracion automatica WiFi Raspberry Pi"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://wpa_supplicant.conf \
    file://aurabot-wifi.service \
    file://aurabot-wifi-dhcp.service \
    file://aurabot-wifi.timer \
    file://aurabot-captive-dns.service \
    file://udhcpd.conf \
    file://captive_dns.c \
"

RDEPENDS:${PN} += " \
    wpa-supplicant \
    wpa-supplicant-cli \
    busybox \
"

S = "${WORKDIR}"

inherit systemd
SYSTEMD_SERVICE:${PN} = "aurabot-wifi.timer aurabot-wifi.service aurabot-wifi-dhcp.service aurabot-captive-dns.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} ${WORKDIR}/captive_dns.c -o ${B}/aurabot-captive-dns
}

do_install() {
    install -d ${D}${sysconfdir}/wpa_supplicant
    install -d ${D}${sysconfdir}/aurabot
    install -d ${D}${systemd_system_unitdir}
    install -d ${D}${sbindir}

    install -m 0600 \
        ${WORKDIR}/wpa_supplicant.conf \
        ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant.conf

    install -m 0644 ${WORKDIR}/aurabot-wifi.service \
        ${D}${systemd_system_unitdir}/aurabot-wifi.service
    install -m 0644 ${WORKDIR}/aurabot-wifi-dhcp.service \
        ${D}${systemd_system_unitdir}/aurabot-wifi-dhcp.service
    install -m 0644 ${WORKDIR}/aurabot-wifi.timer \
        ${D}${systemd_system_unitdir}/aurabot-wifi.timer
    install -m 0644 ${WORKDIR}/aurabot-captive-dns.service \
        ${D}${systemd_system_unitdir}/aurabot-captive-dns.service
    install -m 0644 ${WORKDIR}/udhcpd.conf ${D}${sysconfdir}/aurabot/udhcpd.conf
    install -m 0755 ${B}/aurabot-captive-dns ${D}${sbindir}/aurabot-captive-dns
}

FILES:${PN} += " \
    ${sysconfdir}/wpa_supplicant \
    ${systemd_system_unitdir}/aurabot-wifi.service \
    ${systemd_system_unitdir}/aurabot-wifi-dhcp.service \
    ${systemd_system_unitdir}/aurabot-wifi.timer \
    ${systemd_system_unitdir}/aurabot-captive-dns.service \
    ${sysconfdir}/aurabot/udhcpd.conf \
    ${sbindir}/aurabot-captive-dns \
"
