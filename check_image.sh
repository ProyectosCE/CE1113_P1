#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}
MACHINE=${MACHINE:-raspberrypi4}; IMAGE=${IMAGE:-ce1113-p1}; BUILD_DIR=""

usage() { echo "Uso: ./check_image.sh [-m máquina] [-i imagen] [-p poky] [-b build-dir]"; }
while (($#)); do
    case "$1" in
        -m|--machine) MACHINE=${2:?}; shift 2;; -i|--image) IMAGE=${2:?}; shift 2;;
        -p|--poky) POKY_DIR=${2:?}; shift 2;; -b|--build-dir) BUILD_DIR=${2:?}; shift 2;;
        -h|--help) usage; exit 0;; *) echo "Opción desconocida: $1" >&2; exit 2;;
    esac
done
BUILD_DIR=${BUILD_DIR:-"$POKY_DIR/build-$MACHINE"}; DEPLOY_DIR="$BUILD_DIR/tmp/deploy/images/$MACHINE"
[[ -d "$DEPLOY_DIR" ]] || { echo "No existe el despliegue: $DEPLOY_DIR" >&2; exit 1; }
LATEST_MANIFEST=$(find "$DEPLOY_DIR" -maxdepth 1 -type f -name "${IMAGE}-${MACHINE}*.manifest" -printf '%T@ %p\n' | sort -nr | head -n1 | cut -d' ' -f2-)
[[ -n "$LATEST_MANIFEST" ]] || { echo "No se encontró manifest de $IMAGE para $MACHINE." >&2; exit 1; }
IMAGE_BASE=${LATEST_MANIFEST%.manifest}
printf 'Imagen: %s\nMáquina: %s\nManifest: %s\nPaquetes: %s\n' "$IMAGE" "$MACHINE" "$LATEST_MANIFEST" "$(wc -l < "$LATEST_MANIFEST")"
echo 'Artefactos:'
find "$(dirname "$LATEST_MANIFEST")" -maxdepth 1 \( -type f -o -type l \) -name "$(basename "$IMAGE_BASE").*" -printf '  %10s  %f\n' | sort || true
echo 'Paquetes propios:'
while IFS= read -r recipe; do
    package=$(basename "$recipe"); package=${package%%_*}; package=${package%.bb}
    if awk -v pkg="$package" '$1 == pkg { found=1 } END { exit !found }' "$LATEST_MANIFEST"; then printf '  [OK] %s\n' "$package"; else printf '  [--] %s (capa no activa o paquete ausente)\n' "$package"; fi
done < <(find "$PROJECT_DIR/meta-ce1113" "$PROJECT_DIR/layers" -type f -name '*.bb' ! -path '*/recipes-core/images/*' | sort)
