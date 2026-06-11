#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
KERNEL_EFI="${BUILD_DIR}/sys/kernel/kernel.efi"
OUTPUT_IMG="${BUILD_DIR}/brights-efi.img"

if [[ ! -f "${KERNEL_EFI}" ]]; then
  echo "error: ${KERNEL_EFI} not found. Build first: cmake --build build" >&2
  exit 1
fi

# Try xorriso first (creates proper ISO9660 with El Torito)
if command -v xorriso >/dev/null 2>&1; then
  ISO_DIR="${BUILD_DIR}/iso"
  ESP_DIR="${ISO_DIR}/EFI/BOOT"
  mkdir -p "${ESP_DIR}"
  cp "${KERNEL_EFI}" "${ESP_DIR}/BOOTX64.EFI"

  # Build FAT32 EFI boot image (ESP) and place it in the ISO tree
  ESP_IMG="${BUILD_DIR}/efi-boot.img"
  if command -v mkfs.fat >/dev/null 2>&1 && command -v mcopy >/dev/null 2>&1; then
    dd if=/dev/zero of="${ESP_IMG}" bs=1M count=4 2>/dev/null
    mkfs.fat -F 32 -n "BRIGHTS" "${ESP_IMG}" >/dev/null 2>&1
    mmd -i "${ESP_IMG}" ::/EFI ::/EFI/BOOT 2>/dev/null
    mcopy -i "${ESP_IMG}" "${KERNEL_EFI}" ::/EFI/BOOT/BOOTX64.EFI
  else
    python3 "${ROOT_DIR}/tools/create-efi-img.py" "${KERNEL_EFI}" "${ESP_IMG}"
  fi
  cp "${ESP_IMG}" "${ISO_DIR}/efi-boot.img"

  xorriso -as mkisofs \
    -R -J -joliet-long \
    -V "BRIGHTS" \
    -o "${BUILD_DIR}/brights.iso" \
    -e efi-boot.img \
    -no-emul-boot \
    -isohybrid-gpt-basdat \
    "${ISO_DIR}"

   printf "ISO created: %s\n" "${BUILD_DIR}/brights.iso"
fi

# Always create FAT32 EFI system partition image (UEFI bootable)
echo "Creating FAT32 EFI image..."
python3 "${ROOT_DIR}/tools/create-efi-img.py" "${KERNEL_EFI}" "${OUTPUT_IMG}"

printf "EFI image created: %s\n" "${OUTPUT_IMG}"
printf "Usage: qemu-system-x86_64 -drive file=%s,format=raw -m 512M\n" "${OUTPUT_IMG}"
