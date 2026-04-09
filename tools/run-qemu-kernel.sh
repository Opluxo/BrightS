#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
KERNEL_EFI="${BUILD_DIR}/sys/kernel/kernel.efi"

if [[ ! -f "${KERNEL_EFI}" ]]; then
  echo "error: ${KERNEL_EFI} not found. Build first: cmake --build build" >&2
  exit 1
fi

# Find OVMF files
OVMF_CODE="/usr/share/edk2/x64/OVMF_CODE.4m.fd"
OVMF_VARS="/tmp/brights-ovmf-vars.fd"

if [[ ! -f "${OVMF_CODE}" ]]; then
  echo "error: OVMF_CODE not found at ${OVMF_CODE}" >&2
  exit 1
fi

# Copy OVMF vars to writable location
cp /usr/share/edk2/x64/OVMF_VARS.4m.fd "${OVMF_VARS}" 2>/dev/null || true

echo "=== BrightS QEMU Launcher ==="
echo "Kernel: ${KERNEL_EFI}"
echo ""

# Run QEMU with -kernel to directly load the EFI binary
qemu-system-x86_64 \
  -machine q35 \
  -m 512M \
  -drive if=pflash,format=raw,readonly=on,file="${OVMF_CODE}" \
  -drive if=pflash,format=raw,file="${OVMF_VARS}" \
  -kernel "${KERNEL_EFI}" \
  -serial stdio \
  -display none \
  -no-reboot
