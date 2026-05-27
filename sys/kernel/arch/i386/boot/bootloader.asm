; BrightS i386 BIOS Bootloader

BITS 16

ORG 0x7C00

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    mov si, msg_boot
    call puts
    
    mov bx, 0x1000
    mov ah, 0x02
    mov al, 18
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0
    int 0x13
    
    jnc .loaded
    mov si, msg_err
    call puts
    jmp $

.loaded:
    mov si, msg_ok
    call puts
    
    mov si, 0x1000
    mov di, 0x10000
    mov cx, 9*512
    rep movsb
    
    cli
    lgdt [gdt_desc]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

puts:
    mov ah, 0x0E
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp puts
.done:
    ret

msg_boot db 'BrightS i386', 10, 0
msg_ok db 'OK', 10, 0
msg_err db '!', 10, 0

gdt_desc:
    dw gdt_end - gdt - 1
    dw gdt
    dd 0

gdt:
    db 0, 0, 0, 0
    db 0, 0, 0, 0
    db 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0
    db 0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0
gdt_end:

times 510-($-start) db 0
dw 0xAA55

BITS 32

pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    mov edi, 0xB8000
    mov ah, 0x0F
    mov al, 'B'
    mov [edi], ax
    
    jmp 0x10000