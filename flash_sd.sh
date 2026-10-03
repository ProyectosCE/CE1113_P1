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

for tool in lsblk findmnt sfdisk partprobe blockdev timeout blkid mkfs.ext4 dd sync sha256sum; do
    command -v "$tool" >/dev/null || { echo "Error: falta $tool." >&2; exit 1; }
done

settle_udev() {
    if command -v udevadm >/dev/null; then
        sudo timeout --kill-after=2s 8s udevadm settle --timeout=5 || true
    fi
}

reread_partition_table() {
    local device=$1

    sudo sync
    settle_udev

    if sudo timeout --kill-after=2s 10s partprobe -s "$device"; then
        settle_udev
        return 0
    fi

    echo "Advertencia: partprobe no respondió; intentando blockdev --rereadpt." >&2
    if sudo timeout --kill-after=2s 10s blockdev --rereadpt "$device"; then
        settle_udev
        return 0
    fi

    echo "Error: el kernel no pudo releer la tabla de particiones de $device." >&2
    return 1
}

wait_for_block_device() {
    local device=$1
    local attempt

    for attempt in {1..50}; do
        [[ -b "$device" ]] && return 0
        sleep 0.2
    done
    return 1
}

BUILD_DIR=${BUILD_DIR:-"$POKY_DIR/build-$CE1113_MACHINE"}
# La misma validación que sigue al build debe pasar inmediatamente antes del borrado.
"$PROJECT_DIR/check_image.sh" --no-ui --build-dir "$BUILD_DIR"

DEPLOY_DIR=$(ce1113_deploy_dir "$BUILD_DIR")
MANIFEST=$(ce1113_latest_manifest "$DEPLOY_DIR")
IMAGE_FILE=$(ce1113_image_for_manifest "$MANIFEST")
EXPECTED_HASH=$(sha256sum "$IMAGE_FILE" | awk '{print $1}')

image_size_bytes() {
    case "$1" in
        *.wic.bz2) bzip2 -dc "$1" | wc -c ;;
        *.wic.gz)  gzip -dc "$1" | wc -c ;;
        *.wic.xz)  xz -dc "$1" | wc -c ;;
        *.wic)     stat -c%s "$1" ;;
    esac
}

image_raw_sha256() {
    case "$1" in
        *.wic.bz2) bzip2 -dc "$1" | sha256sum | awk '{print $1}' ;;
        *.wic.gz)  gzip -dc "$1" | sha256sum | awk '{print $1}' ;;
        *.wic.xz)  xz -dc "$1" | sha256sum | awk '{print $1}' ;;
        *.wic)     sha256sum "$1" | awk '{print $1}' ;;
    esac
}

partition_path() {
    local device=$1 number=$2
    if [[ "$device" =~ [0-9]$ ]]; then
        printf '%sp%s\n' "$device" "$number"
    else
        printf '%s%s\n' "$device" "$number"
    fi
}

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

# La partición musical ocupa 2 GiB al final de la SD. Si ya existe una tercera
# partición ext4 etiquetada AURA_AUDIO, se conserva exactamente su geometría y
# contenido aunque el WIC reemplace temporalmente la tabla de particiones.
SECTOR_BYTES=512
AUDIO_SIZE_SECTORS=$((2 * 1024 * 1024 * 1024 / SECTOR_BYTES))
DISK_BYTES=$(lsblk -bdno SIZE "$TARGET_DEVICE")
DISK_SECTORS=$((DISK_BYTES / SECTOR_BYTES))
AUDIO_START=$(( (DISK_SECTORS - AUDIO_SIZE_SECTORS) / 2048 * 2048 ))
AUDIO_SECTORS=$AUDIO_SIZE_SECTORS
AUDIO_EXISTS=0
AUDIO_PARTITION=""

