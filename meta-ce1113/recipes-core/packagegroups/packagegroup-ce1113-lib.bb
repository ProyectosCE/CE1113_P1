SUMMARY = "Bibliotecas dinámicas de AuraBot"
LICENSE = "MIT"
inherit packagegroup

# Las bibliotecas ELF son renombradas por debian.bbclass según su SONAME
# (audio -> libaudio1, pwm -> libpwm1, etc.). El packagegroup no puede ser
# allarch porque esa resolución depende de la arquitectura de destino.
PACKAGE_ARCH = "${TUNE_PKGARCH}"

RDEPENDS:${PN} = "libgpio pwm audio leds sensors aurabot-api"
