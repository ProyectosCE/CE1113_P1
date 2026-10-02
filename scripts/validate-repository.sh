#!/usr/bin/env bash
set -uo pipefail

errors=0
fail() { printf 'ERROR: %s\n' "$*" >&2; errors=$((errors + 1)); }

required_dirs=(.github/workflows docs meta-ce1113/conf meta-ce1113/recipes-core/images layers scripts)
required_files=(README.md LICENSE build.sh check_image.sh flash_sd.sh meta-ce1113/conf/layer.conf meta-ce1113/recipes-core/images/ce1113-p1.bb layers/meta-aura-apps/conf/layer.conf layers/meta-aura-apps/recipes-apps/aurabot/aurabot_1.0.0.bb)
for path in "${required_dirs[@]}"; do [[ -d "$path" ]] || fail "falta el directorio requerido: $path"; done
for path in "${required_files[@]}"; do [[ -f "$path" ]] || fail "falta el archivo requerido: $path"; done

while IFS= read -r -d '' path; do
    [[ "$path" != *' '* ]] || fail "no se permiten espacios en rutas: $path"
    [[ "$path" =~ ^[A-Za-z0-9._/%+-]+$ ]] || fail "ruta con caracteres no permitidos: $path"
    case "$path" in
        *.log|*.tmp|*.bak|*.swp|*.swo|*~|*/__pycache__/*|*/cache/*|build-*/*|downloads/*|sstate-cache/*)
            fail "archivo generado o temporal versionado: $path" ;;
    esac
    if [[ -f "$path" ]] && (( $(stat -c%s "$path") > 10 * 1024 * 1024 )); then
        fail "archivo mayor de 10 MiB: $path"
    fi
done < <(git ls-files --cached --others --exclude-standard -z | while IFS= read -r -d '' item; do [[ -e "$item" ]] && printf '%s\0' "$item"; done)

mapfile -t layers_found < <(find meta-ce1113 layers -mindepth 1 -maxdepth 3 -type f -path '*/conf/layer.conf' -printf '%h\n' | sed 's#/conf$##' | sort)
(( ${#layers_found[@]} > 0 )) || fail "no se encontraron capas Yocto"

declare -A collections=()
for layer in "${layers_found[@]}"; do
    layer_conf="$layer/conf/layer.conf"
    collection=$(sed -n 's/^BBFILE_COLLECTIONS.*"\([a-z0-9][a-z0-9_-]*\)".*/\1/p' "$layer_conf" | head -n1)
    [[ -n "$collection" ]] || { fail "$layer_conf no declara BBFILE_COLLECTIONS"; continue; }
    [[ -z "${collections[$collection]:-}" ]] || fail "colección duplicada '$collection' en $layer y ${collections[$collection]}"
    collections[$collection]="$layer"
    grep -Eq "^LAYERSERIES_COMPAT_${collection}.*scarthgap" "$layer_conf" || fail "$layer_conf no declara compatibilidad con scarthgap"
    if [[ "$layer" != meta-ce1113 ]]; then
        grep -Eq "^LAYERDEPENDS_${collection}.*ce1113" "$layer_conf" || fail "$layer debe depender de la capa principal ce1113"
    fi
done

mapfile -t primary_images < <(find meta-ce1113/recipes-core/images -maxdepth 1 -type f -name '*.bb' -printf '%f\n')
(( ${#primary_images[@]} == 1 )) || fail "meta-ce1113 debe contener exactamente una receta de imagen principal"
[[ "${primary_images[0]:-}" == ce1113-p1.bb ]] || fail "la imagen principal debe llamarse ce1113-p1.bb"
if find meta-ce1113 -type f -path '*/recipes-*/*/*.bb' ! -path '*/recipes-core/images/ce1113-p1.bb' -print -quit | grep -q .; then
    fail "meta-ce1113 debe conservar solo la receta de imagen; las aplicaciones van dentro de layers/"
fi

while IFS= read -r recipe; do
    name=$(basename "$recipe")
    [[ "$name" =~ ^[a-z0-9][a-z0-9+._%-]*\.(bb|bbappend)$ ]] || fail "nombre de receta no permitido: $recipe"
    [[ "$recipe" =~ /recipes-[a-z0-9+.-]+/[^/]+/[^/]+$ ]] || fail "receta fuera de recipes-<categoria>/<nombre>: $recipe"
done < <(find meta-ce1113 layers -type f \( -name '*.bb' -o -name '*.bbappend' \) | sort)

while IFS= read -r append; do
    [[ $(basename "$append") == ce1113-p1.bbappend ]] || fail "los módulos deben ampliar la imagen con ce1113-p1.bbappend: $append"
done < <(find layers -type f -path '*/recipes-core/images/*.bbappend' | sort)

while IFS= read -r append; do
    [[ "$append" == */recipes-core/images/ce1113-p1.bbappend ]] && continue
    append_name=$(basename "$append")
    case "$append_name" in
        # La receta de meta-raspberrypi se llama rpi-cmdline.bb y no tiene
        # sufijo de versión. El append correcto también debe ir sin sufijo;
        # rpi-cmdline_%.bbappend no coincide y BitBake lo considera huérfano.
        rpi-cmdline.bbappend)
            ;;
        *_%.bbappend)
            ;;
        *)
            fail "un bbappend externo debe declarar versión o comodín explícito: $append"
            ;;
    esac
    if grep -q 'file://' "$append"; then
        grep -q '^FILESEXTRAPATHS:prepend' "$append" || fail "$append usa file:// sin FILESEXTRAPATHS:prepend"
    fi
done < <(find layers meta-ce1113 -type f -name '*.bbappend' | sort)

while IFS= read -r source; do
    [[ "$source" =~ ^layers/meta-[^/]+/recipes-[^/]+/[^/]+/files/ ]] || fail "código fuera de files/ de una receta ubicada en layers/: $source"
done < <(git ls-files --cached --others --exclude-standard '*.c' '*.h' 'CMakeLists.txt' | while read -r item; do [[ -e "$item" ]] && echo "$item"; done)

for script in build.sh check_image.sh flash_sd.sh scripts/*.sh; do
    bash -n "$script" || fail "sintaxis shell inválida: $script"
done

if (( errors > 0 )); then
    printf '\nValidación fallida: %d error(es).\n' "$errors" >&2
    exit 1
fi
printf 'Repositorio válido: estructura, capas, recetas, nombres y scripts verificados.\n'
