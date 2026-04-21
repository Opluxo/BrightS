; BrightS i386 Kernel Entry Point
; Entry for 32-bit protected mode

BITS 32

section .text

extern brights_kernel_main
extern brights_i386_gdt_init
extern brights_i386_idt_init
extern brights_i386_paging_init

global brights_i386_start
global brights_i386_kernel_esp

brights_i386_start:
    mov esp, brights_i386_kernel_esp
    
    push ebx
    call brights_i386_gdt_init
    call brights_i386_idt_init
    
    add esp, 4
    
    push 0x100000
    push 0x200000
    call brights_i386_paging_init
    add esp, 8
    
    call brights_kernel_main
    
brights_i386_kernel_esp:
    dd 0x90000