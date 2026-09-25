SUMMARY = "Imagen base minimalista para el proyecto CE1113"
LICENSE = "MIT"

# Heredar la imagen minimalista oficial de Poky
require recipes-core/images/core-image-minimal.bb

# Sin paquetes de idiomas innecesarios
IMAGE_LINGUAS = ""

# Generar:
# - ext4: filesystem raíz
# - tar.bz2: respaldo/despliegue manual
# - wic.bz2: imagen completa arrancable para Raspberry Pi
IMAGE_FSTYPES = "ext4 tar.bz2 wic.bz2"