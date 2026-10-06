#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}
MACHINE=""; IMAGE=""; BUILD_DIR=""; NO_UI=0; SKIP_CHECK=0
# shellcheck source=scripts/lib/image-common.sh
source "$PROJECT_DIR/scripts/lib/image-common.sh"

usage() {
    cat <<'EOF'
Uso: ./build.sh [opciones]
  -m, --machine raspberrypi4
  -i, --image ce1113-p1
  -p, --poky DIR       Árbol Poky (o variable POKY_DIR)
  -b, --build-dir DIR  Build (por defecto build-<machine>)
      --no-ui          Usa valores por defecto sin menús
      --skip-check     No verifica los artefactos al terminar
EOF
}

while (($#)); do
    case "$1" in
        -m|--machine) MACHINE=${2:?falta la máquina}; shift 2;;
        -i|--image) IMAGE=${2:?falta la imagen}; shift 2;;
        -p|--poky) POKY_DIR=${2:?falta la ruta}; shift 2;;
        -b|--build-dir) BUILD_DIR=${2:?falta el directorio}; shift 2;;
        --no-ui) NO_UI=1; shift;;
        --skip-check) SKIP_CHECK=1; shift;;
        -h|--help) usage; exit 0;;
        *) printf 'Opción desconocida: %s\n' "$1" >&2; usage >&2; exit 2;;
    esac
done

MACHINE=${MACHINE:-raspberrypi4}; IMAGE=${IMAGE:-ce1113-p1}; BUILD_DIR=${BUILD_DIR:-"build-$MACHINE"}
[[ "$MACHINE" == raspberrypi4 ]] || { echo "Este producto solo admite raspberrypi4" >&2; exit 2; }
[[ "$IMAGE" == ce1113-p1 ]] || { echo "La única imagen del producto es ce1113-p1" >&2; exit 2; }
[[ -f "$POKY_DIR/oe-init-build-env" ]] || { echo "No existe $POKY_DIR/oe-init-build-env" >&2; exit 1; }

cd "$POKY_DIR"
# oe-init-build-env de Scarthgap consulta variables opcionales como BBSERVER sin
# usar un valor por defecto. Desactivamos nounset solo mientras se carga Poky.
set +u
# shellcheck disable=SC1091
source oe-init-build-env "$BUILD_DIR" >/dev/null
set -u

# bblayers.conf puede conservar capas del proyecto que fueron renombradas o
# eliminadas. BitBake no puede iniciar si una sola ruta de BBLAYERS no existe,
# por lo que se quitan primero todas las entradas administradas por este script.
# Más abajo se agregan de nuevo únicamente las capas descubiertas en disco.
sed -i "\#${PROJECT_DIR}/layers/meta-#d" conf/bblayers.conf
sed -i "\#${PROJECT_DIR}/meta-ce1113#d" conf/bblayers.conf

sed -i '/^# BEGIN CE1113 MANAGED$/,/^# END CE1113 MANAGED$/d' conf/local.conf
{
    echo '# BEGIN CE1113 MANAGED'
    printf 'MACHINE = "%s"\n' "$MACHINE"
    echo 'DISTRO = "poky"'
    echo 'INIT_MANAGER = "systemd"'
    echo 'LICENSE_FLAGS_ACCEPTED:append = " synaptics-killswitch"'
    echo '# END CE1113 MANAGED'
} >> conf/local.conf

# ==========================================
# Agregar capas al bblayers.conf
# ==========================================
add_layer_if_missing() {
    local layer_path="$1"
    local layer_name
    layer_name=$(basename "$layer_path")

    if grep -Fq "$layer_path" conf/bblayers.conf; then
        echo "Capa ya configurada: $layer_name"
        return 0
    fi

    echo "Agregando capa: $layer_name"

    if ! bitbake-layers add-layer "$layer_path"; then
        echo "Error: no se pudo agregar la capa $layer_path" >&2
        exit 1
    fi
}

# Dependencias específicas de Raspberry Pi
if [[ "$MACHINE" == raspberrypi4 ]]; then
    if [[ -f "$POKY_DIR/meta-raspberrypi/conf/layer.conf" ]]; then
        add_layer_if_missing "$POKY_DIR/meta-raspberrypi"
    else
        echo "Error: falta meta-raspberrypi en $POKY_DIR" >&2
        exit 1
    fi
fi

# Capa única del producto; se añade después de su BSP declarado.
add_layer_if_missing "$PROJECT_DIR/meta-ce1113"

# ==========================================
# Ejecutar compilación
# ==========================================
echo "-----------------------------------------------------------"
printf 'Compilando %s para %s\n' "$IMAGE" "$MACHINE"
echo 'Capa del producto: meta-ce1113'
echo "-----------------------------------------------------------"

bitbake "$IMAGE"

if ((SKIP_CHECK == 0)); then
    # BitBake decide qué reconstruir mediante hashes de contenido, no por la
    # fecha de los archivos. Guardar esta huella junto al manifest evita falsos
    # positivos después de checkout, stash, clone o restauraciones de Git.
    DEPLOY_DIR=$(ce1113_deploy_dir "$BUILDDIR")
    MANIFEST=$(ce1113_latest_manifest "$DEPLOY_DIR")
    if [[ -z "$MANIFEST" ]]; then
        echo "Error: BitBake terminó pero no produjo el manifest de $IMAGE." >&2
        exit 1
    fi
    ce1113_metadata_digest "$PROJECT_DIR" > "${MANIFEST}.metadata.sha256"
    "$PROJECT_DIR/check_image.sh" --no-ui --build-dir "$BUILDDIR"
fi
