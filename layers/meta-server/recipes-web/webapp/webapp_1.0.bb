SUMMARY = "Web application for operaciones"
DESCRIPTION = "Web interface served by BusyBox httpd"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://LICENSE;md5=01a1da8f041f97d0885a60dfc1ad8459"

# DEPENDS = "liboperaciones"
# RDEPENDS:${PN} = "liboperaciones"

SRC_URI = " \
    file://index.html \
    file://style.css \
    file://app.js \
    file://httpd.conf \
    file://LICENSE \
    file://operaciones.cgi.c \
    file://json_response.c \
    file://json_response.h \
    file://CMakeLists.txt \
"

S = "${WORKDIR}"

inherit cmake

do_install:append() {
    install -d ${D}/www
    install -d ${D}/www/cgi-bin
    install -d ${D}/etc

    install -m 0644 ${WORKDIR}/index.html \
        ${D}/www/index.html

    install -m 0644 ${WORKDIR}/style.css \
        ${D}/www/style.css

    install -m 0644 ${WORKDIR}/app.js \
        ${D}/www/app.js

    install -m 0644 ${WORKDIR}/httpd.conf \
        ${D}/etc/httpd.conf
}

FILES:${PN} += " \
    /www \
    /www/index.html \
    /www/style.css \
    /www/app.js \
    /www/cgi-bin \
    /www/cgi-bin/operaciones.cgi \
    /etc/httpd.conf \
"
