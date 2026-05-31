#!/bin/bash
# BrightS x86_64 UEFI Build Script
# Uses CMake + clang + lld-link to produce a PE32+ EFI application
set -e

cd "$(dirname "$0")/.."
PROJ=$(pwd)

BUILD_DIR="$PROJ/build_x64"

echo "=== BrightS x86_64 UEFI Build ==="
echo "Project: $PROJ"
echo "Build dir: $BUILD_DIR"

# Configure with CMake
echo "[1/3] Configuring CMake..."
mkdir -p "$BUILD_DIR"
cmake -S "$PROJ" -B "$BUILD_DIR" -DUEFI_CLANG=ON

# Build kernel EFI
echo "[2/3] Building kernel..."
cmake --build "$BUILD_DIR" -j$(nproc)

# Verify output
echo "[3/3] Verifying..."
KERNEL="$BUILD_DIR/sys/kernel/kernel.efi"
BOOT="$BUILD_DIR/sys/kernel/EFI/BOOT/BOOTX64.EFI"

if [ -f "$KERNEL" ]; then
    echo "  kernel.efi: $(stat -c%s "$KERNEL") bytes"
else
    echo "  ERROR: kernel.efi not found!"
    exit 1
fi

if [ -f "$BOOT" ]; then
    echo "  BOOTX64.EFI: $(stat -c%s "$BOOT") bytes"
fi

echo ""
echo "=== Done ==="
echo ""
echo "To run in QEMU:"
echo "  qemu-system-x86_64 -bios /usr/share/edk2/x64/OVMF.4m.fd"
echo "    -drive file=fat:rw:${BUILD_DIR}/sys/kernel,format=raw"
echo "    -serial stdio -m 512"
echo ""
echo "To debug:"
echo "  qemu-system-x86_64 -bios /usr/share/edk2/x64/OVMF.4m.fd"
echo "    -drive file=fat:rw:${BUILD_DIR}/sys/kernel,format=raw"
echo "    -serial stdio -m 512 -s -S"
echo "  gdb -ex 'target remote :1234'"
