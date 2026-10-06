SUMMARY = "Web application for operaciones"
DESCRIPTION = "Web interface served by BusyBox httpd"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://LICENSE;md5=01a1da8f041f97d0885a60dfc1ad8459"

RDEPENDS:${PN} += "busybox"
DEPENDS += "aurabot-api"

# Credenciales iniciales para desarrollo. En una imagen entregable deben
# sobrescribirse en conf/local.conf; solo el hash resultante llega al rootfs.
AURABOT_WEB_USERNAME ?= "admin"
AURABOT_WEB_PASSWORD ?= "AuraBot!"
AURABOT_WEB_AUTH_SALT ?= "CE1113-AuraBot-development-salt"
AURABOT_WEB_AUTH_ITERATIONS ?= "60000"

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
    file://sha256.c \
    file://sha256.h \
    file://auth.js \
    file://robot-control.js \
    file://CMakeLists.txt \
    file://webapp-httpd.init \
    file://webapp-httpd-supervisor \
"

S = "${WORKDIR}"

inherit cmake update-rc.d

python do_generate_web_auth() {
    import hashlib
    import os
    import re

    username = d.getVar("AURABOT_WEB_USERNAME") or ""
    password = d.getVar("AURABOT_WEB_PASSWORD") or ""
    salt_seed = d.getVar("AURABOT_WEB_AUTH_SALT") or ""
    try:
        iterations = int(d.getVar("AURABOT_WEB_AUTH_ITERATIONS") or "0")
    except ValueError:
        bb.fatal("AURABOT_WEB_AUTH_ITERATIONS debe ser un entero")
    if not re.fullmatch(r"[A-Za-z0-9_.-]{1,32}", username):
        bb.fatal("AURABOT_WEB_USERNAME contiene caracteres no permitidos")
    if len(password) < 12:
        bb.fatal("AURABOT_WEB_PASSWORD debe tener al menos 12 caracteres")
    if not salt_seed:
        bb.fatal("AURABOT_WEB_AUTH_SALT no puede estar vacío")
    if iterations < 1000 or iterations > 1000000:
        bb.fatal("AURABOT_WEB_AUTH_ITERATIONS debe estar entre 1000 y 1000000")

    salt = hashlib.sha256(salt_seed.encode("utf-8")).digest()[:16]
    digest = hashlib.sha256(salt + password.encode("utf-8")).digest()
    for _ in range(1, iterations):
        digest = hashlib.sha256(digest + salt).digest()

    destination = os.path.join(d.getVar("WORKDIR"), "aurabot-web-auth.conf")
    with open(destination, "w", encoding="ascii") as config:
        config.write(f"{username}:{salt.hex()}:{iterations}:{digest.hex()}\n")
}

addtask generate_web_auth after do_unpack before do_install

INITSCRIPT_NAME = "webapp-httpd"
INITSCRIPT_PARAMS = "defaults 80"

do_install:append() {
    install -d ${D}/www
    install -d ${D}/www/cgi-bin
    install -d ${D}${sysconfdir}/aurabot
    install -d ${D}${sysconfdir}/init.d ${D}${sbindir}

    install -m 0644 ${WORKDIR}/index.html \
        ${D}/www/index.html

    install -m 0644 ${WORKDIR}/style.css \
        ${D}/www/style.css

    install -m 0644 ${WORKDIR}/app.js \
        ${D}/www/app.js
    install -m 0644 ${WORKDIR}/auth.js ${D}/www/auth.js
    install -m 0644 ${WORKDIR}/robot-control.js ${D}/www/robot-control.js

    install -m 0600 ${WORKDIR}/aurabot-web-auth.conf \
        ${D}${sysconfdir}/aurabot/web-auth.conf

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
    /www/auth.js \
    /www/cgi-bin \
    /www/cgi-bin/operaciones.cgi \
    /etc/httpd.conf \
    ${sysconfdir}/aurabot/web-auth.conf \
    ${sysconfdir}/init.d/webapp-httpd \
    ${sbindir}/webapp-httpd-supervisor \
"

CONFFILES:${PN} += "${sysconfdir}/aurabot/web-auth.conf"
