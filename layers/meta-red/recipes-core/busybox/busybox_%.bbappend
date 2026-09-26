FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://busybox-wifi.cfg"

BUSYBOX_CONFIG_FRAGMENT += "${WORKDIR}/busybox-wifi.cfg"

do_install:append() {
    :
}
