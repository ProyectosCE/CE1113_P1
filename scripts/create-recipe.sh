#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
LAYERS_DIR="$PROJECT_DIR/layers"
IMAGE_DIR="$PROJECT_DIR/meta-ce1113/recipes-core/images"
IMAGE_NAME=""

die() { printf 'Error: %s\n' "$*" >&2; exit 1; }
valid_name() { [[ "$1" =~ ^[a-z0-9][a-z0-9-]*$ ]]; }

detect_image() {
    local image_file
    mapfile -t image_recipes < <(find "$IMAGE_DIR" -maxdepth 1 -type f -name '*.bb' -printf '%f\n' | sort)
    ((${#image_recipes[@]} == 1)) || die "se esperaba exactamente una imagen .bb en $IMAGE_DIR"
    image_file=${image_recipes[0]}
    IMAGE_NAME=${image_file%.bb}
}

create_layer() {
    local layer_name=$1 layer_dir="$LAYERS_DIR/$1"
    local collection=${layer_name#meta-}
    collection=${collection//-/}
    [[ ! -e "$layer_dir" ]] || die "ya existe $layer_dir"
    mkdir -p "$layer_dir/conf"
    cat > "$layer_dir/conf/layer.conf" <<EOF
BBPATH .= ":\${LAYERDIR}"
BBFILES += "\${LAYERDIR}/recipes-*/*/*.bb \\
            \${LAYERDIR}/recipes-*/*/*.bbappend"

BBFILE_COLLECTIONS += "$collection"
BBFILE_PATTERN_$collection = "^\${LAYERDIR}/"
BBFILE_PRIORITY_$collection = "7"
LAYERVERSION_$collection = "1"
LAYERDEPENDS_$collection = "core ce1113"
LAYERSERIES_COMPAT_$collection = "scarthgap"
EOF
    printf 'Layer creado: %s\n' "$layer_dir"
}

select_layer() {
    local index answer raw_name
    mapfile -t available_layers < <(
        find "$LAYERS_DIR" -mindepth 1 -maxdepth 1 -type d -name 'meta-*' \
            -exec test -f '{}/conf/layer.conf' \; -printf '%f\n' | sort
    )
    echo 'Layers disponibles:' >&2
    for index in "${!available_layers[@]}"; do
        printf '  %d) %s\n' "$((index + 1))" "${available_layers[$index]}" >&2
    done
    printf '  %d) Crear un layer nuevo\n' "$((${#available_layers[@]} + 1))" >&2
    read -r -p 'Seleccione una opción: ' answer
    [[ "$answer" =~ ^[0-9]+$ ]] || die 'la selección debe ser un número'
    ((answer >= 1 && answer <= ${#available_layers[@]} + 1)) || die 'selección fuera de rango'
    if ((answer <= ${#available_layers[@]})); then
        selected_layer=${available_layers[$((answer - 1))]}
        return
    fi
    read -r -p 'Nombre del nuevo layer (con o sin prefijo meta-): ' raw_name
    raw_name=${raw_name#meta-}
    valid_name "$raw_name" || die 'use minúsculas, números y guiones'
    selected_layer="meta-$raw_name"
    create_layer "$selected_layer"
}

create_recipe() {
    local layer_name=$1 recipe_name=$2
    local layer_dir="$LAYERS_DIR/$layer_name"
    local recipe_dir="$layer_dir/recipes-apps/$recipe_name"
    local recipe_file="$recipe_dir/${recipe_name}_1.0.0.bb"
    local append_dir="$layer_dir/recipes-core/images"
    local append_file="$append_dir/$IMAGE_NAME.bbappend"
    [[ -f "$layer_dir/conf/layer.conf" ]] || die "$layer_name no es un layer válido"
    [[ ! -e "$recipe_dir" ]] || die "ya existe: $recipe_dir"
    mkdir -p "$recipe_dir/files" "$append_dir"
    cat > "$recipe_file" <<EOF
SUMMARY = "Aplicación $recipe_name para CE1113"
DESCRIPTION = "Receta base; agregue manualmente fuentes, dependencias e instalación"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://\${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Añada manualmente los archivos específicos, por ejemplo:
# SRC_URI = "file://archivo.c file://CMakeLists.txt"
SRC_URI = ""
S = "\${WORKDIR}"

# Permite validar la receta antes de añadir contenido instalable.
ALLOW_EMPTY:\${PN} = "1"
EOF
    touch "$recipe_dir/files/.gitkeep"
    if [[ ! -f "$append_file" ]]; then
        cat > "$append_file" <<EOF
# Paquetes que este layer añade a la imagen principal.
IMAGE_INSTALL:append = " $recipe_name"
EOF
    elif ! grep -Eq "(^|[[:space:]\"])${recipe_name}([[:space:]\"]|$)" "$append_file"; then
        printf '\nIMAGE_INSTALL:append = " %s"\n' "$recipe_name" >> "$append_file"
    fi
    printf '\nReceta: %s\nArchivos manuales: %s\nBBappend: %s\n' \
        "$recipe_file" "$recipe_dir/files" "$append_file"
}

[[ -d "$LAYERS_DIR" ]] || die "no existe $LAYERS_DIR"
detect_image
printf 'Imagen principal detectada: %s\n' "$IMAGE_NAME"
select_layer
read -r -p 'Nombre de la receta: ' recipe_name
valid_name "$recipe_name" || die 'use minúsculas, números y guiones'
create_recipe "$selected_layer" "$recipe_name"
echo 'Complete los archivos específicos y compruebe el resultado con bitbake-layers show-appends.'
