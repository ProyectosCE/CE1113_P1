#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}
BUILD_DIR=""; TARGET_DEVICE=""; ASSUME_YES=0
# shellcheck source=scripts/lib/image-common.sh
source "$PROJECT_DIR/scripts/lib/image-common.sh"

usage() {
    cat <<'EOF'
Uso: ./flash_sd.sh [--build-dir DIR] [--device /dev/sdX] [--yes]

Verifica ce1113-p1 y graba su WIC en una SD removible. Sin --device usa GUI.
--yes omite la confirmación, pero nunca las comprobaciones de seguridad.
EOF
}

while (($#)); do
    case "$1" in
        -b|--build-dir) BUILD_DIR=${2:?falta el directorio}; shift 2 ;;
        -d|--device) TARGET_DEVICE=${2:?falta el dispositivo}; shift 2 ;;
        -y|--yes) ASSUME_YES=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) printf 'Opción desconocida: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

for tool in lsblk findmnt wipefs dd sync sha256sum; do
    command -v "$tool" >/dev/null || { echo "Error: falta $tool." >&2; exit 1; }
done

BUILD_DIR=${BUILD_DIR:-"$POKY_DIR/build-$CE1113_MACHINE"}
# La misma validación que sigue al build debe pasar inmediatamente antes del borrado.
"$PROJECT_DIR/check_image.sh" --no-ui --build-dir "$BUILD_DIR"

DEPLOY_DIR=$(ce1113_deploy_dir "$BUILD_DIR")
MANIFEST=$(ce1113_latest_manifest "$DEPLOY_DIR")
IMAGE_FILE=$(ce1113_image_for_manifest "$MANIFEST")
EXPECTED_HASH=$(sha256sum "$IMAGE_FILE" | awk '{print $1}')

ROOT_SOURCE=$(findmnt -n -o SOURCE / 2>/dev/null || true)
ROOT_PARENT=""
if [[ -n "$ROOT_SOURCE" ]]; then
    ROOT_PARENT=$(lsblk -ndo PKNAME "$ROOT_SOURCE" 2>/dev/null | head -n1 || true)
    [[ -n "$ROOT_PARENT" ]] && ROOT_PARENT="/dev/$ROOT_PARENT"
    [[ -z "$ROOT_PARENT" && -b "$ROOT_SOURCE" ]] && ROOT_PARENT=$ROOT_SOURCE
fi

is_safe_removable_disk() {
    local device=$1 type removable transport
    [[ -b "$device" ]] || return 1
    type=$(lsblk -dn -o TYPE "$device")
    removable=$(lsblk -dn -o RM "$device")
    transport=$(lsblk -dn -o TRAN "$device")
    [[ "$type" == disk && "$device" != "$ROOT_PARENT" ]] || return 1
    [[ "$removable" == 1 || "$transport" == usb || "$transport" == mmc ]]
}

if [[ -z "$TARGET_DEVICE" ]]; then
    command -v whiptail >/dev/null || {
        echo 'Error: sin --device se requiere whiptail.' >&2; exit 1;
    }
    DEVICE_OPTIONS=()
    while IFS= read -r device; do
        if is_safe_removable_disk "$device"; then
            description=$(lsblk -dn -o SIZE,MODEL,TRAN "$device" | sed 's/[[:space:]]*$//')
            DEVICE_OPTIONS+=("$device" "$description")
        fi
    done < <(lsblk -dnpo NAME)
    ((${#DEVICE_OPTIONS[@]} > 0)) || {
        whiptail --title 'SD no detectada' --msgbox \
            'No se detectó una SD, unidad USB o dispositivo MMC removible.' 10 72
        exit 1
    }
    TARGET_DEVICE=$(whiptail --title 'Grabar CE1113 en SD' --menu \
        'Seleccione el disco completo que será borrado:' 18 84 8 \
        "${DEVICE_OPTIONS[@]}" 3>&1 1>&2 2>&3) || exit 0
fi

is_safe_removable_disk "$TARGET_DEVICE" || {
    echo "Error: $TARGET_DEVICE no es un disco removible seguro o contiene /." >&2; exit 1;
}
DEVICE_INFO=$(lsblk -dn -o NAME,SIZE,MODEL,TRAN,RM "$TARGET_DEVICE")

if ((ASSUME_YES == 0)); then
    message="Dispositivo: $DEVICE_INFO
Imagen: $(basename "$IMAGE_FILE")
SHA-256: $EXPECTED_HASH

TODOS LOS DATOS DE $TARGET_DEVICE SERÁN BORRADOS."
    if [[ -t 0 ]] && command -v whiptail >/dev/null; then
        whiptail --title 'BORRADO TOTAL DE LA SD' --yesno "$message" 17 88 || exit 0
    else
        printf '%s\nEscriba exactamente BORRAR para continuar: ' "$message" >&2
        read -r confirmation
        [[ "$confirmation" == BORRAR ]] || exit 0
    fi
fi

sudo -v
is_safe_removable_disk "$TARGET_DEVICE" || {
    echo 'Error: el dispositivo cambió o fue desconectado.' >&2; exit 1;
}
mapfile -t PARTITIONS < <(lsblk -lnpo PATH "$TARGET_DEVICE" | tail -n +2)
for partition in "${PARTITIONS[@]}"; do sudo umount "$partition" 2>/dev/null || true; done
sudo umount "$TARGET_DEVICE" 2>/dev/null || true
sudo wipefs -a "$TARGET_DEVICE"
sudo dd if=/dev/zero of="$TARGET_DEVICE" bs=1M count=10 status=none conv=fsync

echo "Grabando $(basename "$IMAGE_FILE") en $TARGET_DEVICE..."
case "$IMAGE_FILE" in
    *.wic.bz2) bzip2 -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    *.wic.gz)  gzip -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    *.wic.xz)  xz -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    *.wic)     sudo dd if="$IMAGE_FILE" of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
esac
sudo sync

message="Grabación completada en $TARGET_DEVICE.
Origen verificado: $EXPECTED_HASH
Puede retirar la tarjeta de forma segura."
if [[ -t 0 ]] && command -v whiptail >/dev/null; then
    whiptail --title 'Grabación completada' --msgbox "$message" 11 80
else
    printf '%s\n' "$message"
fi
