FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# linux-raspberrypi inherits linux-yocto.inc, so .cfg entries in SRC_URI are
# merged and audited by the normal kernel configuration fragment mechanism.
SRC_URI:append:raspberrypi4 = " file://pwm.cfg"
