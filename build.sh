#!/bin/bash

# ==========================================
# 1. Definición de Rutas Absolutas
# ==========================================
POKY_DIR="$HOME/poky-scarthgap-5.0.19"
PROJECT_DIR="$HOME/CE1113_P1"
CUSTOM_LAYERS_DIR="$PROJECT_DIR/layers"
META_BASE="$PROJECT_DIR/meta-ce1113"

if ! command -v whiptail &> /dev/null; then
    echo "Error: whiptail no está instalado. Instálalo con 'sudo apt-get install whiptail'."
    exit 1
fi

# ==========================================
# 2. Menú de Selección de Máquina
# ==========================================
MACHINE_SEL=$(whiptail --title "Selección de Arquitectura" --radiolist \
"Selecciona la máquina destino (Espacio para marcar, Enter para aceptar):" 15 65 2 \
"qemuarm64" "Emulador QEMU ARM64 (Soporte nativo)" ON \
"raspberrypi4" "Raspberry Pi 4 (Requiere meta-raspberrypi)" OFF 3>&1 1>&2 2>&3)

if [ $? -ne 0 ]; then
    echo "Operación cancelada por el usuario."
    exit 1
fi

# ==========================================
# 3. Menú de Selección de Imagen
# ==========================================
IMAGE_SEL=$(whiptail --title "Selección de Imagen" --radiolist \
"Selecciona la imagen a compilar (Espacio para marcar, Enter para aceptar):" 16 75 3 \
"CE1113-P1" "Imagen final personalizada con tus módulos" ON \
"core-image-minimal" "Imagen base limpia de Poky (Para pruebas)" OFF \
"rpi-test-image" "Imagen de pruebas específica para Raspberry Pi" OFF 3>&1 1>&2 2>&3)

if [ $? -ne 0 ]; then
    echo "Operación cancelada por el usuario."
    exit 1
fi

# ==========================================
# 4. Menú Dinámico de Selección de Capas Custom
# ==========================================
OPTIONS=()
if [ -d "$CUSTOM_LAYERS_DIR" ]; then
    for layer in "$CUSTOM_LAYERS_DIR"/meta-*; do
        if [ -d "$layer" ]; then
            layer_name=$(basename "$layer")
            OPTIONS+=("$layer_name" "Capa personalizada" "off")
        fi
    done
fi

LAYERS_CLEAN=""
if [ ${#OPTIONS[@]} -gt 0 ]; then
    LAYERS_SEL=$(whiptail --title "Selección de Capas CE1113" --checklist \
    "Selecciona las capas a incluir (Espacio para marcar, Enter para aceptar):" 20 70 10 \
    "${OPTIONS[@]}" 3>&1 1>&2 2>&3)

    if [ $? -ne 0 ]; then
        echo "Operación cancelada por el usuario."
        exit 1
    fi
    LAYERS_CLEAN=$(echo "$LAYERS_SEL" | tr -d '"')
else
    whiptail --msgbox "No se encontraron capas en $CUSTOM_LAYERS_DIR. Se usarán solo las base." 10 60
fi

# ==========================================
# 5. Inicializar el Entorno de Yocto
# ==========================================
BUILD_DIR_NAME="build-$MACHINE_SEL"
echo "Inicializando entorno de Yocto en $POKY_DIR/$BUILD_DIR_NAME..."

cd "$POKY_DIR"
source oe-init-build-env "$BUILD_DIR_NAME"

# ==========================================
# 6. Configurar variables (local.conf)
# ==========================================
echo "Configurando la máquina a $MACHINE_SEL en local.conf..."
sed -i "s/^MACHINE ??=.*/MACHINE ??= \"$MACHINE_SEL\"/" conf/local.conf

if ! grep -q "^MACHINE ??= \"$MACHINE_SEL\"" conf/local.conf; then
    echo "MACHINE ??= \"$MACHINE_SEL\"" >> conf/local.conf
fi

# Aceptar licencias restrictivas necesarias para el firmware de la Raspberry Pi
if ! grep -q "synaptics-killswitch" conf/local.conf; then
    echo 'LICENSE_FLAGS_ACCEPTED += "synaptics-killswitch"' >> conf/local.conf
    echo "Aceptada licencia synaptics-killswitch en local.conf"
fi

# ==========================================
# 7. Agregar Capas al bblayers.conf
# ==========================================
echo "Agregando capa base meta-ce1113..."
bitbake-layers add-layer "$META_BASE" > /dev/null 2>&1

if [ "$MACHINE_SEL" == "raspberrypi4" ]; then
    if [ -d "$POKY_DIR/meta-raspberrypi" ]; then
        echo "Agregando dependencias de Raspberry Pi..."
        bitbake-layers add-layer "$POKY_DIR/meta-raspberrypi" > /dev/null 2>&1
    else
        echo "ADVERTENCIA: Seleccionaste RPi4 pero no se encontró $POKY_DIR/meta-raspberrypi"
    fi
fi

for layer in $LAYERS_CLEAN; do
    echo "Agregando capa custom: $layer"
    bitbake-layers add-layer "$CUSTOM_LAYERS_DIR/$layer" > /dev/null 2>&1
done

# ==========================================
# 8. Ejecutar Compilación
# ==========================================
echo "-----------------------------------------------------------"
echo "Iniciando compilación de $IMAGE_SEL para $MACHINE_SEL..."
echo "-----------------------------------------------------------"
bitbake "$IMAGE_SEL"
