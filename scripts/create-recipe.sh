#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
META_DIR="$PROJECT_DIR/meta-ce1113"
PACKAGEGROUP_DIR="$META_DIR/recipes-core/packagegroups"
CATEGORIES=(lib test hw auraapp)
CATEGORY=""; RECIPE_NAME=""; VERSION=1.0.0

usage() {
    cat <<'EOF'
Uso: ./scripts/create-recipe.sh [--category ÁREA --name NOMBRE] [--version VERSIÓN]

Áreas permitidas: hw, webapp, api, auraapp. Sin argumentos usa el asistente.
La receta se registra en el packagegroup de su área; la imagen no se modifica.
EOF
}

die() { printf 'Error: %s\n' "$*" >&2; exit 1; }
valid_name() { [[ "$1" =~ ^[a-z0-9][a-z0-9-]*$ ]]; }

select_category() {
    local index answer

    echo 'Categorías disponibles:' >&2
    for index in "${!CATEGORIES[@]}"; do
        printf '  %d) %s\n' "$((index + 1))" "${CATEGORIES[$index]}" >&2
    done
    read -r -p 'Seleccione una categoría: ' answer
    [[ "$answer" =~ ^[0-9]+$ ]] || die 'la selección debe ser un número'
    ((answer >= 1 && answer <= ${#CATEGORIES[@]})) || die 'selección fuera de rango'
    CATEGORY=${CATEGORIES[$((answer - 1))]}
}

create_recipe() {
    local category=$1 recipe_name=$2
    local recipe_dir="$META_DIR/recipes-$category/$recipe_name"
    local recipe_file="$recipe_dir/${recipe_name}_${VERSION}.bb"
    local packagegroup="$PACKAGEGROUP_DIR/packagegroup-ce1113-$category.bb"

    [[ -f "$META_DIR/conf/layer.conf" ]] || die 'meta-ce1113 no es una capa válida'
    [[ -f "$packagegroup" ]] || die "no existe el packagegroup del área: $packagegroup"
    [[ ! -e "$recipe_dir" ]] || die "ya existe: $recipe_dir"

    mkdir -p "$recipe_dir/files"
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
    : > "$recipe_dir/files/.gitkeep"

    if ! grep -Eq "(^|[[:space:]\"])${recipe_name}([[:space:]\"]|$)" "$packagegroup"; then
        printf '\n# Añadido por create-recipe.sh\nRDEPENDS:${PN}:append = " %s"\n' \
            "$recipe_name" >> "$packagegroup"
    fi

    printf '\nReceta: %s\nArchivos manuales: %s\nPackagegroup: %s\n' \
        "$recipe_file" "$recipe_dir/files" "$packagegroup"
}

[[ -d "$META_DIR" ]] || die "no existe $META_DIR"
while (($#)); do
    case "$1" in
        --category) CATEGORY=${2:?falta la categoría}; shift 2 ;;
        --name) RECIPE_NAME=${2:?falta el nombre}; shift 2 ;;
        --version) VERSION=${2:?falta la versión}; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) die "opción desconocida: $1" ;;
    esac
done

if [[ -z "$CATEGORY" ]]; then select_category; fi
[[ " ${CATEGORIES[*]} " == *" $CATEGORY "* ]] || die "categoría no permitida: $CATEGORY"
if [[ -z "$RECIPE_NAME" ]]; then read -r -p 'Nombre de la receta: ' RECIPE_NAME; fi
valid_name "$RECIPE_NAME" || die 'use minúsculas, números y guiones'
[[ "$VERSION" =~ ^[0-9]+([.][0-9]+)*$ ]] || die 'la versión debe ser numérica (por ejemplo, 1.0.0)'
create_recipe "$CATEGORY" "$RECIPE_NAME"
echo 'Complete los archivos específicos y ejecute ./scripts/validate-repository.sh.'
