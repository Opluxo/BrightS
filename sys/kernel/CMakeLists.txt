set(KERNEL_EFI ${CMAKE_CURRENT_BINARY_DIR}/kernel.efi)

if(UEFI_CLANG)
  find_program(CLANG_BIN clang)
  find_program(LLD_LINK lld-link)
  if(NOT CLANG_BIN OR NOT LLD_LINK)
    message(FATAL_ERROR "clang and lld-link are required for UEFI build")
  endif()

set(KERNEL_SOURCES
    ${CMAKE_SOURCE_DIR}/arch/x86_64/uefi_entry.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/idt.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/trap.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/isr.S
    ${CMAKE_SOURCE_DIR}/arch/x86_64/pci.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/gdt.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/cpu_local.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/syscall_entry.S
    ${CMAKE_SOURCE_DIR}/arch/x86_64/user_enter.S
    ${CMAKE_SOURCE_DIR}/arch/x86_64/user_stub.S
    ${CMAKE_SOURCE_DIR}/arch/x86_64/enter_user_from_syscall.S
    ${CMAKE_SOURCE_DIR}/arch/x86_64/syscall_abi.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/uefi_memmap.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/paging.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/pic_pit.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/apic.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/ioapic.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/hpet.c
    ${CMAKE_SOURCE_DIR}/arch/x86_64/mtrr.c
    ${CMAKE_SOURCE_DIR}/drivers/serial.c
    ${CMAKE_SOURCE_DIR}/drivers/block.c
    ${CMAKE_SOURCE_DIR}/drivers/ramdisk.c
    ${CMAKE_SOURCE_DIR}/drivers/rtc.c
    ${CMAKE_SOURCE_DIR}/drivers/ps2kbd.c
    ${CMAKE_SOURCE_DIR}/drivers/tty.c
    ${CMAKE_SOURCE_DIR}/drivers/fb.c
    ${CMAKE_SOURCE_DIR}/drivers/font.c
    ${CMAKE_SOURCE_DIR}/drivers/tui.c
    ${CMAKE_SOURCE_DIR}/drivers/theme.c
    ${CMAKE_SOURCE_DIR}/drivers/im.c
    ${CMAKE_SOURCE_DIR}/drivers/ahci.c
    ${CMAKE_SOURCE_DIR}/drivers/nvme.c
    ${CMAKE_SOURCE_DIR}/drivers/usb_core.c
    ${CMAKE_SOURCE_DIR}/drivers/uhci.c
    ${CMAKE_SOURCE_DIR}/drivers/usb_hid.c
    ${CMAKE_SOURCE_DIR}/drivers/usb_msc.c
    ${CMAKE_SOURCE_DIR}/kernel/ipc/pipe.c
    ${CMAKE_SOURCE_DIR}/kernel/ipc/pipe_vfs.c
    ${CMAKE_SOURCE_DIR}/kernel/core/clock.c
    ${CMAKE_SOURCE_DIR}/kernel/core/acpi.c
    ${CMAKE_SOURCE_DIR}/kernel/core/hwinfo.c
    ${CMAKE_SOURCE_DIR}/kernel/core/kmalloc.c
    ${CMAKE_SOURCE_DIR}/kernel/core/cache.c
    ${CMAKE_SOURCE_DIR}/kernel/core/lightshell.c
    ${CMAKE_SOURCE_DIR}/kernel/core/boot_splash.c
    ${CMAKE_SOURCE_DIR}/kernel/core/lightshell_cmds/netget.c
    ${CMAKE_SOURCE_DIR}/kernel/core/proc.c
    ${CMAKE_SOURCE_DIR}/kernel/core/runtime.c
    ${CMAKE_SOURCE_DIR}/kernel/core/vmware.c
    ${CMAKE_SOURCE_DIR}/kernel/core/sched.c
    ${CMAKE_SOURCE_DIR}/kernel/core/signal.c
    ${CMAKE_SOURCE_DIR}/kernel/core/syshook.c
    ${CMAKE_SOURCE_DIR}/kernel/core/sleep.c
    ${CMAKE_SOURCE_DIR}/kernel/core/storage.c
    ${CMAKE_SOURCE_DIR}/kernel/core/syscall.c
    ${CMAKE_SOURCE_DIR}/kernel/core/sysent.c
    ${CMAKE_SOURCE_DIR}/kernel/core/kernel_main.c
    ${CMAKE_SOURCE_DIR}/kernel/core/userinit.c
    ${CMAKE_SOURCE_DIR}/kernel/core/elf.c
    ${CMAKE_SOURCE_DIR}/kernel/core/smp.c
    ${CMAKE_SOURCE_DIR}/kernel/core/pmem.c
    ${CMAKE_SOURCE_DIR}/kernel/core/vm.c
    ${CMAKE_SOURCE_DIR}/kernel/core/printf.c
    ${CMAKE_SOURCE_DIR}/kernel/core/pf.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/vfs2.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/boot_fs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs_vfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/btrfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/devfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/devfs_vfs.c
    ${CMAKE_SOURCE_DIR}/kernel/net/net.c
    ${CMAKE_SOURCE_DIR}/kernel/net/virtionet.c
    ${CMAKE_SOURCE_DIR}/kernel/net/wifi.c
    ${CMAKE_SOURCE_DIR}/kernel/net/dhcp/dhcp.c
    ${CMAKE_SOURCE_DIR}/kernel/net/dns/dns.c
    ${CMAKE_SOURCE_DIR}/kernel/net/http/http.c
  )

  set(RUST_KERNEL_SRC ${CMAKE_SOURCE_DIR}/kernel/rust)
  set(RUST_KERNEL_LIB ${RUST_KERNEL_SRC}/target/x86_64-unknown-none/release/librust_kernel.a)

  set(RUST_LINK "")
  find_program(CARGO_BIN cargo)
  if(CARGO_BIN)
    add_custom_command(
      OUTPUT ${RUST_KERNEL_LIB}
      COMMAND cargo build --release
      WORKING_DIRECTORY ${RUST_KERNEL_SRC}
      COMMENT "Building Rust kernel module"
    )
    set(RUST_LINK ${RUST_KERNEL_LIB})
  endif()

  add_custom_command(
    OUTPUT ${KERNEL_EFI}
     COMMAND ${CLANG_BIN}
       -target x86_64-unknown-windows
       -ffreestanding -fno-stack-protector -fno-pic
        -Os -fomit-frame-pointer -mno-red-zone
        -march=bonnell -mtune=bonnell -mssse3 -mno-sse4 -mno-avx -mno-movbe
        -falign-functions=16 -falign-loops=16
         -ffunction-sections -fdata-sections
        -fno-tree-vectorize
       -Wl,/subsystem:efi_application
       -Wl,/entry:efi_main
       -Wl,/nodefaultlib
       -Wl,/base:0
       -Wl,/align:4096
       -Wl,/opt:ref
       -Wl,/opt:icf
       -fuse-ld=lld
      -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes
      -Wmissing-prototypes -Wno-unused-parameter -Wno-pointer-sign
      -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-cast-qual
        -I${CMAKE_SOURCE_DIR}
         -I${CMAKE_SOURCE_DIR}/arch/x86_64
         -I${CMAKE_SOURCE_DIR}/kernel
        -I${CMAKE_SOURCE_DIR}/kernel/core
        -I${CMAKE_SOURCE_DIR}/kernel/dev
        -I${CMAKE_SOURCE_DIR}/kernel/fs
        -I${CMAKE_SOURCE_DIR}/kernel/ipc
        -I${CMAKE_SOURCE_DIR}/kernel/net
        -I${CMAKE_SOURCE_DIR}/kernel/net/dhcp
        -I${CMAKE_SOURCE_DIR}/kernel/net/dns
        -I${CMAKE_SOURCE_DIR}/kernel/net/http
         -I${CMAKE_SOURCE_DIR}/drivers
         -I${CMAKE_SOURCE_DIR}/include
         -I${CMAKE_SOURCE_DIR}/include/kernel
        ${KERNEL_SOURCES}
        ${RUST_LINK}
      -o ${KERNEL_EFI}
    DEPENDS ${KERNEL_SOURCES} ${RUST_LINK}
    VERBATIM
  )

  add_custom_target(kernel_efi ALL DEPENDS ${KERNEL_EFI})

  # Find QEMU
  find_program(QEMU_BIN qemu-system-x86_64)
  if(QEMU_BIN)
    # Create EFI boot directory structure
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/EFI/BOOT/BOOTX64.EFI
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/EFI/BOOT
      COMMAND ${CMAKE_COMMAND} -E copy ${KERNEL_EFI} ${CMAKE_CURRENT_BINARY_DIR}/EFI/BOOT/BOOTX64.EFI
      DEPENDS ${KERNEL_EFI}
      VERBATIM
    )

    add_custom_target(efi_boot ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/EFI/BOOT/BOOTX64.EFI)

    # Run target - direct EFI execution
    add_custom_target(run
      COMMAND ${CMAKE_COMMAND} -E copy /usr/share/edk2/x64/OVMF_VARS.4m.fd /tmp/brights-ovmf-vars.fd
      COMMAND ${QEMU_BIN}
        -machine q35
        -m 1G
        -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd
        -drive if=pflash,format=raw,file=/tmp/brights-ovmf-vars.fd
        -drive file=fat:rw:${CMAKE_CURRENT_BINARY_DIR},format=raw
        -net none
        -vga std
        -display gtk
        -serial stdio
        -no-reboot
        -no-shutdown
      DEPENDS efi_boot
      USES_TERMINAL
      VERBATIM
    )

    # Debug run target
    add_custom_target(run-debug
      COMMAND ${QEMU_BIN}
        -bios /usr/share/edk2/x64/OVMF.4m.fd
        -drive file=fat:rw:${CMAKE_CURRENT_BINARY_DIR},format=raw
        -net none
        -serial stdio
        -no-reboot
        -no-shutdown
        -s -S
        -m 512
      DEPENDS efi_boot
      USES_TERMINAL
      VERBATIM
    )
  else()
    message(STATUS "QEMU not found - run targets disabled")
  endif()
else()
  message(STATUS "UEFI_CLANG=OFF: kernel_efi target disabled")
endif()