while read -r path part_number start size_bytes label filesystem; do
    if [[ "$part_number" == 3 && "$label" == AURA_AUDIO && "$filesystem" == ext4 ]]; then
        AUDIO_PARTITION=$path
        AUDIO_START=$start
        AUDIO_SECTORS=$((size_bytes / SECTOR_BYTES))
        AUDIO_EXISTS=1
    fi
done < <(lsblk -brnpo PATH,PARTN,START,SIZE,LABEL,FSTYPE "$TARGET_DEVICE" | tail -n +2)

IMAGE_BYTES=$(image_size_bytes "$IMAGE_FILE")
IMAGE_SECTORS=$(( (IMAGE_BYTES + SECTOR_BYTES - 1) / SECTOR_BYTES ))
if ((AUDIO_START <= IMAGE_SECTORS + 2048)); then
    echo "Error: el WIC ($IMAGE_BYTES bytes) invade la partición musical reservada." >&2
    exit 1
fi
if ((AUDIO_START + AUDIO_SECTORS > DISK_SECTORS)); then
    echo 'Error: la geometría guardada de AURA_AUDIO excede el tamaño de la SD.' >&2
    exit 1
fi

if ((ASSUME_YES == 0)); then
    message="Dispositivo: $DEVICE_INFO
Imagen: $(basename "$IMAGE_FILE")
SHA-256: $EXPECTED_HASH
Partición musical: $([[ $AUDIO_EXISTS == 1 ]] && echo 'se conservará' || echo 'se creará (2 GiB)')

LAS PARTICIONES DE SISTEMA SERÁN REEMPLAZADAS.
LOS AUDIOS DE AURA_AUDIO SE CONSERVARÁN SI YA EXISTE."
    if [[ -t 0 ]] && command -v whiptail >/dev/null; then
        whiptail --title 'Reemplazar sistema y conservar música' --yesno "$message" 17 88 || exit 0
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
echo "Grabando $(basename "$IMAGE_FILE") en $TARGET_DEVICE..."
case "$IMAGE_FILE" in
    *.wic.bz2) bzip2 -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    *.wic.gz)  gzip -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    *.wic.xz)  xz -dc "$IMAGE_FILE" | sudo dd of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
    *.wic)     sudo dd if="$IMAGE_FILE" of="$TARGET_DEVICE" bs=4M status=progress conv=fsync ;;
esac
sudo sync

# Antes de tocar la tabla que dejó el WIC, se relee del dispositivo exactamente
# la cantidad grabada. Esto detecta una escritura incompleta o dirigida al medio
# equivocado antes de anunciar una SD arrancable.
reread_partition_table "$TARGET_DEVICE"
BOOT_PARTITION=$(partition_path "$TARGET_DEVICE" 1)
SYSTEM_PARTITION=$(partition_path "$TARGET_DEVICE" 2)
AUDIO_PARTITION=$(partition_path "$TARGET_DEVICE" 3)
wait_for_block_device "$BOOT_PARTITION" || {
    echo "Error: el WIC no creó $BOOT_PARTITION." >&2; exit 1;
}
wait_for_block_device "$SYSTEM_PARTITION" || {
    echo "Error: el WIC no creó $SYSTEM_PARTITION." >&2; exit 1;
}

EXPECTED_RAW_HASH=$(image_raw_sha256 "$IMAGE_FILE")
WRITTEN_RAW_HASH=$(sudo dd if="$TARGET_DEVICE" bs=4M iflag=count_bytes \
    count="$IMAGE_BYTES" status=none | sha256sum | awk '{print $1}')
if [[ "$WRITTEN_RAW_HASH" != "$EXPECTED_RAW_HASH" ]]; then
    echo 'Error: la verificación de lectura posterior a la grabación falló.' >&2
    echo "Esperado: $EXPECTED_RAW_HASH" >&2
    echo "Leído:    $WRITTEN_RAW_HASH" >&2
    exit 1
fi

