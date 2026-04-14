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
    ${CMAKE_SOURCE_DIR}/drivers/ahci.c
    ${CMAKE_SOURCE_DIR}/drivers/nvme.c
    ${CMAKE_SOURCE_DIR}/kernel/pipe.c
    ${CMAKE_SOURCE_DIR}/kernel/clock.c
    ${CMAKE_SOURCE_DIR}/kernel/acpi.c
    ${CMAKE_SOURCE_DIR}/kernel/hwinfo.c
    ${CMAKE_SOURCE_DIR}/kernel/kmalloc.c
    ${CMAKE_SOURCE_DIR}/kernel/lightshell.c
    ${CMAKE_SOURCE_DIR}/kernel/lightshell_cmds/netget.c
    ${CMAKE_SOURCE_DIR}/kernel/proc.c
    ${CMAKE_SOURCE_DIR}/kernel/runtime.c
    ${CMAKE_SOURCE_DIR}/kernel/sched.c
    ${CMAKE_SOURCE_DIR}/kernel/signal.c
    ${CMAKE_SOURCE_DIR}/kernel/syshook.c
    ${CMAKE_SOURCE_DIR}/kernel/sleep.c
    ${CMAKE_SOURCE_DIR}/kernel/storage.c
    ${CMAKE_SOURCE_DIR}/kernel/syscall.c
    ${CMAKE_SOURCE_DIR}/kernel/sysent.c
    ${CMAKE_SOURCE_DIR}/kernel/kernel_main.c
    ${CMAKE_SOURCE_DIR}/kernel/userinit.c
    ${CMAKE_SOURCE_DIR}/kernel/elf.c
    ${CMAKE_SOURCE_DIR}/kernel/smp.c
    ${CMAKE_SOURCE_DIR}/kernel/pmem.c
    ${CMAKE_SOURCE_DIR}/kernel/vm.c
    ${CMAKE_SOURCE_DIR}/kernel/printf.c
    ${CMAKE_SOURCE_DIR}/kernel/pf.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/vfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/vfs2.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/ramfs_vfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/btrfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/devfs.c
    ${CMAKE_SOURCE_DIR}/kernel/fs/devfs_vfs.c
    ${CMAKE_SOURCE_DIR}/kernel/net/net.c
    ${CMAKE_SOURCE_DIR}/kernel/net/wifi.c
    ${CMAKE_SOURCE_DIR}/kernel/net/dhcp/dhcp.c
    ${CMAKE_SOURCE_DIR}/kernel/net/dns/dns.c
    ${CMAKE_SOURCE_DIR}/kernel/net/http/http.c
  )

  add_custom_command(
    OUTPUT ${KERNEL_EFI}
     COMMAND ${CLANG_BIN}
       -target x86_64-unknown-windows
       -ffreestanding -fno-stack-protector -fno-pic
        -Os -fomit-frame-pointer -mno-red-zone
        -march=bonnell -mtune=bonnell -mssse3 -mno-sse4 -mno-avx
        -falign-functions=16 -falign-loops=16 -falign-jumps=16
        -ffunction-sections -fdata-sections -flto
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
       -I${CMAKE_SOURCE_DIR}/arch/x86_64
        -I${CMAKE_SOURCE_DIR}/kernel
        -I${CMAKE_SOURCE_DIR}/kernel/net
        -I${CMAKE_SOURCE_DIR}/kernel/net/dhcp
        -I${CMAKE_SOURCE_DIR}/kernel/net/dns
        -I${CMAKE_SOURCE_DIR}/kernel/net/http
        -I${CMAKE_SOURCE_DIR}/drivers
        -I${CMAKE_SOURCE_DIR}/include
       ${KERNEL_SOURCES}
      -o ${KERNEL_EFI}
    DEPENDS ${KERNEL_SOURCES}
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
      COMMAND ${QEMU_BIN}
        -bios /usr/share/edk2/x64/OVMF.4m.fd
        -drive file=fat:rw:${CMAKE_CURRENT_BINARY_DIR},format=raw
        -net none
        -serial stdio
        -no-reboot
        -no-shutdown
        -m 512
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
