#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}

for tool in whiptail lsblk findmnt wipefs dd sync; do
    command -v "$tool" >/dev/null || { echo "Error: falta la herramienta $tool." >&2; exit 1; }
done

MAIN_IMAGE=$(find "$PROJECT_DIR/meta-ce1113/recipes-core/images" -maxdepth 1 -type f -name '*.bb' -printf '%f\n' | sort | head -n1)
[[ -n "$MAIN_IMAGE" ]] || { echo "Error: no se encontró la imagen de meta-ce1113." >&2; exit 1; }
MAIN_IMAGE=${MAIN_IMAGE%.bb}

MACHINE=$(whiptail --title "Grabar tarjeta SD" --menu \
    "Seleccione el target:" 15 70 2 \
    raspberrypi4 "Raspberry Pi 4 (grabable en SD)" \
    qemuarm64 "QEMU ARM64 (no grabable en SD)" 3>&1 1>&2 2>&3) || exit 0
if [[ "$MACHINE" != raspberrypi4 ]]; then
    whiptail --title "Target no grabable" --msgbox \
        "Las imágenes QEMU no se graban en una tarjeta SD. Seleccione raspberrypi4." 10 72
    exit 1
fi

IMAGE=$(whiptail --title "Grabar tarjeta SD" --menu \
    "Seleccione la imagen:" 16 75 3 \
    "$MAIN_IMAGE" "Imagen principal CE1113" \
    core-image-minimal "Imagen mínima de Poky" \
    rpi-test-image "Imagen de pruebas de Raspberry Pi" 3>&1 1>&2 2>&3) || exit 0

BUILD_DIR="$POKY_DIR/build-$MACHINE"
DEPLOY_DIR="$BUILD_DIR/tmp/deploy/images/$MACHINE"
LATEST_MANIFEST=$(find -L "$DEPLOY_DIR" -maxdepth 1 -type f -name "${IMAGE}-${MACHINE}*.manifest" \
    -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n1 | cut -d' ' -f2-)
if [[ -z "$LATEST_MANIFEST" ]]; then
    whiptail --title "Imagen no encontrada" --msgbox \
        "No existe una compilación de $IMAGE para $MACHINE en:\n$DEPLOY_DIR" 12 76
    exit 1
fi

IMAGE_BASE=${LATEST_MANIFEST%.manifest}
IMAGE_FILE=""; IMAGE_TYPE=""
for extension in wic.bz2 wic.gz wic.xz wic; do
    if [[ -f "$IMAGE_BASE.$extension" ]]; then
        IMAGE_FILE="$IMAGE_BASE.$extension"; IMAGE_TYPE=$extension; break
    fi
done
if [[ -z "$IMAGE_FILE" ]]; then
    whiptail --title "Imagen de disco ausente" --msgbox \
        "Existe el manifest, pero falta un artefacto .wic comprimido o sin comprimir:\n$IMAGE_BASE" 12 78
    exit 1
fi

# Verificar que el archivo puede leerse/descomprimirse antes de tocar la tarjeta.
case "$IMAGE_TYPE" in
    wic.bz2) command -v bzip2 >/dev/null || { whiptail --msgbox "Falta bzip2." 8 40; exit 1; }; bzip2 -t "$IMAGE_FILE" ;;
    wic.gz) command -v gzip >/dev/null || { whiptail --msgbox "Falta gzip." 8 40; exit 1; }; gzip -t "$IMAGE_FILE" ;;
    wic.xz) command -v xz >/dev/null || { whiptail --msgbox "Falta xz." 8 40; exit 1; }; xz -t "$IMAGE_FILE" ;;
    wic) dd if="$IMAGE_FILE" of=/dev/null bs=1M count=1 status=none ;;
esac

ROOT_SOURCE=$(findmnt -n -o SOURCE / 2>/dev/null || true)
ROOT_PARENT=""
if [[ -n "$ROOT_SOURCE" ]]; then
    ROOT_PARENT=$(lsblk -ndo PKNAME "$ROOT_SOURCE" 2>/dev/null | head -n1 || true)
    [[ -n "$ROOT_PARENT" ]] && ROOT_PARENT="/dev/$ROOT_PARENT"
    [[ -z "$ROOT_PARENT" && -b "$ROOT_SOURCE" ]] && ROOT_PARENT="$ROOT_SOURCE"
