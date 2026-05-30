; BrightS i386 Bootloader (Extended INT 13h LBA)
BITS 16
ORG 0x7C00

KERNEL_LOAD_SEG equ 0x1000
KERNEL_DEST     equ 0x10000

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl

    mov si, msg_boot
    call puts

    mov word [load_seg], KERNEL_LOAD_SEG

read_loop:
    cmp word [sectors_left], 0
    je load_done

    mov ax, [lba_cur]
    mov [dap_lba], ax
    mov word [dap_lba+2], 0
    mov word [dap_lba+4], 0
    mov word [dap_lba+6], 0

    mov ax, [load_seg]
    mov word [dap_seg], ax
    mov word [dap_off], 0
    mov word [dap_sects], 1

    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jnc read_ok

    push ax
    mov si, msg_err
    call puts
    pop ax
    mov al, ah
    call puthex
    jmp $

read_ok:
    add word [load_seg], 0x20
    add word [lba_cur], 1
    sub word [sectors_left], 1
    jmp read_loop

load_done:
    mov si, msg_ok
    call puts

    xor ax, ax
    mov es, ax
    mov ds, ax

    cli
    lgdt [gdt_desc]

    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:KERNEL_DEST

msg_boot     db 'B', 0
msg_ok       db 'O', 0
msg_err      db '!', 0
boot_drive   db 0
lba_cur      dw 1
sectors_left dw KERNEL_SECTORS
load_seg     dw KERNEL_LOAD_SEG

ALIGN 4
dap:
    db 0x10
    db 0
dap_sects dw 1
dap_off   dw 0
dap_seg   dw KERNEL_LOAD_SEG
dap_lba   dq 0

ALIGN 4
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

gdt:
    db 0, 0, 0, 0, 0, 0, 0, 0
    db 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0
    db 0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0
gdt_end:

puts:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

puthex:
    push ax
    push cx
    mov cl, 4
    shr al, cl
    cmp al, 10
    jb .low
    add al, 'A' - 10
    jmp .print
.low:
    add al, '0'
.print:
    mov ah, 0x0E
    int 0x10
    pop cx
    pop ax
    and al, 0x0F
    cmp al, 10
    jb .low2
    add al, 'A' - 10
    jmp .print2
.low2:
    add al, '0'
.print2:
    mov ah, 0x0E
    int 0x10
    ret

times 510-($-start) db 0
dw 0xAA55
