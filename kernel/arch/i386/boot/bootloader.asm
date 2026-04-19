; BrightS i386 BIOS Bootloader
; This bootloader loads the kernel from disk and starts it in protected mode

BITS 16

ORG 0x7C00

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    mov si, msg_loading
    call print_string
    
    mov ah, 0x02
    mov al, 1
    mov bx, 0x1000
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    int 0x13
    jc disk_error
    
    mov si, msg_ok
    call print_string
    
    cli
    lgdt [gdtr]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:protected_mode

print_string:
    mov ah, 0x0E
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp print_string
    .done:
    ret

disk_error:
    mov si, msg_error
    call print_string
    jmp $

msg_loading db 'Loading BrightS...', 13, 10, 0
msg_ok db 'OK', 13, 10, 0
msg_error db 'Disk error!', 13, 10, 0

gdtr:
    dw gdt_end - gdt - 1
    dd 0x1000 + (gdt - start)

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
    
    jmp 0x1000