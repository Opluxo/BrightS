set(KERNEL_EFI ${CMAKE_CURRENT_BINARY_DIR}/kernel.efi)

if(UEFI_CLANG)
  find_program(CLANG_BIN clang)
  find_program(LLD_LINK lld-link)
  if(NOT CLANG_BIN OR NOT LLD_LINK)
    message(FATAL_ERROR "clang and lld-link are required for UEFI build")
  endif()

  add_custom_command(
    OUTPUT ${KERNEL_EFI}
    COMMAND ${CLANG_BIN}
      -target x86_64-unknown-windows
      -ffreestanding -fno-stack-protector -fno-pic
      -Wl,/subsystem:efi_application
      -Wl,/entry:efi_main
      -Wl,/nodefaultlib
      -Wl,/base:0
      -Wl,/align:4096
      -fuse-ld=lld
      -I${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/uefi_entry.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/idt.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/trap.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/isr.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/pci.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/gdt.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/tss.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/cpu_local.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/syscall_entry.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/user_enter.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/user_stub.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/syscall_abi.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/uefi_memmap.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/paging.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/serial.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/disk.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/block.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/ramdisk.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/rtc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/ps2kbd.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/tty.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/ahci.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/nvme.c
      ${CMAKE_CURRENT_SOURCE_DIR}/ipc/pipe.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/clock.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/acpi.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/hwinfo.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/kmalloc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/kshell.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/proc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/runtime.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/sched.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/signal.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/sleep.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/storage.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syscall.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/sysent.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/kernel_main.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/userinit.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/pmem.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/vm.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/printf.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/vfs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/vnode.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/namei.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/ramfs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/btrfs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/btrfs_btree.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/v6fs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/inode.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/alloc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/devfs.c
      -o ${KERNEL_EFI}
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/uefi_entry.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/idt.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/trap.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/isr.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/pci.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/gdt.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/tss.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/cpu_local.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/syscall_entry.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/user_enter.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/user_stub.S
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/syscall_abi.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/uefi_memmap.c
      ${CMAKE_CURRENT_SOURCE_DIR}/platform/x86_64/paging.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/serial.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/disk.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/block.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/ramdisk.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/rtc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/ps2kbd.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/tty.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/ahci.c
      ${CMAKE_CURRENT_SOURCE_DIR}/dev/nvme.c
      ${CMAKE_CURRENT_SOURCE_DIR}/ipc/pipe.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/clock.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/acpi.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/hwinfo.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/kmalloc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/kshell.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/proc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/runtime.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/sched.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/signal.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/sleep.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/storage.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/syscall.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/sysent.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/kernel_main.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/userinit.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/pmem.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/vm.c
      ${CMAKE_CURRENT_SOURCE_DIR}/core/printf.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/vfs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/vnode.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/namei.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/ramfs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/btrfs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/v6fs.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/inode.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/alloc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/fs/devfs.c
    VERBATIM
  )

  add_custom_target(kernel_efi ALL DEPENDS ${KERNEL_EFI})
else()
  message(STATUS "UEFI_CLANG=OFF: kernel_efi target disabled")
endif()
