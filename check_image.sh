#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}

command -v whiptail >/dev/null || { echo "Error: instale whiptail." >&2; exit 1; }

MAIN_IMAGE=$(find "$PROJECT_DIR/meta-ce1113/recipes-core/images" -maxdepth 1 -type f -name '*.bb' -printf '%f\n' | sort | head -n1)
[[ -n "$MAIN_IMAGE" ]] || { echo "Error: no se encontró la imagen de meta-ce1113." >&2; exit 1; }
MAIN_IMAGE=${MAIN_IMAGE%.bb}

MACHINE=$(whiptail --title "Verificación de imagen" --menu \
    "Seleccione el target compilado:" 15 70 2 \
    qemuarm64 "Emulador QEMU ARM64" \
    raspberrypi4 "Raspberry Pi 4" 3>&1 1>&2 2>&3) || exit 0

IMAGE_OPTIONS=("$MAIN_IMAGE" "Imagen principal CE1113" core-image-minimal "Imagen mínima de Poky")
if [[ "$MACHINE" == raspberrypi4 ]]; then
    IMAGE_OPTIONS+=(rpi-test-image "Imagen de pruebas de Raspberry Pi")
fi
IMAGE=$(whiptail --title "Verificación de imagen" --menu \
    "Seleccione la imagen:" 16 75 3 "${IMAGE_OPTIONS[@]}" 3>&1 1>&2 2>&3) || exit 0

BUILD_DIR="$POKY_DIR/build-$MACHINE"
DEPLOY_DIR="$BUILD_DIR/tmp/deploy/images/$MACHINE"
if [[ ! -d "$DEPLOY_DIR" ]]; then
    whiptail --title "Build no encontrado" --msgbox \
        "No existe:\n$DEPLOY_DIR\n\nCompile primero $IMAGE para $MACHINE." 12 76
    exit 1
fi

LATEST_MANIFEST=$(find -L "$DEPLOY_DIR" -maxdepth 1 -type f -name "${IMAGE}-${MACHINE}*.manifest" \
    -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n1 | cut -d' ' -f2-)
if [[ -z "$LATEST_MANIFEST" ]]; then
    whiptail --title "Manifest no encontrado" --msgbox \
        "No se encontró un manifest de $IMAGE para $MACHINE en:\n$DEPLOY_DIR" 11 76
    exit 1
fi

REPORT=$(mktemp /tmp/ce1113-image-report.XXXXXX)
trap 'rm -f "$REPORT"' EXIT
{
    echo "IMAGEN: $IMAGE"
    echo "TARGET: $MACHINE"
    echo "MANIFEST: $LATEST_MANIFEST"
    echo "PAQUETES: $(wc -l < "$LATEST_MANIFEST")"
    echo
    echo "ARTEFACTOS"
    manifest_base=$(basename "$LATEST_MANIFEST" .manifest)
    find -L "$DEPLOY_DIR" -maxdepth 1 -type f -name "$manifest_base.*" \
        -printf '  %10s bytes  %f\n' 2>/dev/null | sort || true
    echo
    echo "PAQUETES DE LAS RECETAS DEL PROYECTO"
    while IFS= read -r recipe; do
        package=$(basename "$recipe"); package=${package%%_*}; package=${package%.bb}
        if awk -v pkg="$package" '$1 == pkg { found=1 } END { exit !found }' "$LATEST_MANIFEST"; then
            printf '  [OK] %s\n' "$package"
        else
            printf '  [--] %s (capa no seleccionada o paquete ausente)\n' "$package"
        fi
    done < <(find "$PROJECT_DIR/layers" -type f -name '*.bb' ! -path '*/recipes-core/images/*' | sort)
} > "$REPORT"

LINES=$(wc -l < "$REPORT"); HEIGHT=$((LINES + 6)); ((HEIGHT > 34)) && HEIGHT=34; ((HEIGHT < 16)) && HEIGHT=16
whiptail --title "Resultado: $IMAGE ($MACHINE)" --scrolltext --textbox "$REPORT" "$HEIGHT" 90