fi

DEVICE_OPTIONS=()
while IFS= read -r name; do
    [[ -b "$name" ]] || continue
    type=$(lsblk -dn -o TYPE "$name")
    removable=$(lsblk -dn -o RM "$name")
    transport=$(lsblk -dn -o TRAN "$name")
    size=$(lsblk -dn -o SIZE "$name")
    model=$(lsblk -dn -o MODEL "$name" | sed 's/[[:space:]]*$//')
    [[ "$type" == disk ]] || continue
    [[ "$name" != "$ROOT_PARENT" ]] || continue
    if [[ "$removable" == 1 || "$transport" == usb || "$transport" == mmc ]]; then
        DEVICE_OPTIONS+=("$name" "$size ${model:-sin-modelo} [${transport:-removible}]")
    fi
done < <(lsblk -dnpo NAME)

if ((${#DEVICE_OPTIONS[@]} == 0)); then
    whiptail --title "SD no detectada" --msgbox \
        "No se detectó ningún disco removible, USB o MMC.\n\nConecte la SD, espere unos segundos y vuelva a ejecutar el script.\nPuede comprobarla con: lsblk" 13 76
    exit 1
fi

TARGET_DEVICE=$(whiptail --title "Seleccionar tarjeta SD" --menu \
    "Seleccione cuidadosamente el dispositivo que se borrará:" 18 82 8 \
    "${DEVICE_OPTIONS[@]}" 3>&1 1>&2 2>&3) || exit 0

[[ -b "$TARGET_DEVICE" ]] || { whiptail --msgbox "$TARGET_DEVICE ya no está conectado." 9 60; exit 1; }
[[ "$(lsblk -dn -o TYPE "$TARGET_DEVICE")" == disk ]] || { whiptail --msgbox "El destino no es un disco completo." 9 60; exit 1; }
[[ "$TARGET_DEVICE" != "$ROOT_PARENT" ]] || { whiptail --msgbox "Se bloqueó el intento de borrar el disco del sistema." 9 68; exit 1; }

DEVICE_INFO=$(lsblk -dn -o NAME,SIZE,MODEL,TRAN "$TARGET_DEVICE")
STALE=""
if find "$PROJECT_DIR/meta-ce1113" "$PROJECT_DIR/layers" -type f \( -name '*.bb' -o -name '*.bbappend' \) \
    -newer "$LATEST_MANIFEST" -print -quit | grep -q .; then
    STALE="\n\nADVERTENCIA: hay recetas más nuevas que esta imagen."
fi

whiptail --title "BORRADO TOTAL DE LA SD" --yesno \
    "Dispositivo: $DEVICE_INFO\nImagen: $(basename "$IMAGE_FILE")$STALE\n\nTODOS LOS DATOS DE $TARGET_DEVICE SERÁN BORRADOS.\n\n¿Desea continuar?" 18 82 || exit 0

sudo -v
[[ -b "$TARGET_DEVICE" ]] || { whiptail --msgbox "La SD fue desconectada." 9 55; exit 1; }

mapfile -t PARTITIONS < <(lsblk -lnpo PATH "$TARGET_DEVICE" | tail -n +2)
for partition in "${PARTITIONS[@]}"; do sudo umount "$partition" 2>/dev/null || true; done
sudo umount "$TARGET_DEVICE" 2>/dev/null || true

sudo wipefs -a "$TARGET_DEVICE"
sudo dd if=/dev/zero of="$TARGET_DEVICE" bs=1M count=10 status=none conv=fsync

clear
echo "Grabando $(basename "$IMAGE_FILE") en $TARGET_DEVICE..."
case "$IMAGE_TYPE" in
    wic.bz2) bzip2 -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    wic.gz) gzip -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    wic.xz) xz -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    wic) sudo dd if="$IMAGE_FILE" of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
esac
sudo sync

whiptail --title "Grabación completada" --msgbox \
    "La imagen se grabó correctamente en $TARGET_DEVICE.\n\nPuede retirar la tarjeta SD de forma segura." 11 70
