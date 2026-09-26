FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://httpd-cgi.cfg \
"

do_install:append() {
    :
}