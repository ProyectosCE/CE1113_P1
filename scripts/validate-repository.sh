#!/usr/bin/env bash
set -uo pipefail

errors=0
fail() { printf 'ERROR: %s\n' "$*" >&2; errors=$((errors + 1)); }

required_dirs=(.github/workflows docs meta-ce1113/conf meta-ce1113/recipes-core/images meta-ce1113/recipes-core/packagegroups scripts scripts/lib)
required_files=(
    README.md
    LICENSE
    build.sh
    check_image.sh
    flash_sd.sh
    load_audio.sh
    scripts/check-repository.sh
    scripts/create-recipe.sh
    scripts/lib/image-common.sh
    meta-ce1113/conf/layer.conf
    meta-ce1113/recipes-core/images/ce1113-p1.bb
    meta-ce1113/recipes-core/packagegroups/packagegroup-ce1113.bb
    meta-ce1113/recipes-core/packagegroups/packagegroup-ce1113-hw.bb
    meta-ce1113/recipes-core/packagegroups/packagegroup-ce1113-lib.bb
    meta-ce1113/recipes-core/packagegroups/packagegroup-ce1113-test.bb
    meta-ce1113/recipes-core/packagegroups/packagegroup-ce1113-auraapp.bb
    meta-ce1113/recipes-auraapp/aurabot/aurabot_1.0.0.bb
)
for path in "${required_dirs[@]}"; do [[ -d "$path" ]] || fail "falta el directorio requerido: $path"; done
for path in "${required_files[@]}"; do [[ -f "$path" ]] || fail "falta el archivo requerido: $path"; done

# La capa de producto usa una taxonomía cerrada. Esto evita que reaparezcan
# recipes-api, recipes-webapp u otras categorías paralelas después de mover una
# receta sin retirar su estructura anterior.
while IFS= read -r directory; do
    case "$(basename "$directory")" in
        conf|recipes-core|recipes-lib|recipes-test|recipes-hw|recipes-auraapp) ;;
        *) fail "categoría superior no permitida en meta-ce1113: $directory" ;;
    esac
done < <(find meta-ce1113 -mindepth 1 -maxdepth 1 -type d | sort)

for component in libgpio pwm audio leds sensors; do
    [[ -d "meta-ce1113/recipes-lib/$component" ]] || \
        fail "falta la biblioteca requerida recipes-lib/$component"
done
while IFS= read -r directory; do
    case "$(basename "$directory")" in
        libgpio|pwm|audio|leds|sensors) ;;
        *) fail "recipes-lib solo admite libgpio, pwm, audio, leds y sensors: $directory" ;;
    esac
done < <(find meta-ce1113/recipes-lib -mindepth 1 -maxdepth 1 -type d | sort)

[[ -d meta-ce1113/recipes-test/app-operaciones ]] || \
    fail 'falta recipes-test/app-operaciones'
while IFS= read -r directory; do
    [[ "$(basename "$directory")" == app-operaciones ]] || \
        fail "componente no permitido en recipes-test: $directory"
done < <(find meta-ce1113/recipes-test -mindepth 1 -maxdepth 1 -type d | sort)

for component in aurabot aurabot-audio-server webapp; do
    [[ -d "meta-ce1113/recipes-auraapp/$component" ]] || \
        fail "falta la aplicación recipes-auraapp/$component"
done
while IFS= read -r directory; do
    case "$(basename "$directory")" in
        aurabot|aurabot-audio-server|webapp) ;;
        *) fail "componente no permitido en recipes-auraapp: $directory" ;;
    esac
done < <(find meta-ce1113/recipes-auraapp -mindepth 1 -maxdepth 1 -type d | sort)

expected_hw_components=(
    audio-config
    audio-storage
    bootfiles
    brcmfmac-firmware-fix
    busybox
    linux
    wifi-config
)
for component in "${expected_hw_components[@]}"; do
    [[ -d "meta-ce1113/recipes-hw/$component" ]] || \
        fail "falta el componente físico recipes-hw/$component"
