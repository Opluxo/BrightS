BITS 16
ORG 0x7C00

KERNEL_SECTORS equ 359
KERNEL_LOAD_SEG equ 0x1000

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl

    mov si, msg_boot
    call puts

    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    xor bx, bx

    mov word [lba_cur], 1
    mov word [sectors_left], KERNEL_SECTORS

.read_loop:
    cmp word [sectors_left], 0
    je .load_done

    mov ax, [lba_cur]
    xor dx, dx
    mov cx, 36
    div cx
    mov ch, al

    xchg ax, dx
    mov bl, 18
    div bl
    mov dh, al
    mov cl, ah
    add cl, 1

    mov [head_save], dh
    mov [cyl_sec], cx

    mov al, 19
    sub al, cl

    push dx
    mov dx, [sectors_left]
    cmp dx, ax
    jae .count_ok
    mov ax, dx
.count_ok:
    mov byte [sec_count], al
    pop dx

    ; Reset disk before each read (for head/cylinder changes)
    mov dl, [boot_drive]
    xor ax, ax
    int 0x13

    mov dl, [boot_drive]
    mov dh, [head_save]
    mov cx, [cyl_sec]
    mov ah, 0x02
    mov al, [sec_count]
    int 0x13
    jnc .read_ok

    xor ax, ax
    int 0x13
    mov dh, [head_save]
    mov cx, [cyl_sec]
    mov dl, [boot_drive]
    mov ah, 0x02
    mov al, [sec_count]
    int 0x13
    jnc .read_ok

    mov si, msg_err
    call puts
    push ax
    mov al, [lba_cur]
    call puthex
    pop ax
    jmp $

.read_ok:
    xor ch, ch
    mov cl, [sec_count]
    shl cx, 9
    add bx, cx

    xor ch, ch
    mov cl, [sec_count]
    add [lba_cur], cx
    sub [sectors_left], cx

    jmp .read_loop

.load_done:
    mov si, msg_ok
    call puts
    jmp $

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

msg_boot   db 'B', 0
msg_ok     db 'O', 0
msg_err    db '!', 0
boot_drive db 0
lba_cur      dw 0
sectors_left dw 0
sec_count    db 0
head_save    db 0
cyl_sec      dw 0

times 510-($-start) db 0
dw 0xAA55
