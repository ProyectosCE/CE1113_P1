#!/bin/bash

POKY_DIR="$HOME/poky-scarthgap-5.0.19"
PROJECT_DIR="$HOME/CE1113_P1"
TARGET_DEV="/dev/mmcblk0"

if ! command -v whiptail &> /dev/null; then
    echo "Error: whiptail no está instalado."
    exit 1
fi

# --- Verificación: ¿la SD está realmente conectada? ---
if [ ! -b "$TARGET_DEV" ]; then
    # Intentar forzar a udev a recrearlo antes de rendirse
    sudo udevadm trigger --subsystem-match=block --action=add 2>/dev/null
    sudo udevadm settle 2>/dev/null
fi

if [ ! -b "$TARGET_DEV" ]; then
    whiptail --title "SD no detectada" \
        --msgbox "Error: $TARGET_DEV no existe o no es un dispositivo de bloque.\n\nVerifica que la tarjeta SD esté conectada e inténtalo de nuevo.\n\nPuedes confirmar el nombre correcto del dispositivo con: lsblk" \
        12 75
    exit 1
fi

MACHINE_SEL=$(whiptail \
    --title "Selección de Máquina" \
    --menu "Selecciona la máquina:" \
    15 65 2 \
    "qemuarm64" "Emulador QEMU ARM64" \
    "raspberrypi4" "Raspberry Pi 4" \
    3>&1 1>&2 2>&3)

[ $? -ne 0 ] && exit 1

if [ "$MACHINE_SEL" == "qemuarm64" ]; then
    whiptail --msgbox \
        "Error: Las imágenes de QEMU no se graban en SD. Selecciona raspberrypi4." \
        10 65
    exit 1
fi

IMAGE_SEL=$(whiptail \
    --title "Selección de Imagen" \
    --menu "Selecciona la imagen a grabar:" \
    16 65 3 \
    "CE1113-P1" "Imagen final personalizada" \
    "core-image-minimal" "Imagen base limpia de Poky" \
    "rpi-test-image" "Imagen de prueba" \
    3>&1 1>&2 2>&3)

[ $? -ne 0 ] && exit 1

clear
echo "Buscando el sistema de archivos más reciente..."

LATEST_MANIFEST=$(find "$POKY_DIR"/build*/tmp/deploy/images \
    -type f \
    -name "${IMAGE_SEL}*.manifest" \
    -printf '%T@ %p\n' \
    2>/dev/null |
    grep "$MACHINE_SEL" |
    sort -n |
    tail -1 |
    awk '{print $2}')

if [ -z "$LATEST_MANIFEST" ]; then
    LATEST_MANIFEST=$(find "$POKY_DIR"/build*/tmp/deploy/images \
        -type f \
        -name "${IMAGE_SEL}*.manifest" \
        -printf '%T@ %p\n' \
        2>/dev/null |
        sort -n |
        tail -1 |
        awk '{print $2}')
fi

if [ -z "$LATEST_MANIFEST" ]; then
    whiptail --msgbox \
        "Error: No se encontró ningún archivo .manifest para ${IMAGE_SEL} (${MACHINE_SEL})." \
        12 75
    exit 1
fi

DEPLOY_DIR=$(dirname "$LATEST_MANIFEST")
IMAGE_BASE=$(basename "$LATEST_MANIFEST" .manifest)

# Preferimos .wic.bz2 / .wic: son imágenes de disco completas
# (boot + rootfs), listas para grabar tal cual en la SD.
# Un .ext4 suelto NO tiene partición de boot y la Raspberry Pi
# no podrá arrancar con él.

IMAGE_TYPE=""
LATEST_TRV=""

if [ -f "${DEPLOY_DIR}/${IMAGE_BASE}.wic.bz2" ]; then
    LATEST_TRV="${DEPLOY_DIR}/${IMAGE_BASE}.wic.bz2"
    IMAGE_TYPE="wic.bz2"

elif [ -f "${DEPLOY_DIR}/${IMAGE_BASE}.wic" ]; then
    LATEST_TRV="${DEPLOY_DIR}/${IMAGE_BASE}.wic"
    IMAGE_TYPE="wic"

elif [ -f "${DEPLOY_DIR}/${IMAGE_BASE}.ext4" ]; then
    LATEST_TRV="${DEPLOY_DIR}/${IMAGE_BASE}.ext4"
    IMAGE_TYPE="ext4"
fi

if [ -z "$LATEST_TRV" ]; then
    whiptail --msgbox \
        "Error: Se encontró el manifest (${IMAGE_BASE}) pero no su imagen (.wic.bz2/.wic/.ext4) en:\n${DEPLOY_DIR}" \
        12 75
    exit 1
fi

if [ "$IMAGE_TYPE" == "ext4" ]; then
    whiptail --msgbox \
        "Advertencia: solo se encontró un .ext4 (sin partición de boot). La Raspberry Pi NO arrancará con esto.\n\nAgrega 'wic.bz2' a IMAGE_FSTYPES en tu configuración y recompila para generar una imagen completa." \
        14 75
    exit 1
fi

IMAGE_NAME=$(basename "$LATEST_TRV")

# --- Verificación 1: ¿el build es más reciente que los últimos cambios en tus recetas? ---
# Si algún .bb fue modificado DESPUÉS de que se generó este manifest,
# la imagen que vas a grabar no refleja ese cambio.