done
while IFS= read -r directory; do
    allowed=0
    for expected in "${expected_hw_components[@]}"; do
        [[ "$(basename "$directory")" == "$expected" ]] && allowed=1
    done
    ((allowed == 1)) || \
        fail "recipes-hw solo admite activación/configuración física: $directory"
done < <(find meta-ce1113/recipes-hw -mindepth 1 -maxdepth 1 -type d | sort)

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

layers_found=(meta-ce1113)

declare -A collections=()
for layer in "${layers_found[@]}"; do
    layer_conf="$layer/conf/layer.conf"
    collection=$(sed -n 's/^BBFILE_COLLECTIONS.*"\([a-z0-9][a-z0-9_-]*\)".*/\1/p' "$layer_conf" | head -n1)
    [[ -n "$collection" ]] || { fail "$layer_conf no declara BBFILE_COLLECTIONS"; continue; }
    [[ -z "${collections[$collection]:-}" ]] || fail "colección duplicada '$collection' en $layer y ${collections[$collection]}"
    collections[$collection]="$layer"
    grep -Eq "^LAYERSERIES_COMPAT_${collection}.*scarthgap" "$layer_conf" || fail "$layer_conf no declara compatibilidad con scarthgap"
done

for category in lib test hw auraapp; do
    required_doc="docs/development/repository-structure.md"
    grep -q "recipes-$category" "$required_doc" || fail "$required_doc no documenta recipes-$category"
done

# meta-pwm se conserva fuera del producto actual como referencia experimental;
# ninguna otra capa funcional debe reaparecer bajo layers/.
while IFS= read -r layer; do
    [[ "$layer" == layers/meta-pwm ]] || fail "solo meta-pwm puede conservarse fuera de la capa única: $layer"
done < <(find layers -mindepth 1 -maxdepth 1 -type d -name 'meta-*' | sort)

