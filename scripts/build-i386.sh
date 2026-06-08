#!/bin/bash
# BrightS i386 Build Script
# Full compilation pipeline: NASM -> GCC (-m32) -> LD -> objcopy -> floppy img
set -e

cd "$(dirname "$0")/.."
PROJ=$(pwd)

BUILD="$PROJ/build"
ARCH="$PROJ/arch/i386"
CORE="$PROJ/kernel/core"
FS="$PROJ/kernel/fs"
NET="$PROJ/kernel/net"
IPC="$PROJ/kernel/ipc"
DRIVERS="$PROJ/drivers"

echo "=== BrightS i386 Build ==="
echo "Project: $PROJ"

mkdir -p "$BUILD"

# ---- Step 1: NASM assembly ----
echo "[1/5] Assembling (NASM)..."
nasm -f elf32 -o "$BUILD/entry_asm.o"  "$ARCH/entry.asm"
nasm -f elf32 -o "$BUILD/isr_asm.o"    "$ARCH/isr.asm"
nasm -f elf32 -o "$BUILD/user_enter.o"              "$ARCH/user_enter.S"
nasm -f elf32 -o "$BUILD/user_enter_from_syscall.o"  "$ARCH/user_enter_from_syscall.S"
echo "  OK"

# ---- Step 1b: Rust kernel module (optional) ----
RUST_LIB=""
RUST_DEFINE=""
echo "  RUST..."
RUST_SRC="$PROJ/kernel/rust/src/lib.rs"
RUST_TARGET="$PROJ/kernel/rust/i686-brights-kernel.json"
RUSTC_BOOTSTRAP=1 rustc -Zunstable-options \
  --target "$RUST_TARGET" \
  -C opt-level=z -C panic=abort \
  --crate-type staticlib --out-dir "$BUILD" "$RUST_SRC" 2>/dev/null && {
    echo "  Rust kernel module compiled OK"
    RUST_LIB="-L$BUILD -lrust_kernel"
    RUST_DEFINE="-DBRIGHTS_RUST_ENABLED"
} || {
    echo "  (Rust not available — using pure C fallback)"
    RUST_LIB=""
    RUST_DEFINE=""
}

# ---- Step 2: GCC compilation flags ----
GCC_FLAGS="-m32 -ffreestanding -nostdlib -std=gnu11 -mno-sse -mno-sse2 $RUST_DEFINE"
GCC_FLAGS="$GCC_FLAGS -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual"
GCC_FLAGS="$GCC_FLAGS -Wstrict-prototypes -Wmissing-prototypes"
GCC_FLAGS="$GCC_FLAGS -Wno-unused-parameter -Wno-pointer-sign"
GCC_FLAGS="$GCC_FLAGS -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast"
GCC_FLAGS="$GCC_FLAGS -Wno-cast-qual"
GCC_FLAGS="$GCC_FLAGS -Os -fomit-frame-pointer -fno-stack-protector"
GCC_FLAGS="$GCC_FLAGS -ffunction-sections -fdata-sections"

INCLUDES=""
INCLUDES="$INCLUDES -I$ARCH"
INCLUDES="$INCLUDES -I$CORE"
INCLUDES="$INCLUDES -I$PROJ/kernel"
INCLUDES="$INCLUDES -I$CORE/lightshell_cmds"
INCLUDES="$INCLUDES -I$FS"
INCLUDES="$INCLUDES -I$NET"
INCLUDES="$INCLUDES -I$NET/dhcp"
INCLUDES="$INCLUDES -I$NET/dns"
INCLUDES="$INCLUDES -I$NET/http"
INCLUDES="$INCLUDES -I$IPC"
INCLUDES="$INCLUDES -I$DRIVERS"
INCLUDES="$INCLUDES -I$PROJ/include/kernel"
INCLUDES="$INCLUDES -I$PROJ/include"

CC=${CC:-gcc}

cc() {
    local src="$1"
    local obj="$BUILD/$(basename "$src" .c).o"
    echo "  CC  $src"
    $CC $GCC_FLAGS $INCLUDES -c -o "$obj" "$src"
}

cc_asm() {
    local src="$1"
    local obj="$BUILD/$(basename "$src" .S).o"
    echo "  CC  $src"
    gcc $GCC_FLAGS $INCLUDES -c -o "$obj" "$src"
}

# ---- Step 2: Compile i386 arch C files ----
echo "[2/5] Compiling i386 arch..."
cc "$ARCH/entry.c"
cc "$ARCH/gdt.c"
cc "$ARCH/idt.c"
cc "$ARCH/trap.c"
cc "$ARCH/pci.c"
cc "$ARCH/pic_pit.c"
cc "$ARCH/paging.c"
cc "$ARCH/syscall_abi.c"
cc "$ARCH/apic.c"
cc "$ARCH/ioapic.c"
cc "$ARCH/hpet.c"
cc "$ARCH/mtrr.c"

