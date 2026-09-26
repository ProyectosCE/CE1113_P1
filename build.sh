#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
POKY_DIR=${POKY_DIR:-"$HOME/poky-scarthgap-5.0.19"}
MACHINE=""; IMAGE=""; LAYERS="all"; BUILD_DIR=""; NO_UI=0

usage() {
    cat <<'EOF'
Uso: ./build.sh [opciones]
  -m, --machine qemuarm64|raspberrypi4
  -i, --image ce1113-p1|core-image-minimal|rpi-test-image
  -l, --layers all|none|meta-red,meta-server,...
  -p, --poky DIR       Árbol Poky (o variable POKY_DIR)
  -b, --build-dir DIR  Build (por defecto build-<machine>)
      --no-ui          Usa valores por defecto sin menús
EOF
}

while (($#)); do
    case "$1" in
        -m|--machine) MACHINE=${2:?falta la máquina}; shift 2;;
        -i|--image) IMAGE=${2:?falta la imagen}; shift 2;;
        -l|--layers) LAYERS=${2:?faltan las capas}; shift 2;;
        -p|--poky) POKY_DIR=${2:?falta la ruta}; shift 2;;
        -b|--build-dir) BUILD_DIR=${2:?falta el directorio}; shift 2;;
        --no-ui) NO_UI=1; shift;; -h|--help) usage; exit 0;;
        *) printf 'Opción desconocida: %s\n' "$1" >&2; usage >&2; exit 2;;
    esac
done

if ((NO_UI == 0)) && command -v whiptail >/dev/null && [[ -t 0 ]]; then
    [[ -n "$MACHINE" ]] || MACHINE=$(whiptail --title "Máquina" --radiolist "Destino:" 12 65 2 qemuarm64 "Emulador ARM64" ON raspberrypi4 "Raspberry Pi 4" OFF 3>&1 1>&2 2>&3) || exit 1
    [[ -n "$IMAGE" ]] || IMAGE=$(whiptail --title "Imagen" --radiolist "Receta:" 14 72 3 ce1113-p1 "Imagen principal CE1113" ON core-image-minimal "Diagnóstico mínimo" OFF rpi-test-image "Pruebas Raspberry Pi" OFF 3>&1 1>&2 2>&3) || exit 1
fi
MACHINE=${MACHINE:-qemuarm64}; IMAGE=${IMAGE:-ce1113-p1}; BUILD_DIR=${BUILD_DIR:-"build-$MACHINE"}
[[ "$MACHINE" =~ ^(qemuarm64|raspberrypi4)$ ]] || { echo "Máquina no permitida: $MACHINE" >&2; exit 2; }
[[ "$IMAGE" =~ ^(ce1113-p1|core-image-minimal|rpi-test-image)$ ]] || { echo "Imagen no permitida: $IMAGE" >&2; exit 2; }
[[ "$IMAGE" != rpi-test-image || "$MACHINE" == raspberrypi4 ]] || { echo "rpi-test-image requiere raspberrypi4" >&2; exit 2; }
[[ -f "$POKY_DIR/oe-init-build-env" ]] || { echo "No existe $POKY_DIR/oe-init-build-env" >&2; exit 1; }

mapfile -t AVAILABLE_LAYERS < <(find "$PROJECT_DIR/layers" -mindepth 1 -maxdepth 1 -type d -name 'meta-*' -printf '%f\n' | sort)
case "$LAYERS" in
    all)
        SELECTED_LAYERS=()
        for layer in "${AVAILABLE_LAYERS[@]}"; do
            [[ "$MACHINE" != raspberrypi4 && "$layer" == meta-red ]] || SELECTED_LAYERS+=("$layer")
        done
        ;;
    none|'') SELECTED_LAYERS=();;
    *) IFS=',' read -r -a SELECTED_LAYERS <<< "$LAYERS";;
esac
for layer in "${SELECTED_LAYERS[@]}"; do [[ -f "$PROJECT_DIR/layers/$layer/conf/layer.conf" ]] || { echo "Capa desconocida: $layer" >&2; exit 2; }; done
[[ "$MACHINE" == raspberrypi4 || ! " ${SELECTED_LAYERS[*]} " =~ [[:space:]]meta-red[[:space:]] ]] || { echo "meta-red requiere raspberrypi4" >&2; exit 2; }

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
    echo 'LICENSE_FLAGS_ACCEPTED:append = " synaptics-killswitch"'
    echo '# END CE1113 MANAGED'
} >> conf/local.conf

bitbake-layers add-layer "$PROJECT_DIR/meta-ce1113"
if [[ "$MACHINE" == raspberrypi4 ]]; then
    [[ -f "$POKY_DIR/meta-raspberrypi/conf/layer.conf" ]] || { echo "Falta meta-raspberrypi en $POKY_DIR" >&2; exit 1; }
    bitbake-layers add-layer "$POKY_DIR/meta-raspberrypi" >/dev/null 2>&1 || true
fi
for layer in "${SELECTED_LAYERS[@]}"; do bitbake-layers add-layer "$PROJECT_DIR/layers/$layer"; done
printf 'Compilando %s para %s; capas: %s\n' "$IMAGE" "$MACHINE" "${SELECTED_LAYERS[*]:-(ninguna)}"
bitbake "$IMAGE"