STALE_WARNING=""

if [ -d "$PROJECT_DIR" ]; then
    NEWER_RECIPES=$(find "$PROJECT_DIR" \
        -name "*.bb" \
        -newer "$LATEST_MANIFEST" \
        2>/dev/null)

    if [ -n "$NEWER_RECIPES" ]; then
        STALE_WARNING="⚠ ADVERTENCIA: estas recetas fueron modificadas DESPUÉS de esta compilación (recompila antes de grabar):\n"

        STALE_WARNING+=$(echo "$NEWER_RECIPES" |
            xargs -n1 basename |
            sed 's/^/  - /' |
            tr '\n' '|')

        STALE_WARNING=$(echo "$STALE_WARNING" | tr '|' '\n')
    fi
fi

# --- Verificación 2: ¿los paquetes custom del proyecto están en el manifest? ---

CUSTOM_REPORT=""
MISSING_COUNT=0

if [ -d "$PROJECT_DIR" ]; then

    CUSTOM_PKGS=$(find "$PROJECT_DIR" \
        -name "*.bb" \
        2>/dev/null |
        grep -v "/recipes-core/images/" |
        awk -F/ '{print $NF}' |
        cut -d'_' -f1 |
        cut -d'.' -f1 |
        sort -u)

    MISSING_COUNT=0

    for pkg in $CUSTOM_PKGS; do

        if awk '{print $1}' "$LATEST_MANIFEST" |
            grep -q "^${pkg}"; then

            CUSTOM_REPORT+="  [OK] $pkg\n"

        else

            CUSTOM_REPORT+="  [X]  $pkg  <-- NO está en la imagen\n"
            MISSING_COUNT=$((MISSING_COUNT + 1))

        fi

    done
fi

# Si hay recetas más nuevas que el build, o paquetes custom faltantes,
# avisamos y pedimos confirmación explícita antes de seguir.

if [ -n "$STALE_WARNING" ] || [ "$MISSING_COUNT" -gt 0 ]; then

    ALERT_MSG="$STALE_WARNING\n"

    if [ "$MISSING_COUNT" -gt 0 ]; then
        ALERT_MSG+="⚠ $MISSING_COUNT paquete(s) custom NO están en esta imagen:\n$CUSTOM_REPORT\n"
    fi

    ALERT_MSG+="\n¿Deseas grabar esta imagen de todas formas?"

    whiptail \
        --title "Verificación de vigencia" \
        --yesno "$ALERT_MSG" \
        22 78

    if [ $? -ne 0 ]; then
        echo "Operación cancelada: recompila la imagen y vuelve a intentar."
        exit 0
    fi

else

    whiptail \
        --title "Verificación OK" \
        --msgbox "✓ La imagen está al día con tus recetas.\n✓ Todos los paquetes custom están presentes:\n\n$CUSTOM_REPORT" \
        20 78
fi

whiptail \
    --title "¡ADVERTENCIA DE SEGURIDAD!" \
    --yesno "Se encontró la imagen rootfs:\n\n${IMAGE_NAME}\n\nSe grabará en la tarjeta SD.\n\n${TARGET_DEV} - ESTO BORRARÁ TODOS LOS DATOS EN LA SD\n\n¿Estás completamente seguro de continuar?" \
    16 70

if [ $? -ne 0 ]; then
    echo "Operación cancelada por el usuario."
    exit 0
fi

clear

echo "================================================="
echo " INICIANDO GRABACIÓN EN $TARGET_DEV"
echo "================================================="

# --- Verificación (otra vez): la SD sigue conectada justo antes de escribir ---

if [ ! -b "$TARGET_DEV" ]; then
    # Intento de recuperación de udev justo antes de escribir
    sudo udevadm trigger --subsystem-match=block --action=add 2>/dev/null
    sudo udevadm settle 2>/dev/null
fi

if [ ! -b "$TARGET_DEV" ]; then
    whiptail \
        --title "SD no detectada" \
        --msgbox "Error: $TARGET_DEV ya no está disponible. ¿Se desconectó la SD?" \
        10 70
    exit 1
fi

echo "Desmontando particiones previas si están activas..."

sudo umount "${TARGET_DEV}"* 2>/dev/null

echo "Limpiando tabla de particiones y firmas de filesystem anteriores..."

if command -v wipefs &> /dev/null; then
    sudo wipefs -a "$TARGET_DEV" 2>/dev/null
fi

# Borra los primeros 10 MB (MBR/GPT primario) por si wipefs
# no está disponible o no alcanzó a limpiar todo.

sudo dd \
    if=/dev/zero \
    of="$TARGET_DEV" \
    bs=1M \
    count=10 \
    status=none \
    2>/dev/null

sudo sync

echo "Escribiendo la imagen sobre la SD (esto puede tardar)..."

if [ "$IMAGE_TYPE" == "wic.bz2" ]; then

    bzcat "$LATEST_TRV" |
        sudo dd \
            of="$TARGET_DEV" \
            bs=4M \
            status=progress

else

    sudo dd \
        if="$LATEST_TRV" \
        of="$TARGET_DEV" \
        bs=4M \
        status=progress

fi

sudo sync

echo "-------------------------------------------------"
echo " ¡LISTO! La memoria SD fue preparada."
echo " Puedes retirarla y ponerla en tu Raspberry Pi 4."
echo "================================================="