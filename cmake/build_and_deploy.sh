#!/bin/bash

# Detener el script si ocurre algún error durante la ejecución
set -e

# Definir variables útiles
BUILD_DIR="build"
MOUNT_POINT="/media/rootfs"
DEVICE="/dev/mmcblk0p2"

echo "=== Limpiando directorio de compilación ==="
sudo rm -rf "${BUILD_DIR:?}"/*

echo "=== Entrando al directorio y configurando CMake ==="
cd "$BUILD_DIR"
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm_toolchain.cmake -DCMAKE_INSTALL_PREFIX:PATH="$HOME/build/usr"

echo "=== Compilando con Make ==="
make -j$(nproc)

echo "=== Montando la partición objetivo ==="
# Verificar si el punto de montaje existe, si no, crearlo
if [ ! -d "$MOUNT_POINT" ]; then
    sudo mkdir -p "$MOUNT_POINT"
fi

sudo mount "$DEVICE" "$MOUNT_POINT"

# Asegurar que se desmonte el dispositivo incluso si falla el copiado
trap 'echo "=== Desmontando unidad ==="; sync; sudo umount "$MOUNT_POINT"' EXIT

echo "=== Copiando binarios y librerías ==="
sudo cp -r ../../musica "$MOUNT_POINT/home/"
sudo cp src/main "$MOUNT_POINT/bin/"
sudo cp libdriver/lib/libdriver.so "$MOUNT_POINT/lib/"
sudo cp libgpio/lib/libgpio.so "$MOUNT_POINT/lib/"
sudo cp libaudio/lib/libaudio.so "$MOUNT_POINT/lib/"

echo "=== Despliegue completado con éxito ==="
# El bloque 'trap' se encargará de ejecutar el sync y umount automáticamente al salir