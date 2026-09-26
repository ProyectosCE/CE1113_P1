FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://brcmfmac.cfg \
"

do_configure:append() {
    cat ${WORKDIR}/brcmfmac.cfg >> ${B}/.config
}
