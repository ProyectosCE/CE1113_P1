#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
SOURCE_DIR=""; AUDIO_PARTITION=""; NO_UI=0
MOUNT_DIR=""; MOUNTED_HERE=0; PLAYLIST_FILE=""

usage() {
    cat <<'EOF'
Uso: ./load_audio.sh [--source DIR] [--partition /dev/mmcblk0p3] [--no-ui]

Copia únicamente archivos MP3 a la partición ext4 AURA_AUDIO. No ejecuta
BitBake, no modifica el WIC y no toca las particiones boot/rootfs.
EOF
}

while (($#)); do
    case "$1" in
        -s|--source) SOURCE_DIR=${2:?falta la carpeta}; shift 2 ;;
        -p|--partition) AUDIO_PARTITION=${2:?falta la partición}; shift 2 ;;
        --no-ui) NO_UI=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) printf 'Opción desconocida: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

for tool in lsblk findmnt find cp mount umount df stat sort realpath awk sed tr dirname mktemp; do
    command -v "$tool" >/dev/null || { echo "Error: falta $tool." >&2; exit 1; }
done

if [[ -z "$SOURCE_DIR" ]]; then
    options=()
    [[ -d "$PROJECT_DIR/media" ]] && options+=("$PROJECT_DIR/media" 'Carpeta media del repositorio')
    [[ -d "$PROJECT_DIR/audios" ]] && options+=("$PROJECT_DIR/audios" 'Carpeta audios del repositorio')
    if ((NO_UI == 0)) && [[ -t 0 ]] && command -v whiptail >/dev/null && ((${#options[@]})); then
        SOURCE_DIR=$(whiptail --title 'Cargar canciones' --menu \
            'Seleccione la carpeta de origen:' 16 84 6 "${options[@]}" \
            3>&1 1>&2 2>&3) || exit 0
    elif [[ -d "$PROJECT_DIR/media" ]]; then
        SOURCE_DIR="$PROJECT_DIR/media"
    elif [[ -d "$PROJECT_DIR/audios" ]]; then
        SOURCE_DIR="$PROJECT_DIR/audios"
    else
        echo 'Error: indique --source DIR o cree ./media o ./audios.' >&2
        exit 1
    fi
fi
SOURCE_DIR=$(realpath "$SOURCE_DIR")
[[ -d "$SOURCE_DIR" ]] || { echo "Error: no existe $SOURCE_DIR" >&2; exit 1; }

mapfile -d '' -t SONGS < <(find "$SOURCE_DIR" -type f -iname '*.mp3' -print0)
((${#SONGS[@]} > 0)) || { echo "Error: no hay archivos MP3 en $SOURCE_DIR" >&2; exit 1; }

if [[ -z "$AUDIO_PARTITION" ]]; then
    mapfile -t AUDIO_PARTITIONS < <(lsblk -rpo PATH,LABEL,FSTYPE,TYPE \
        | awk '$2 == "AURA_AUDIO" && $3 == "ext4" && $4 == "part" {print $1}')
    ((${#AUDIO_PARTITIONS[@]} > 0)) || {
        echo 'Error: no se encontró una partición ext4 etiquetada AURA_AUDIO.' >&2; exit 1;
    }
    if ((${#AUDIO_PARTITIONS[@]} == 1)); then
        AUDIO_PARTITION=${AUDIO_PARTITIONS[0]}
    elif ((NO_UI == 0)) && [[ -t 0 ]] && command -v whiptail >/dev/null; then
        partition_options=()
        for partition in "${AUDIO_PARTITIONS[@]}"; do
            partition_options+=("$partition" "$(lsblk -dn -o SIZE "$partition") AURA_AUDIO")
        done
        AUDIO_PARTITION=$(whiptail --title 'Partición musical' --menu \
            'Seleccione el destino:' 16 76 6 "${partition_options[@]}" \
            3>&1 1>&2 2>&3) || exit 0
    else
        echo 'Error: hay varias particiones AURA_AUDIO; indique --partition.' >&2; exit 1
    fi
fi

[[ -b "$AUDIO_PARTITION" ]] || { echo "Error: $AUDIO_PARTITION no es una partición." >&2; exit 1; }
[[ "$(lsblk -dn -o LABEL "$AUDIO_PARTITION")" == AURA_AUDIO ]] || {
    echo 'Error: la partición no tiene la etiqueta AURA_AUDIO.' >&2; exit 1;
}
[[ "$(lsblk -dn -o FSTYPE "$AUDIO_PARTITION")" == ext4 ]] || {
    echo 'Error: AURA_AUDIO debe usar ext4.' >&2; exit 1;
}

PARENT_NAME=$(lsblk -ndo PKNAME "$AUDIO_PARTITION")
[[ -n "$PARENT_NAME" ]] || { echo 'Error: no se pudo determinar el disco padre.' >&2; exit 1; }
PARENT_DEVICE="/dev/$PARENT_NAME"
ROOT_SOURCE=$(findmnt -n -o SOURCE / 2>/dev/null || true)
ROOT_PARENT=$(lsblk -ndo PKNAME "$ROOT_SOURCE" 2>/dev/null | head -n1 || true)
[[ -z "$ROOT_PARENT" || "$PARENT_DEVICE" != "/dev/$ROOT_PARENT" ]] || {
    echo 'Error: se bloqueó el disco que contiene el sistema host.' >&2; exit 1;
}
transport=$(lsblk -dn -o TRAN "$PARENT_DEVICE")
removable=$(lsblk -dn -o RM "$PARENT_DEVICE")
[[ "$removable" == 1 || "$transport" == usb || "$transport" == mmc ]] || {
    echo "Error: $PARENT_DEVICE no parece una SD o unidad removible." >&2; exit 1;
}

cleanup() {
    if ((MOUNTED_HERE == 1)); then
        sudo umount "$MOUNT_DIR" || true
        rmdir "$MOUNT_DIR" 2>/dev/null || true
    fi
    [[ -z "$PLAYLIST_FILE" ]] || rm -f -- "$PLAYLIST_FILE"
}
trap cleanup EXIT

MOUNT_DIR=$(findmnt -nr -S "$AUDIO_PARTITION" -o TARGET | head -n1 || true)
if [[ -z "$MOUNT_DIR" ]]; then
    MOUNT_DIR=$(mktemp -d /tmp/aurabot-audio.XXXXXX)
    sudo mount "$AUDIO_PARTITION" "$MOUNT_DIR"
    MOUNTED_HERE=1
fi

required_bytes=0
for song in "${SONGS[@]}"; do
    required_bytes=$((required_bytes + $(stat -c%s "$song")))
done
available_bytes=$(df -B1 --output=avail "$MOUNT_DIR" | tail -n1 | tr -d ' ')
((required_bytes <= available_bytes)) || {
    echo "Error: se requieren $required_bytes bytes y solo hay $available_bytes libres." >&2; exit 1;
}

printf 'Copiando %d MP3 a %s...\n' "${#SONGS[@]}" "$AUDIO_PARTITION"
for song in "${SONGS[@]}"; do
    relative=${song#"$SOURCE_DIR"/}
    destination="$MOUNT_DIR/$relative"
    sudo mkdir -p "$(dirname "$destination")"
    sudo cp -f -- "$song" "$destination"
done

# La lista contiene rutas válidas dentro de la Raspberry, no rutas del host.
PLAYLIST_FILE=$(mktemp /tmp/aurabot-playlist.XXXXXX)
find "$MOUNT_DIR" -type f -iname '*.mp3' -printf '%P\n' \
    | LC_ALL=C sort \
    | sed 's#^#/media/audio/#' > "$PLAYLIST_FILE"
sudo cp -f -- "$PLAYLIST_FILE" "$MOUNT_DIR/playlist.txt"
sudo sync

printf 'Carga terminada: %d canción(es). Playlist: /media/audio/playlist.txt\n' "${#SONGS[@]}"