mapfile -t primary_images < <(find meta-ce1113/recipes-core/images -maxdepth 1 -type f -name '*.bb' -printf '%f\n')
(( ${#primary_images[@]} == 1 )) || fail "meta-ce1113 debe contener exactamente una receta de imagen principal"
[[ "${primary_images[0]:-}" == ce1113-p1.bb ]] || fail "la imagen principal debe llamarse ce1113-p1.bb"

for category in recipes-lib recipes-test recipes-hw recipes-auraapp; do
    [[ -d "meta-ce1113/$category" ]] || fail "falta la categoría funcional meta-ce1113/$category"
done

expected_packagegroups=(
    packagegroup-ce1113.bb
    packagegroup-ce1113-auraapp.bb
    packagegroup-ce1113-hw.bb
    packagegroup-ce1113-lib.bb
    packagegroup-ce1113-test.bb
)
for packagegroup in "${expected_packagegroups[@]}"; do
    [[ -f "meta-ce1113/recipes-core/packagegroups/$packagegroup" ]] || \
        fail "falta el packagegroup requerido: $packagegroup"
done
while IFS= read -r packagegroup; do
    allowed=0
    for expected in "${expected_packagegroups[@]}"; do
        [[ "$packagegroup" == "$expected" ]] && allowed=1
    done
    ((allowed == 1)) || fail "packagegroup heredado o no permitido: $packagegroup"
done < <(find meta-ce1113/recipes-core/packagegroups -maxdepth 1 -type f \
    -name '*.bb' -printf '%f\n' | sort)

while IFS= read -r recipe; do
    name=$(basename "$recipe")
    [[ "$name" =~ ^[a-z0-9][a-z0-9+._%-]*\.(bb|bbappend)$ ]] || fail "nombre de receta no permitido: $recipe"
    [[ "$recipe" =~ /recipes-[a-z0-9+.-]+/[^/]+/[^/]+$ ]] || fail "receta fuera de recipes-<categoria>/<nombre>: $recipe"
    [[ "$recipe" =~ ^meta-ce1113/recipes-(core|lib|test|hw|auraapp)/ ]] || fail "categoría de receta no permitida: $recipe"
done < <(find meta-ce1113 -type f \( -name '*.bb' -o -name '*.bbappend' \) | sort)

while IFS= read -r append; do
    fail "la imagen única no debe ampliarse mediante bbappend; use packagegroups: $append"
done < <(find meta-ce1113 layers -type f -path '*/recipes-core/images/*.bbappend' | sort)

while IFS= read -r append; do
    [[ "$append" == */recipes-core/images/ce1113-p1.bbappend ]] && continue
    append_name=$(basename "$append")
    # BitBake admite las tres formas siguientes, según el nombre externo:
    #   foo.bb             -> foo.bbappend
    #   foo_1.2.bb         -> foo_1.2.bbappend (versión exacta)
    #   foo_<versión>.bb   -> foo_%.bbappend (comodín)
    # El sufijo _% no es obligatorio: rpi-cmdline.bb y rpi-config_git.bb son
    # recetas externas sin versión. Sin las capas externas no puede deducirse
    # cuál forma corresponde; BitBake detectará después un bbappend huérfano.
    # Aquí solo se rechaza '%' si no es el comodín de versión final "_%".
    if [[ "$append_name" == *%* && "$append_name" != *_%.bbappend ]]; then
        fail "comodín bbappend no permitido; use nombre_%.bbappend: $append"
    fi
    if grep -q 'file://' "$append"; then
        grep -q '^FILESEXTRAPATHS:prepend' "$append" || fail "$append usa file:// sin FILESEXTRAPATHS:prepend"
    fi
done < <(find meta-ce1113 -type f -name '*.bbappend' | sort)

while IFS= read -r source; do
    [[ "$source" =~ ^meta-ce1113/recipes-(lib|test|hw|auraapp)/[^/]+/files/ ]] || fail "código fuera de files/ de una receta: $source"
done < <(git ls-files --cached --others --exclude-standard '*.c' '*.h' 'CMakeLists.txt' | while read -r item; do [[ -e "$item" ]] && echo "$item"; done)

# La música vive únicamente en la partición persistente AURA_AUDIO. Incluirla
# en una receta aumentaría el WIC y obligaría a recompilar para cambiar canciones.
while IFS= read -r media_file; do
    fail "archivo de audio dentro de metadata Yocto; use load_audio.sh: $media_file"
done < <(find meta-ce1113 -type f \( -iname '*.mp3' -o -iname '*.wav' -o -iname '*.flac' -o -iname '*.ogg' \) | sort)

grep -q 'packagegroup-ce1113' meta-ce1113/recipes-core/images/ce1113-p1.bb || fail "ce1113-p1 debe instalar únicamente el packagegroup raíz"
for group in lib test hw auraapp; do
    grep -q "packagegroup-ce1113-$group" meta-ce1113/recipes-core/packagegroups/packagegroup-ce1113.bb || fail "el packagegroup raíz no incluye el área $group"
done


# Las aplicaciones consumen las bibliotecas compartidas de recipes-lib. La
# webapp delega audio por FIFO al servidor para mantener un solo dueño de ALSA.
grep -Eq '^DEPENDS = "pwm"' \
    meta-ce1113/recipes-auraapp/aurabot/aurabot_1.0.0.bb || \
    fail 'aurabot debe depender de la receta pwm'
grep -q 'target_link_libraries(aurabot PRIVATE ${PWM_LIBRARY})' \
    meta-ce1113/recipes-auraapp/aurabot/files/CMakeLists.txt || \
    fail 'aurabot debe enlazar libpwm dinámicamente'
grep -Eq '^DEPENDS = "audio"' \
    meta-ce1113/recipes-auraapp/aurabot-audio-server/aurabot-audio-server_1.0.0.bb || \
    fail 'aurabot-audio-server debe depender de la receta audio'
grep -q 'target_link_libraries(aurabot-audio-server PRIVATE ${AUDIO_LIBRARY})' \
    meta-ce1113/recipes-auraapp/aurabot-audio-server/files/CMakeLists.txt || \
    fail 'aurabot-audio-server debe enlazar libaudio dinámicamente'
grep -q '/run/aurabot-audio/control' \
    meta-ce1113/recipes-auraapp/webapp/files/operaciones.cgi.c || \
    fail 'la webapp debe controlar audio mediante el servidor AuraBot'

for component in libgpio pwm audio leds sensors; do
    grep -Eq 'add_library\([^ ]+ SHARED ' \
        "meta-ce1113/recipes-lib/$component/files/CMakeLists.txt" || \
        fail "recipes-lib/$component debe producir una biblioteca SHARED"
done

# BusyBox se amplía desde el área que consume cada función: Wi-Fi pertenece a
# hardware y CGI/HTTP a la aplicación web. No deben volver a duplicarse.
grep -q 'busybox-wifi.cfg' meta-ce1113/recipes-hw/busybox/busybox_%.bbappend || \
    fail 'recipes-hw debe habilitar el soporte Wi-Fi de BusyBox'
grep -q 'httpd-cgi.cfg' meta-ce1113/recipes-auraapp/webapp/busybox_%.bbappend || \
    fail 'recipes-auraapp debe habilitar HTTP/CGI de BusyBox'
grep -q 'httpd-cgi.cfg' meta-ce1113/recipes-hw/busybox/busybox_%.bbappend && \
    fail 'HTTP/CGI no debe configurarse desde recipes-hw'
grep -q 'busybox-wifi.cfg' meta-ce1113/recipes-auraapp/webapp/busybox_%.bbappend && \
    fail 'Wi-Fi no debe configurarse desde recipes-auraapp'

# El flasheo reserva una partición musical persistente al final sin modificar
# el tamaño de boot/rootfs, y verifica lo realmente escrito antes de continuar.
grep -q 'AUDIO_SIZE_SECTORS=.*2 \* 1024 \* 1024 \* 1024' flash_sd.sh || \
    fail 'flash_sd.sh debe reservar 2 GiB para AURA_AUDIO'
grep -q 'sfdisk --append' flash_sd.sh || \
    fail 'flash_sd.sh debe crear AURA_AUDIO como tercera partición'
grep -q 'WRITTEN_RAW_HASH' flash_sd.sh || \
    fail 'flash_sd.sh debe verificar por lectura el WIC escrito'
if grep -Eq 'resizepart|resize2fs' flash_sd.sh; then
    fail 'flash_sd.sh no debe redimensionar boot/rootfs durante el flasheo'
fi

if grep -Eq '^(IMAGE_INSTALL|RPI_EXTRA_CONFIG|KERNEL_MODULE_AUTOLOAD|PACKAGECONFIG)' meta-ce1113/conf/layer.conf; then
    fail "layer.conf debe contener solo metadata de la capa, no configuración funcional"
fi

for script in build.sh check_image.sh flash_sd.sh load_audio.sh scripts/*.sh; do
    bash -n "$script" || fail "sintaxis shell inválida: $script"
done
bash -n scripts/lib/image-common.sh || fail 'sintaxis shell inválida: scripts/lib/image-common.sh'

grep -q 'check_image.sh.*--no-ui' build.sh || fail 'build.sh debe verificar la imagen al terminar'
grep -q 'check_image.sh.*--no-ui' flash_sd.sh || fail 'flash_sd.sh debe verificar la imagen antes de borrar'
grep -q 'scripts/check-repository.sh' .github/workflows/validation.yml || fail 'el workflow debe ejecutar check-repository.sh'

if (( errors > 0 )); then
    printf '\nValidación fallida: %d error(es).\n' "$errors" >&2
    exit 1
fi
printf 'Repositorio válido: estructura, capas, recetas, nombres y scripts verificados.\n'
