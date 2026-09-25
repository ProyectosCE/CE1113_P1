#!/bin/bash

POKY_DIR="$HOME/poky-scarthgap-5.0.19"
PROJECT_DIR="$HOME/CE1113_P1"

if ! command -v whiptail &> /dev/null; then
    echo "Error: whiptail no está instalado."
    exit 1
fi

MACHINE_SEL=$(whiptail --title "Verificación de Imagen" --menu \
"Selecciona la máquina que compilaste:" 15 65 2 \
"qemuarm64" "Emulador QEMU ARM64" \
"raspberrypi4" "Raspberry Pi 4" 3>&1 1>&2 2>&3)

[ $? -ne 0 ] && exit 1

IMAGE_SEL=$(whiptail --title "Verificación de Imagen" --menu \
"Selecciona la imagen que compilaste:" 16 65 3 \
"CE1113-P1" "Imagen final personalizada" \
"core-image-minimal" "Imagen base limpia de Poky" \
"rpi-test-image" "Imagen de pruebas RPi" 3>&1 1>&2 2>&3)

[ $? -ne 0 ] && exit 1

LATEST_MANIFEST=$(find "$POKY_DIR"/build*/tmp/deploy/images -type f -name "${IMAGE_SEL}*.manifest" -printf '%T@ %p\n' 2>/dev/null | grep "$MACHINE_SEL" | sort -n | tail -1 | awk '{print $2}')

if [ -z "$LATEST_MANIFEST" ]; then
    LATEST_MANIFEST=$(find "$POKY_DIR"/build*/tmp/deploy/images -type f -name "${IMAGE_SEL}*.manifest" -printf '%T@ %p\n' 2>/dev/null | sort -n | tail -1 | awk '{print $2}')
fi

if [ -z "$LATEST_MANIFEST" ]; then
    whiptail --msgbox "Error: No se encontró ningún archivo .manifest para $IMAGE_SEL ($MACHINE_SEL)\n\n¿Estás seguro de que la compilación terminó exitosamente?" 12 70
    exit 1
fi

DEPLOY_DIR=$(dirname "$LATEST_MANIFEST")
IMAGE_BASE=$(basename "$LATEST_MANIFEST" .manifest)

REPORT_FILE=$(mktemp)

echo "=================================================" > "$REPORT_FILE"
echo " REPORTE DE COMPILACION: $IMAGE_SEL" >> "$REPORT_FILE"
echo "=================================================" >> "$REPORT_FILE"
echo "Ruta: $DEPLOY_DIR" >> "$REPORT_FILE"
echo "=================================================" >> "$REPORT_FILE"
echo " TAMANO DE ARCHIVOS" >> "$REPORT_FILE"
echo "=================================================" >> "$REPORT_FILE"

ls -lhL "$DEPLOY_DIR"/${IMAGE_BASE}.* 2>/dev/null | grep -E "\.(ext4|wic|wic\.bz2|tar\.bz2|cpio\.gz)$" | awk '{print "- " $5 " \t" $9}' | while read line; do
    size=$(echo "$line" | awk '{print $2}')
    name=$(basename $(echo "$line" | awk '{print $3}'))
    echo "  Peso: $size | Archivo: $name" >> "$REPORT_FILE"
done

echo "=================================================" >> "$REPORT_FILE"
echo " CONTENIDO DE LA IMAGEN" >> "$REPORT_FILE"
echo "=================================================" >> "$REPORT_FILE"
PKG_COUNT=$(wc -l < "$LATEST_MANIFEST")
echo "  Total de paquetes instalados: $PKG_COUNT" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

echo "  --- Verificacion Dinamica de Paquetes Custom ---" >> "$REPORT_FILE"
CUSTOM_PKGS=$(find "$PROJECT_DIR" -name "*.bb" 2>/dev/null | grep -v "/recipes-core/images/" | awk -F/ '{print $NF}' | cut -d'_' -f1 | cut -d'.' -f1 | sort -u)

if [ -z "$CUSTOM_PKGS" ]; then
    echo "  (No se detectaron recetas de paquetes en tus capas)" >> "$REPORT_FILE"
else
    for pkg in $CUSTOM_PKGS; do
        if awk '{print $1}' "$LATEST_MANIFEST" | grep -q "^${pkg}$"; then
            echo "  [OK] $pkg (Inyectado y presente!)" >> "$REPORT_FILE"
        else
            echo "  [X] $pkg (No encontrado en esta imagen)" >> "$REPORT_FILE"
        fi
    done
fi

echo "" >> "$REPORT_FILE"
echo "  --- Primeros 10 paquetes del sistema base ---" >> "$REPORT_FILE"
head -n 10 "$LATEST_MANIFEST" | awk '{print "  - " $1 " (v" $3 ")"}' >> "$REPORT_FILE"
echo "  ..." >> "$REPORT_FILE"

# Altura dinámica: líneas del reporte + margen para bordes/título/botón,
# limitada al alto real de la terminal para que no se salga de pantalla.
CONTENT_LINES=$(wc -l < "$REPORT_FILE")
TERM_LINES=$(tput lines 2>/dev/null || echo 40)
BOX_HEIGHT=$((CONTENT_LINES + 6))
MAX_HEIGHT=$((TERM_LINES - 2))
[ "$BOX_HEIGHT" -gt "$MAX_HEIGHT" ] && BOX_HEIGHT=$MAX_HEIGHT
[ "$BOX_HEIGHT" -lt 15 ] && BOX_HEIGHT=15

whiptail --title "Reporte: $IMAGE_SEL ($MACHINE_SEL)" --textbox "$REPORT_FILE" "$BOX_HEIGHT" 80

rm -f "$REPORT_FILE"