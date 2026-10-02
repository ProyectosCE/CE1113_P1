#!/usr/bin/env bash

# Funciones compartidas por check_image.sh y flash_sd.sh.
CE1113_MACHINE=raspberrypi4
CE1113_IMAGE=ce1113-p1

ce1113_deploy_dir() {
    printf '%s/tmp/deploy/images/%s\n' "$1" "$CE1113_MACHINE"
}

ce1113_latest_manifest() {
    find -L "$1" -maxdepth 1 -type f \
        -name "${CE1113_IMAGE}-${CE1113_MACHINE}.rootfs-*.manifest" \
        -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n1 | cut -d' ' -f2-
}

ce1113_image_for_manifest() {
    local base extension
    base=${1%.manifest}
    for extension in wic.bz2 wic.gz wic.xz wic; do
        if [[ -f "$base.$extension" ]]; then
            printf '%s\n' "$base.$extension"
            return 0
        fi
    done
    return 1
}

ce1113_test_image_stream() {
    case "$1" in
        *.wic.bz2) command -v bzip2 >/dev/null && bzip2 -t "$1" ;;
        *.wic.gz)  command -v gzip >/dev/null && gzip -t "$1" ;;
        *.wic.xz)  command -v xz >/dev/null && xz -t "$1" ;;
        *.wic)     [[ -s "$1" ]] ;;
        *)         return 1 ;;
    esac
}

ce1113_manifest_has() {
    awk -v package="$2" '$1 == package { found=1 } END { exit !found }' "$1"
}

ce1113_manifest_has_prefix() {
    awk -v prefix="$2" 'index($1, prefix) == 1 { found=1 } END { exit !found }' "$1"
}

ce1113_required_packages() {
    printf '%s\n' \
        packagegroup-ce1113 packagegroup-ce1113-hw packagegroup-ce1113-webapp \
        packagegroup-ce1113-api packagegroup-ce1113-auraapp \
        aurabot hola app-operaciones webapp audio-mp3 ce1113-audio-config \
        alsa-utils alsa-utils-aplay alsa-utils-alsamixer mpg123 wifi-config
}

ce1113_sources_newer_than() {
    find "$1/meta-ce1113" -type f -newer "$2" \
        -print -quit 2>/dev/null | grep -q .
}
