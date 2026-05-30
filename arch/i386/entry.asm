; BrightS i386 Kernel Entry Point
; Called from bootloader at 0x10000
; Bootloader GDT: code=0x08, data=0x10

BITS 32

global start
extern brights_arch_entry

section .text.entry
start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    call brights_arch_entry
.halt:
    hlt
    jmp .halt
