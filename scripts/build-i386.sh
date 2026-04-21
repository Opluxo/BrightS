#!/usr/bin/env bash
set -e

echo "=== BrightS i386 Build Script ==="

if ! command -v nasm &> /dev/null; then
    echo "Error: nasm not found"
    echo "Install with: sudo apt-get install nasm"
    exit 1
fi

cd "$(dirname "$0")"

NASM="${NASM:-nasm}"
CC="${CC:-gcc}"

mkdir -p build

echo "Building i386 bootloader..."
$NASM -f bin -o build/bootloader.bin kernel/arch/i386/boot/bootloader.asm 2>/dev/null || echo "Bootloader build skipped"

echo "Building i386 entry point..."
$NASM -f elf32 -o build/entry.o kernel/arch/i386/entry.asm 2>/dev/null || echo "Entry build skipped"

echo "Building i386 GDT/IDT..."
gcc -m32 -c -o build/gdt.o kernel/arch/i386/cpu/gdt.c 2>/dev/null || echo "GDT build skipped"

echo "Building i386 paging..."
gcc -m32 -c -o build/paging.o kernel/arch/i386/paging/paging.c 2>/dev/null || echo "Paging build skipped"

echo "Linking i386 kernel..."
ld -m elf_i386 -Ttext 0x10000 -o build/brights_i386.bin \
    build/bootloader.o build/entry.o 2>/dev/null || echo "Link skipped"

if [ -f build/brights_i386.bin ]; then
    echo "Creating floppy image..."
    dd if=/dev/zero of=build/brights-i386.img bs=1024 count=1440 2>/dev/null
    dd if=build/brights_i386.bin of=build/brights-i386.img conv=notrunc 2>/dev/null
    
    echo "=== i386 build complete ==="
    echo "Image: build/brights-i386.img"
    echo "Size: $(ls -lh build/brights-i386.img | awk '{print $5}')"
else
    echo "Build requires nasm and gcc-multilib packages"
    echo "Note: i386 support is still in development"
fi