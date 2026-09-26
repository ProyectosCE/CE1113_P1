#!/usr/bin/env bash
set -euo pipefail

POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}
MACHINE=raspberrypi4; IMAGE=ce1113-p1; BUILD_DIR=""; TARGET_DEVICE=""; ASSUME_YES=0; FORCE=0

usage() {
    cat <<'EOF'
Uso: ./flash_sd.sh --device /dev/sdX [opciones]
  -d, --device DEV     Dispositivo completo, nunca una partición
  -i, --image NOMBRE   Por defecto: ce1113-p1
  -p, --poky DIR       Árbol Poky (o variable POKY_DIR)
  -b, --build-dir DIR  Por defecto: <poky>/build-raspberrypi4
  -y, --yes            Omite confirmación interactiva
      --force          Permite dispositivo no marcado como removible
EOF
}
while (($#)); do
    case "$1" in
        -d|--device) TARGET_DEVICE=${2:?}; shift 2;; -i|--image) IMAGE=${2:?}; shift 2;;
        -p|--poky) POKY_DIR=${2:?}; shift 2;; -b|--build-dir) BUILD_DIR=${2:?}; shift 2;;
        -y|--yes) ASSUME_YES=1; shift;; --force) FORCE=1; shift;; -h|--help) usage; exit 0;;
        *) echo "Opción desconocida: $1" >&2; exit 2;;
    esac
done
[[ -n "$TARGET_DEVICE" ]] || { usage >&2; echo "Error: --device es obligatorio." >&2; exit 2; }
[[ -b "$TARGET_DEVICE" ]] || { echo "$TARGET_DEVICE no es un dispositivo de bloque." >&2; exit 1; }
[[ "$(lsblk -dn -o TYPE "$TARGET_DEVICE")" == disk ]] || { echo "Use el disco completo, no una partición: $TARGET_DEVICE" >&2; exit 1; }
if ((FORCE == 0)) && [[ "$(lsblk -dn -o RM "$TARGET_DEVICE")" != 1 ]]; then
    echo "$TARGET_DEVICE no está marcado como removible; use --force solo tras verificarlo con lsblk." >&2; exit 1
fi

BUILD_DIR=${BUILD_DIR:-"$POKY_DIR/build-$MACHINE"}; DEPLOY_DIR="$BUILD_DIR/tmp/deploy/images/$MACHINE"
LATEST_MANIFEST=$(find "$DEPLOY_DIR" -maxdepth 1 -type f -name "${IMAGE}-${MACHINE}*.manifest" -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n1 | cut -d' ' -f2-)
[[ -n "$LATEST_MANIFEST" ]] || { echo "No existe una compilación de $IMAGE para $MACHINE." >&2; exit 1; }
IMAGE_BASE=${LATEST_MANIFEST%.manifest}; IMAGE_FILE=""; IMAGE_TYPE=""
for extension in wic.bz2 wic; do
    if [[ -f "$IMAGE_BASE.$extension" ]]; then IMAGE_FILE="$IMAGE_BASE.$extension"; IMAGE_TYPE=$extension; break; fi
done
[[ -n "$IMAGE_FILE" ]] || { echo "Falta .wic.bz2 o .wic junto a $LATEST_MANIFEST." >&2; exit 1; }

if find "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)" -type f \( -name '*.bb' -o -name '*.bbappend' \) -newer "$LATEST_MANIFEST" -print -quit | grep -q .; then
    echo "Advertencia: hay metadata Yocto más nueva que la imagen." >&2
fi
echo "Se sobrescribirá TODO $TARGET_DEVICE con $(basename "$IMAGE_FILE")."
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINTS "$TARGET_DEVICE"
if ((ASSUME_YES == 0)); then
    read -r -p "Escriba exactamente BORRAR para continuar: " answer
    [[ "$answer" == BORRAR ]] || { echo "Cancelado."; exit 0; }
fi

mapfile -t partitions < <(lsblk -ln -o PATH "$TARGET_DEVICE" | tail -n +2)
for partition in "${partitions[@]}"; do sudo umount "$partition" 2>/dev/null || true; done
if [[ "$IMAGE_TYPE" == wic.bz2 ]]; then bzcat "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync; else sudo dd if="$IMAGE_FILE" of="$TARGET_DEVICE" bs=4M status=progress conv=fsync; fi
sync
echo "Imagen grabada correctamente en $TARGET_DEVICE."
