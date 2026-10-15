SUMMARY = "Web application for operaciones"
DESCRIPTION = "Web interface served by BusyBox httpd"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://LICENSE;md5=01a1da8f041f97d0885a60dfc1ad8459"

RDEPENDS:${PN} += "busybox"
DEPENDS += "aurabot-api sqlite3 libxcrypt"

# Cuenta inicial de desarrollo. La imagen contiene exclusivamente el hash
# bcrypt; una imagen entregable debe sobrescribir usuario y hash.
AURABOT_WEB_USERNAME ?= "admin"
AURABOT_WEB_PASSWORD_HASH ?= "$2b$12$kR7JbG3QmN9sT2vX5zA8CeAeZWc7ZNo3Htk6VNpD.rBXRErdNIpb."

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
    file://web_auth.c \
    file://web_auth.h \
    file://auth.js \
    file://users.js \
    file://robot-control.js \
    file://CMakeLists.txt \
    file://webapp-httpd.service \
"

S = "${WORKDIR}"

inherit cmake systemd

python do_generate_web_auth() {
    import os
    import re

    username = d.getVar("AURABOT_WEB_USERNAME") or ""
    password_hash = d.getVar("AURABOT_WEB_PASSWORD_HASH") or ""
    if not re.fullmatch(r"[A-Za-z0-9_.-]{1,32}", username):
        bb.fatal("AURABOT_WEB_USERNAME contiene caracteres no permitidos")
    if not re.fullmatch(r"\$2b\$1[0-9]\$[./A-Za-z0-9]{53}", password_hash):
        bb.fatal("AURABOT_WEB_PASSWORD_HASH debe ser un hash bcrypt $2b$ con costo >= 10")

    destination = os.path.join(d.getVar("WORKDIR"), "aurabot-web-auth-bootstrap.conf")
    with open(destination, "w", encoding="ascii") as config:
        config.write(f"{username}:admin:{password_hash}\n")
}

addtask generate_web_auth after do_unpack before do_install

SYSTEMD_SERVICE:${PN} = "webapp-httpd.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}/www
    install -d ${D}/www/cgi-bin
    install -d ${D}${sysconfdir}/aurabot
    install -d ${D}${systemd_system_unitdir}

    install -m 0644 ${WORKDIR}/index.html \
        ${D}/www/index.html

    install -m 0644 ${WORKDIR}/style.css \
        ${D}/www/style.css

    install -m 0644 ${WORKDIR}/app.js \
        ${D}/www/app.js
    install -m 0644 ${WORKDIR}/auth.js ${D}/www/auth.js
    install -m 0644 ${WORKDIR}/users.js ${D}/www/users.js
    install -m 0644 ${WORKDIR}/robot-control.js ${D}/www/robot-control.js

    install -m 0600 ${WORKDIR}/aurabot-web-auth-bootstrap.conf \
        ${D}${sysconfdir}/aurabot/web-auth-bootstrap.conf

    install -m 0644 ${WORKDIR}/httpd.conf \
        ${D}/etc/httpd.conf

    install -m 0644 ${WORKDIR}/webapp-httpd.service \
        ${D}${systemd_system_unitdir}/webapp-httpd.service
}

FILES:${PN} += " \
    /www \
    /www/index.html \
    /www/style.css \
    /www/app.js \
    /www/auth.js \
    /www/cgi-bin \
    /www/cgi-bin/operaciones.cgi \
    /etc/httpd.conf \
    ${sysconfdir}/aurabot/web-auth-bootstrap.conf \
    ${systemd_system_unitdir}/webapp-httpd.service \
"

CONFFILES:${PN} += "${sysconfdir}/aurabot/web-auth-bootstrap.conf"
