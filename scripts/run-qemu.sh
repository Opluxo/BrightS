#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -d "${ROOT_DIR}/build-brights" ]]; then
  BUILD_DIR="${ROOT_DIR}/build-brights"
else
  BUILD_DIR="${ROOT_DIR}/build"
fi
ESP_DIR="${BUILD_DIR}/esp"
KERNEL_EFI="${BUILD_DIR}/sys/kernel/kernel.efi"
OVMF_CODE=""
OVMF_VARS=""

find_ovmf() {
  local code_candidates=(
    "/usr/share/OVMF/OVMF_CODE.fd"
    "/usr/share/OVMF/x64/OVMF_CODE.4m.fd"
    "/usr/share/edk2/x64/OVMF_CODE.4m.fd"
    "/usr/share/edk2-ovmf/x64/OVMF_CODE.4m.fd"
  )
  local vars_candidates=(
    "/usr/share/OVMF/OVMF_VARS.fd"
    "/usr/share/OVMF/x64/OVMF_VARS.4m.fd"
    "/usr/share/edk2/x64/OVMF_VARS.4m.fd"
    "/usr/share/edk2-ovmf/x64/OVMF_VARS.4m.fd"
  )
  for p in "${code_candidates[@]}"; do
    if [[ -f "${p}" ]]; then
      OVMF_CODE="${p}"
      break
    fi
  done
  for p in "${vars_candidates[@]}"; do
    if [[ -f "${p}" ]]; then
      OVMF_VARS="${p}"
      break
    fi
  done
}

if [[ ! -f "${KERNEL_EFI}" ]]; then
  echo "error: ${KERNEL_EFI} not found. Build first: cmake --build build" >&2
  exit 1
fi

find_ovmf
if [[ -z "${OVMF_CODE}" || -z "${OVMF_VARS}" ]]; then
  echo "error: OVMF firmware files not found in common locations" >&2
  exit 1
fi

mkdir -p "${ESP_DIR}/EFI/BOOT"
cp "${KERNEL_EFI}" "${ESP_DIR}/EFI/BOOT/BOOTX64.EFI"

qemu-system-x86_64 \
  -machine q35 \
  -m 512M \
  -drive if=pflash,format=raw,readonly=on,file="${OVMF_CODE}" \
  -drive if=pflash,format=raw,file="${OVMF_VARS}" \
  -drive format=raw,file=fat:rw:"${ESP_DIR}" \
  -monitor none \
  -serial stdio
