#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
ISO_DIR="${BUILD_DIR}/iso"
ESP_DIR="${ISO_DIR}/EFI/BOOT"
KERNEL_EFI="${BUILD_DIR}/sys/kernel/kernel.efi"

if [[ ! -f "${KERNEL_EFI}" ]]; then
  echo "error: ${KERNEL_EFI} not found. Build first: cmake --build build" >&2
  exit 1
fi

if ! command -v xorriso >/dev/null 2>&1; then
  echo "error: xorriso not found. Install xorriso to build ISO." >&2
  exit 1
fi

mkdir -p "${ESP_DIR}"
cp "${KERNEL_EFI}" "${ESP_DIR}/BOOTX64.EFI"

xorriso -as mkisofs \
  -R -J -joliet-long \
  -V "BRIGHTS" \
  -o "${BUILD_DIR}/brights.iso" \
  "${ISO_DIR}"

printf "ISO created: %s\n" "${BUILD_DIR}/brights.iso"