# Guardar la geometría arrancable del WIC. La partición musical se añade sin
# redimensionar ni modificar la partición raíz en vivo.
BOOT_START=$(lsblk -bdno START "$BOOT_PARTITION")
BOOT_SIZE=$(lsblk -bdno SIZE "$BOOT_PARTITION")
SYSTEM_START=$(lsblk -bdno START "$SYSTEM_PARTITION")
SYSTEM_SIZE=$(lsblk -bdno SIZE "$SYSTEM_PARTITION")

# El WIC restaura sus particiones de arranque/rootfs y su tabla, pero no escribe
# hasta el final de la SD. Se vuelve a registrar la partición 3 en la geometría
# reservada; los bloques de datos antiguos nunca fueron sobrescritos.
if lsblk -rnpo PARTN "$TARGET_DEVICE" | grep -qx 3; then
    echo 'Error: el WIC ya contiene una partición 3; no se puede reservar AURA_AUDIO.' >&2
    exit 1
fi
printf 'start=%s, size=%s, type=83\n' "$AUDIO_START" "$AUDIO_SECTORS" \
    | sudo sfdisk --append "$TARGET_DEVICE"
reread_partition_table "$TARGET_DEVICE"
wait_for_block_device "$AUDIO_PARTITION" || {
    echo "Error: no apareció $AUDIO_PARTITION después de recrear la tabla." >&2; exit 1;
}

if ((AUDIO_EXISTS == 0)); then
    sudo mkfs.ext4 -F -L AURA_AUDIO "$AUDIO_PARTITION"
else
    [[ "$(sudo blkid -s TYPE -o value "$AUDIO_PARTITION")" == ext4 ]] || {
        echo 'Error: la partición musical conservada ya no se reconoce como ext4.' >&2; exit 1;
    }
    [[ "$(sudo blkid -s LABEL -o value "$AUDIO_PARTITION")" == AURA_AUDIO ]] || {
        echo 'Error: no se recuperó la etiqueta AURA_AUDIO.' >&2; exit 1;
    }
fi
sudo sync

# El alta de p3 no puede cambiar los límites de arranque ni de rootfs.
[[ "$(lsblk -bdno START "$BOOT_PARTITION")" == "$BOOT_START" &&
   "$(lsblk -bdno SIZE "$BOOT_PARTITION")" == "$BOOT_SIZE" &&
   "$(lsblk -bdno START "$SYSTEM_PARTITION")" == "$SYSTEM_START" &&
   "$(lsblk -bdno SIZE "$SYSTEM_PARTITION")" == "$SYSTEM_SIZE" ]] || {
    echo 'Error: la geometría de las particiones arrancables cambió.' >&2
    exit 1
}
[[ "$(lsblk -bdno START "$AUDIO_PARTITION")" == "$AUDIO_START" &&
   $(( $(lsblk -bdno SIZE "$AUDIO_PARTITION") / SECTOR_BYTES )) == "$AUDIO_SECTORS" ]] || {
    echo 'Error: AURA_AUDIO no quedó en la geometría solicitada.' >&2
    exit 1
}
[[ "$(sudo blkid -p -s TYPE -o value "$BOOT_PARTITION")" == vfat ]] || {
    echo "Error: $BOOT_PARTITION no contiene un filesystem FAT arrancable." >&2
    exit 1
}
[[ "$(sudo blkid -p -s TYPE -o value "$SYSTEM_PARTITION")" == ext4 ]] || {
    echo "Error: $SYSTEM_PARTITION no contiene el rootfs ext4." >&2
    exit 1
}

message="Grabación completada en $TARGET_DEVICE.
Origen verificado: $EXPECTED_HASH
Lectura verificada: $WRITTEN_RAW_HASH
Música: $AUDIO_PARTITION ($([[ $AUDIO_EXISTS == 1 ]] && echo preservada || echo creada))
Puede retirar la tarjeta de forma segura."
if [[ -t 0 ]] && command -v whiptail >/dev/null; then
    whiptail --title 'Grabación completada' --msgbox "$message" 11 80
else
    printf '%s\n' "$message"
fi
