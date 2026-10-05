#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source "$PROJECT_DIR/scripts/lib/flash-common.sh"
TEST_DIR=$(mktemp -d /tmp/ce1113-readback-test.XXXXXX)
trap 'rm -f -- "$TEST_DIR/image" "$TEST_DIR/errors"; rmdir -- "$TEST_DIR"' EXIT

DD_TOOL=$(command -v gnudd || command -v dd)
"$DD_TOOL" --version | grep -q 'coreutils)' || {
    echo 'Esta prueba requiere GNU dd.' >&2
    exit 1
}

# Todas las pruebas leen un archivo temporal. Nunca usan sudo ni una SD.
sudo() {
    if [[ "$1" == blockdev ]]; then
        return "${FLUSH_RESULT:-0}"
    fi
    "$@"
}
fail() { echo "PRUEBA FALLIDA: $*" >&2; exit 1; }

TARGET_DEVICE="$TEST_DIR/image"
FLASH_STAGE=prueba
DIRECT_READ=0
# Incluye más bytes que el WIC simulado y un último bloque parcial de 512 bytes.
"$DD_TOOL" if=/dev/zero of="$TARGET_DEVICE" bs=512 count=8195 status=none
IMAGE_BYTES=$((4 * 1024 * 1024 + 512))
EXPECTED_RAW_HASH=$("$DD_TOOL" if="$TARGET_DEVICE" bs=512 count=8193 status=none \
    | sha256sum | awk '{print $1}')
ce1113_verify_readback || fail 'verificación válida o límite exacto de bytes'
[[ "$WRITTEN_RAW_HASH" == "$EXPECTED_RAW_HASH" ]] || fail 'hash válido'

IMAGE_BYTES=$((IMAGE_BYTES + 4096))
if ce1113_verify_readback 2>/dev/null; then
    fail 'se aceptó una lectura más corta de lo solicitado'
fi
[[ -z "$WRITTEN_RAW_HASH" ]] || fail 'hash inválido no descartado'
IMAGE_BYTES=$((4 * 1024 * 1024 + 512))

EXPECTED_RAW_HASH=incorrecto
if ce1113_verify_readback 2>/dev/null; then
    fail 'se aceptó contenido diferente'
fi

FLAGS_EXPECTED=fullblock,count_bytes,direct
recording_reader() {
    [[ "$*" == *"bs=4M"* && "$*" == *"iflag=$FLAGS_EXPECTED"* &&
       "$*" == *"count=$IMAGE_BYTES"* ]] || return 8
    printf 'datos simulados'
}
DIRECT_READ=1
DD_TOOL=recording_reader
EXPECTED_RAW_HASH=$(printf 'datos simulados' | sha256sum | awk '{print $1}')
ce1113_verify_readback || fail 'opción de lectura directa'

# Aunque un lector produzca bytes cuyo hash coincide, un error invalida la SD.
failing_reader() { printf 'datos simulados'; return 7; }
DD_TOOL=failing_reader
if ce1113_verify_readback 2>/dev/null; then
    fail 'se ignoró el error del lector dentro del pipeline'
else
    [[ $? == 7 ]] || fail 'no se conservó el código del lector'
fi
[[ -z "$WRITTEN_RAW_HASH" ]] || fail 'se conservó el hash de una lectura fallida'

FLUSH_RESULT=9
if ce1113_verify_readback 2>/dev/null; then
    fail 'se ignoró el fallo al vaciar la caché'
else
    [[ $? == 9 ]] || fail 'código de vaciado de caché'
fi

# Reproduce el ERR heredado por una sustitución de comando; informa una vez.
export -f ce1113_flash_failed
export FLASH_STAGE TARGET_DEVICE
if bash -c 'set -Eeuo pipefail; trap '\''ce1113_flash_failed "$?"'\'' ERR; value=$(false)' \
        2>"$TEST_DIR/errors"; then
    fail 'el error no terminó el proceso'
fi
[[ $(grep -c 'Error durante:' "$TEST_DIR/errors") == 1 ]] || fail 'error duplicado'

echo 'Pruebas correctas: bytes exactos, lectura corta, hash distinto, lectura directa, E/S, vaciado y error único.'
