SUMMARY = "Composición completa de la imagen CE1113"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    packagegroup-ce1113-lib \
    packagegroup-ce1113-hw \
    packagegroup-ce1113-auraapp \
    packagegroup-ce1113-test \
"
