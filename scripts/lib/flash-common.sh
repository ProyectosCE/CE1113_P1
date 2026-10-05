#!/usr/bin/env bash

# El llamador debe usar set -euo pipefail y definir FLASH_STAGE/TARGET_DEVICE.
ce1113_flash_failed() {
    local result=${1:-1}
    trap - ERR
    # ERR se hereda dentro de $(...) y pipelines. Solo el proceso principal
    # informa el fallo; el subshell conserva el código para que llegue al padre.
    if ((BASH_SUBSHELL > 0)); then
        exit "$result"
    fi
    printf '\nError durante: %s (código %s).\n' "$FLASH_STAGE" "$result" >&2
    echo 'La SD no se ha declarado verificada. No se reintentará su escritura automáticamente.' >&2
    if [[ -n "${TARGET_DEVICE:-}" ]]; then
        echo 'Para revisar errores del lector/tarjeta: sudo dmesg | tail -60' >&2
    fi
    exit "$result"
}

ce1113_readback_hash() {
    local reader=$1 device=$2 bytes=$3 direct_read=$4
    local flags=fullblock,count_bytes
    if ((direct_read)); then
        flags+=,direct
    fi
    # count_bytes limita por bytes, incluso si el WIC no es múltiplo de 4 MiB.
    # No usar noerror ni rellenar datos: cualquier fallo debe invalidar la SD.
    sudo "$reader" if="$device" bs=4M iflag="$flags" count="$bytes" \
        status=progress | sha256sum | awk '{print $1}'
}

ce1113_verify_readback() {
    local result
    FLASH_STAGE='lectura de verificación'
    WRITTEN_RAW_HASH=''
    echo "Verificando por lectura $IMAGE_BYTES bytes de $TARGET_DEVICE..."
    # Descartar datos de caché antes de verificar. El destino debe estar desmontado.
    sudo blockdev --flushbufs "$TARGET_DEVICE" || return "$?"
    if WRITTEN_RAW_HASH=$(ce1113_readback_hash "$DD_TOOL" "$TARGET_DEVICE" \
            "$IMAGE_BYTES" "$DIRECT_READ"); then
        if [[ "$WRITTEN_RAW_HASH" == "$EXPECTED_RAW_HASH" ]]; then
            echo 'Contenido del WIC verificado correctamente por lectura.'
            return 0
        fi
        echo 'Error: los bytes leídos no coinciden con el WIC.' >&2
        echo "Esperado: $EXPECTED_RAW_HASH" >&2
        echo "Leído:    $WRITTEN_RAW_HASH" >&2
        WRITTEN_RAW_HASH=''
        return 1
    else
        result=$?
        WRITTEN_RAW_HASH=''
        echo 'La lectura falló: pruebe otro lector/adaptador o tarjeta antes de volver a escribir.' >&2
        if ((DIRECT_READ)); then
            echo 'Si dd muestra Invalid argument, el dispositivo podría no admitir lectura directa.' >&2
        fi
        return "$result"
    fi
}