# ---- Step 3: Compile core kernel C files ----
echo "[3/5] Compiling core kernel..."
cc "$CORE/kernel_main.c"
cc "$CORE/sched.c"
cc "$CORE/hwinfo.c"
cc "$CORE/acpi.c"
cc "$CORE/pmem.c"
cc "$CORE/vm.c"
cc "$CORE/proc.c"
cc "$CORE/elf.c"
cc "$CORE/printf.c"
cc "$CORE/pf.c"
cc "$CORE/kmalloc.c"
cc "$CORE/cache.c"
cc "$CORE/clock.c"
cc "$CORE/signal.c"
cc "$CORE/syscall.c"
cc "$CORE/sysent.c"
cc "$CORE/syshook.c"
cc "$CORE/runtime.c"
# cc "$CORE/syscalls_extended.c"   # needs includes fixed
# cc "$CORE/simd.c"               # needs freestanding compat (skipped)
cc "$CORE/lightshell.c"
cc "$CORE/boot_splash.c"
cc "$CORE/storage.c"
cc "$CORE/sleep.c"
cc "$CORE/vmware.c"
cc "$CORE/smp.c"
cc "$CORE/userinit.c"
cc "$CORE/lightshell_cmds/netget.c"
# cc "$CORE/lightshell_cmds/fileops.c"   # duplicates static fns in lightshell.c, missing header

# ---- Step 3b: Filesystem ----
echo "  FS..."
cc "$FS/boot_fs.c"
cc "$FS/vfs2.c"
cc "$FS/ramfs.c"
cc "$FS/ramfs_vfs.c"
cc "$FS/btrfs.c"
cc "$FS/devfs.c"
cc "$FS/devfs_vfs.c"

# ---- Step 3c: Network ----
echo "  NET..."
cc "$NET/net.c"
cc "$NET/virtionet.c"
cc "$NET/wifi.c"
cc "$NET/dhcp/dhcp.c"
cc "$NET/dns/dns.c"
cc "$NET/http/http.c"

# ---- Step 3d: IPC ----
echo "  IPC..."
cc "$IPC/pipe.c"
cc "$IPC/pipe_vfs.c"

# ---- Step 3e: Drivers (from project-root drivers/) ----
echo "  DRIVERS..."
cc "$DRIVERS/serial.c"
cc "$DRIVERS/block.c"
cc "$DRIVERS/ramdisk.c"
cc "$DRIVERS/rtc.c"
cc "$DRIVERS/ps2kbd.c"
cc "$DRIVERS/tty.c"
cc "$DRIVERS/fb.c"
cc "$DRIVERS/font.c"
cc "$DRIVERS/tui.c"
cc "$DRIVERS/im.c"
cc "$DRIVERS/ahci.c"
cc "$DRIVERS/nvme.c"
cc "$DRIVERS/usb_core.c"
cc "$DRIVERS/uhci.c"
cc "$DRIVERS/usb_hid.c"
cc "$DRIVERS/usb_msc.c"
# cc "$DRIVERS/display.c"        # needs libc (string.h) in freestanding
# cc "$DRIVERS/gpu_hal.c"       # needs libc (strncpy) in freestanding
# cc "$DRIVERS/render.c"        # needs libc math (sinf, cosf, tanf, sqrtf) in freestanding
# cc "$DRIVERS/shader.c"         # needs libc (atof/atoi) in freestanding
# cc "$DRIVERS/vulkan.c"         # needs libc (strcpy) in freestanding

KERNEL_OBJS=$(ls "$BUILD"/*.o)

# ---- Step 4: Link ----
echo "[4/5] Linking..."
$CC -m32 -nostdlib -T "$ARCH/link.ld" -o "$BUILD/kernel.elf" $KERNEL_OBJS $RUST_LIB -lgcc
echo "  kernel.elf: $(stat -c%s "$BUILD/kernel.elf") bytes"

# ---- Step 5: Convert to binary ----
echo "[5/5] Converting to binary and creating floppy..."
objcopy -O binary "$BUILD/kernel.elf" "$BUILD/kernel.bin"
KERNEL_SIZE=$(stat -c%s "$BUILD/kernel.bin")
KERNEL_SECTORS=$(( (KERNEL_SIZE + 511) / 512 ))
echo "  kernel.bin: $KERNEL_SIZE bytes ($KERNEL_SECTORS sectors)"

# Build bootloader with correct sector count
BOOT_SECTORS=$KERNEL_SECTORS
if [ "$BOOT_SECTORS" -gt 2880 ]; then
    BOOT_SECTORS=2880
    echo "  WARNING: Kernel exceeds floppy capacity (2880 sectors)"
fi
nasm -f bin -dKERNEL_SECTORS=$BOOT_SECTORS -o "$BUILD/bootloader.bin" "$ARCH/boot/bootloader.asm"

# Create floppy image
dd if=/dev/zero of="$BUILD/floppy.img" bs=512 count=2880 2>/dev/null
dd if="$BUILD/bootloader.bin" of="$BUILD/floppy.img" conv=notrunc 2>/dev/null
dd if="$BUILD/kernel.bin" of="$BUILD/floppy.img" bs=512 seek=1 conv=notrunc 2>/dev/null

echo ""
echo "=== Done ==="
echo "  floppy.img: $(stat -c%s "$BUILD/floppy.img") bytes ($BOOT_SECTORS kernel sectors)"
echo ""

echo "To run in QEMU:"
echo "  qemu-system-i386 -drive file=build/floppy.img,format=raw -serial stdio"
echo ""
echo "To debug:"
echo "  qemu-system-i386 -drive file=build/floppy.img,format=raw -serial stdio -s -S"
echo "  gdb -ex 'target remote :1234' -ex 'set architecture i386' build/kernel.elf"
