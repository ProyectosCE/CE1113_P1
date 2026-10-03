#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}
BUILD_DIR=""; NO_UI=0; REPORT_FILE=""; TEMP_REPORT=0
# shellcheck source=scripts/lib/image-common.sh
source "$PROJECT_DIR/scripts/lib/image-common.sh"

usage() {
    cat <<'EOF'
Uso: ./check_image.sh [--no-ui] [--build-dir DIR] [--report FILE]

Verifica ce1113-p1 para raspberrypi4: manifest, paquetes obligatorios,
compresión, SHA-256 y si la imagen está desactualizada.
EOF
}

while (($#)); do
    case "$1" in
        --no-ui) NO_UI=1; shift ;;
        -b|--build-dir) BUILD_DIR=${2:?falta el directorio}; shift 2 ;;
        --report) REPORT_FILE=${2:?falta el archivo}; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) printf 'Opción desconocida: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

BUILD_DIR=${BUILD_DIR:-"$POKY_DIR/build-$CE1113_MACHINE"}
DEPLOY_DIR=$(ce1113_deploy_dir "$BUILD_DIR")
[[ -d "$DEPLOY_DIR" ]] || { echo "ERROR: no existe $DEPLOY_DIR" >&2; exit 1; }
MANIFEST=$(ce1113_latest_manifest "$DEPLOY_DIR")
[[ -n "$MANIFEST" ]] || { echo "ERROR: no existe un manifest versionado de $CE1113_IMAGE." >&2; exit 1; }
IMAGE_FILE=$(ce1113_image_for_manifest "$MANIFEST") || {
    echo "ERROR: el manifest no tiene su artefacto .wic correspondiente." >&2; exit 1;
}

if [[ -z "$REPORT_FILE" ]]; then
    REPORT_FILE=$(mktemp /tmp/ce1113-image-integrity.XXXXXX)
    TEMP_REPORT=1
fi
trap '((TEMP_REPORT == 1)) && rm -f "$REPORT_FILE"' EXIT

failures=0
{
    echo "INTEGRIDAD DE IMAGEN CE1113"
    echo "Imagen: $CE1113_IMAGE"
    echo "Máquina: $CE1113_MACHINE"
    echo "Manifest: $MANIFEST"
    echo "Artefacto: $IMAGE_FILE"
    echo "Tamaño: $(stat -c%s "$IMAGE_FILE") bytes"
    echo "SHA-256: $(sha256sum "$IMAGE_FILE" | awk '{print $1}')"
    echo "Paquetes: $(wc -l < "$MANIFEST")"
    echo

    if ce1113_test_image_stream "$IMAGE_FILE"; then
        echo '[OK] El flujo WIC se puede leer/descomprimir completamente.'
    else
        echo '[ERROR] El flujo WIC está dañado o falta el descompresor.'
        failures=$((failures + 1))
    fi

    while IFS= read -r package; do
        if ce1113_manifest_has "$MANIFEST" "$package"; then
            printf '[OK] Paquete requerido: %s\n' "$package"
        else
            printf '[ERROR] Falta paquete requerido: %s\n' "$package"
            failures=$((failures + 1))
        fi
    done < <(ce1113_required_packages)

    if ce1113_manifest_has_prefix "$MANIFEST" 'kernel-module-snd-bcm2835-'; then
        echo '[OK] Módulo del jack: kernel-module-snd-bcm2835-*'
    else
        echo '[ERROR] Falta kernel-module-snd-bcm2835-* para el jack analógico.'
        failures=$((failures + 1))
    fi

    metadata_file="${MANIFEST}.metadata.sha256"
    current_digest=$(ce1113_metadata_digest "$PROJECT_DIR")
    if [[ ! -f "$metadata_file" ]]; then
        echo '[ERROR] La imagen no tiene huella de metadata; ejecute ./build.sh otra vez.'
        failures=$((failures + 1))
    elif [[ "$(<"$metadata_file")" != "$current_digest" ]]; then
        echo '[ERROR] La metadata activa cambió después de construir la imagen.'
        failures=$((failures + 1))
    else
        echo '[OK] La imagen corresponde al contenido actual de meta-ce1113.'
    fi

    echo
    if ((failures == 0)); then
        echo 'RESULTADO: ÍNTEGRA'
    else
        printf 'RESULTADO: NO ÍNTEGRA (%d error(es))\n' "$failures"
    fi
} > "$REPORT_FILE"

if ((NO_UI == 0)) && [[ -t 0 ]] && command -v whiptail >/dev/null; then
    whiptail --title "Integridad: $CE1113_IMAGE ($CE1113_MACHINE)" \
        --scrolltext --textbox "$REPORT_FILE" 32 94
else
    cat "$REPORT_FILE"
fi

((failures == 0))
