#!/usr/bin/env bash

set -euo pipefail

IMAGE_NAME="aurabot-qemu"
CONTAINER_NAME="aurabot-qemu"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"

if ! docker image inspect "${IMAGE_NAME}" >/dev/null 2>&1; then
    echo "[Docker] Building ${IMAGE_NAME}..."
    docker build \
        -f docker/qemu/Dockerfile \
        -t "${IMAGE_NAME}" \
        --build-arg UID="$(id -u)" \
        --build-arg GID="$(id -g)" \
        .
fi

if docker ps -a --format '{{.Names}}' | grep -qx "${CONTAINER_NAME}"; then
    docker rm -f "${CONTAINER_NAME}" >/dev/null
fi

echo "[Docker] Starting ARM64 QEMU environment..."

docker run \
    --rm \
    -it \
    --name "${CONTAINER_NAME}" \
    -v "${ROOT_DIR}:/workspace" \
    -p 127.0.0.1:2222:2222 \
    -p 127.0.0.1:5000:5000 \
    -p 127.0.0.1:1234:1234 \
    "${IMAGE_NAME}"