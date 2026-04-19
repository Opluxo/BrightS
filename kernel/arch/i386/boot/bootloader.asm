; BrightS i386 BIOS Bootloader v2
; Enhanced bootloader with memory detection and kernel loading

BITS 16

ORG 0x7C00

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    mov si, msg_banner
    call print_string
    
    call detect_memory
    
    mov si, msg_memory
    call print_string
    call print_hex16
    
    call load_kernel
    
    jc boot_error
    
    mov si, msg_loaded
    call print_string
    
    cli
    lgdt [gdtr]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:protected_mode

detect_memory:
    mov eax, 0xE820
    xor ebx, ebx
    mov edx, 0x534D4150
    mov esi, mem_map
    int 0x15
    jc .done
    mov [mem_count], bx
    .done:
    ret

print_string:
    mov ah, 0x0E
    .loop:
    lodsb
    or al, al
    jz .ret
    int 0x10
    jmp .loop
    .ret:
    ret

print_hex16:
    mov cx, 4
    .loop:
    rol ax, 4
    mov dl, al
    and dl, 0x0F
    cmp dl, 0x0A
    jb .digit
    add dl, 'A' - 10
    jmp .show
    .digit:
    add dl, '0'
    .show:
    mov ah, 0x0E
    int 0x10
    loop .loop
    ret

boot_error:
    mov si, msg_error
    call print_string
    jmp $

load_kernel:
    mov ah, 0x02
    mov al, 32
    mov bx, 0x1000
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc .fail
    .fail:
    ret

msg_banner db 'BrightS Bootloader v2', 13, 10, 0
msg_memory db 'Memory: ', 0
msg_loaded db 'Kernel loaded at 0x10000', 13, 10, 0
msg_error db 'Error!', 13, 10, 0

mem_map times 256 db 0
mem_count dw 0

gdtr:
    dw gdt_end - gdt - 1
    dd 0x1000 + (gdt - 0x7C00)

gdt:
    dw 0, 0, 0, 0
    
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF
    
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF
gdt_end:

times 510-($-start) db 0
dw 0xAA55

BITS 32

protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov esp, 0x90000
    
    call 0x10000
    
    jmp $