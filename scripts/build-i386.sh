#!/bin/bash
# BrightS i386 Build Script
# For 32-bit BIOS boot (Samsung NC10, etc.)

set -e

cd "$(dirname "$0")/.."

echo "=== BrightS i386 Build ==="

mkdir -p build

echo "[1/4] Building bootloader..."
nasm -f bin -o build/bootloader.bin kernel/arch/i386/boot/bootloader.asm

echo "[2/4] Building kernel..."
nasm -f bin -o build/kernel.bin kernel/arch/i386/entry.asm

echo "[3/4] Creating floppy image..."
dd if=/dev/zero of=build/floppy.img bs=1024 count=1440 2>/dev/null
dd if=build/bootloader.bin of=build/floppy.img conv=notrunc
dd if=build/kernel.bin of=build/floppy.img seek=2 conv=notrunc

echo "[4/4] Done!"
echo "Output: build/floppy.img (1.44MB)"
ls -lh build/floppy.img

echo ""
echo "To write to USB:"
echo "  sudo dd if=build/floppy.img of=/dev/sdX bs=1M"