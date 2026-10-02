FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://busybox-wifi.cfg \
    file://httpd-cgi.cfg \
"

BUSYBOX_CONFIG_FRAGMENT += " \
    ${WORKDIR}/busybox-wifi.cfg \
    ${WORKDIR}/httpd-cgi.cfg \
"
