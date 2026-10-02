SUMMARY = "Composición completa de la imagen CE1113"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    packagegroup-ce1113-hw \
    packagegroup-ce1113-webapp \
    packagegroup-ce1113-api \
    packagegroup-ce1113-auraapp \
"
