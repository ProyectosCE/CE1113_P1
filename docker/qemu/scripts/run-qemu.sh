#!/usr/bin/env bash

set -euo pipefail

WORKSPACE="${AURABOT_WORKSPACE:-/workspace}"
STATE_DIR="${WORKSPACE}/.qemu"

IMAGE_NAME="ubuntu-24.04-server-cloudimg-arm64.img"
IMAGE_URL="https://cloud-images.ubuntu.com/releases/noble/release/${IMAGE_NAME}"

BASE_IMAGE="${STATE_DIR}/${IMAGE_NAME}"
DISK_IMAGE="${STATE_DIR}/aurabot-arm64.qcow2"
SEED_IMAGE="${STATE_DIR}/seed.img"

USER_DATA="${STATE_DIR}/user-data"
META_DATA="${STATE_DIR}/meta-data"

UEFI_CODE="/usr/share/AAVMF/AAVMF_CODE.fd"
UEFI_VARS="${STATE_DIR}/AAVMF_VARS.fd"

MEMORY="${QEMU_MEMORY:-2048}"
CPUS="${QEMU_CPUS:-4}"

SSH_PORT="${QEMU_SSH_PORT:-2222}"
FLASK_PORT="${QEMU_FLASK_PORT:-5000}"
GDB_PORT="${QEMU_GDB_PORT:-1234}"

GUEST_USER="${AURABOT_QEMU_USER:-aurabot}"
GUEST_PASSWORD="${AURABOT_QEMU_PASSWORD:-aurabot}"

mkdir -p "${STATE_DIR}"

if [[ ! -f "${BASE_IMAGE}" ]]; then
    echo "[QEMU] Downloading Ubuntu ARM64 image..."
    curl -L --fail --progress-bar \
        "${IMAGE_URL}" \
        -o "${BASE_IMAGE}"
fi

if [[ ! -f "${DISK_IMAGE}" ]]; then
    echo "[QEMU] Creating VM disk..."
    cp "${BASE_IMAGE}" "${DISK_IMAGE}"
    qemu-img resize "${DISK_IMAGE}" 16G
fi

if [[ ! -f "${UEFI_VARS}" ]]; then
    cp /usr/share/AAVMF/AAVMF_VARS.fd "${UEFI_VARS}"
fi

if [[ ! -f "${SEED_IMAGE}" ]]; then
    cat > "${USER_DATA}" <<EOF
#cloud-config

hostname: aurabot-arm64
manage_etc_hosts: true

users:
  - name: ${GUEST_USER}
    shell: /bin/bash
    groups: [adm, sudo]
    sudo: ALL=(ALL) NOPASSWD:ALL
    lock_passwd: false

ssh_pwauth: true

chpasswd:
  expire: false
  users:
    - name: ${GUEST_USER}
      password: ${GUEST_PASSWORD}
      type: text

package_update: true

packages:
  - python3
  - python3-pip
  - python3-venv
  - python3-flask
  - gdbserver
  - openssh-server
  - curl

runcmd:
  - mkdir -p /opt/aurabot
  - chown -R ${GUEST_USER}:${GUEST_USER} /opt/aurabot
  - systemctl enable ssh
  - systemctl restart ssh
EOF

    cat > "${META_DATA}" <<EOF
instance-id: aurabot-arm64
local-hostname: aurabot-arm64
EOF

    cloud-localds \
        "${SEED_IMAGE}" \
        "${USER_DATA}" \
        "${META_DATA}"
fi

echo "[QEMU] Starting ARM64 virtual machine..."
echo "[QEMU] SSH: localhost:${SSH_PORT}"
echo "[QEMU] Flask: localhost:${FLASK_PORT}"
echo "[QEMU] gdbserver: localhost:${GDB_PORT}"

exec qemu-system-aarch64 \
    -machine virt,gic-version=3 \
    -cpu cortex-a72 \
    -accel tcg,thread=multi \
    -smp "${CPUS}" \
    -m "${MEMORY}" \
    -drive if=pflash,format=raw,readonly=on,file="${UEFI_CODE}" \
    -drive if=pflash,format=raw,file="${UEFI_VARS}" \
    -drive if=none,file="${DISK_IMAGE}",format=qcow2,id=systemdisk \
    -device virtio-blk-pci,drive=systemdisk,bootindex=1 \
    -drive if=none,file="${SEED_IMAGE}",format=raw,readonly=on,id=cloudinit \
    -device virtio-blk-pci,drive=cloudinit \
    -device virtio-rng-pci \
    -netdev user,id=net0,hostfwd=tcp::${SSH_PORT}-:22,hostfwd=tcp::${FLASK_PORT}-:5000,hostfwd=tcp::${GDB_PORT}-:1234 \
    -device virtio-net-pci,netdev=net0,romfile= \
    -nographic