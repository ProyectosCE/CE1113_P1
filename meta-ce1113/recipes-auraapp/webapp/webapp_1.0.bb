SUMMARY = "Web application for operaciones"
DESCRIPTION = "Web interface served by BusyBox httpd"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://LICENSE;md5=01a1da8f041f97d0885a60dfc1ad8459"

RDEPENDS:${PN} += "busybox"
DEPENDS += "aurabot-api"

SRC_URI = " \
    file://index.html \
    file://style.css \
    file://app.js \
    file://httpd.conf \
    file://LICENSE \
    file://operaciones.cgi.c \
    file://json_response.c \
    file://json_response.h \
    file://robot_web.c \
    file://robot_web.h \
    file://robot-control.js \
    file://CMakeLists.txt \
    file://webapp-httpd.init \
    file://webapp-httpd-supervisor \
"

S = "${WORKDIR}"

inherit cmake update-rc.d

INITSCRIPT_NAME = "webapp-httpd"
INITSCRIPT_PARAMS = "defaults 80"

do_install:append() {
    install -d ${D}/www
    install -d ${D}/www/cgi-bin
    install -d ${D}/etc
    install -d ${D}${sysconfdir}/init.d ${D}${sbindir}

    install -m 0644 ${WORKDIR}/index.html \
        ${D}/www/index.html

    install -m 0644 ${WORKDIR}/style.css \
        ${D}/www/style.css

    install -m 0644 ${WORKDIR}/app.js \
        ${D}/www/app.js
    install -m 0644 ${WORKDIR}/robot-control.js ${D}/www/robot-control.js

    install -m 0644 ${WORKDIR}/httpd.conf \
        ${D}/etc/httpd.conf

    install -m 0755 ${WORKDIR}/webapp-httpd.init \
        ${D}${sysconfdir}/init.d/webapp-httpd
    install -m 0755 ${WORKDIR}/webapp-httpd-supervisor \
        ${D}${sbindir}/webapp-httpd-supervisor
}

FILES:${PN} += " \
    /www \
    /www/index.html \
    /www/style.css \
    /www/app.js \
    /www/cgi-bin \
    /www/cgi-bin/operaciones.cgi \
    /etc/httpd.conf \
    ${sysconfdir}/init.d/webapp-httpd \
    ${sbindir}/webapp-httpd-supervisor \
"
