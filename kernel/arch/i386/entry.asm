; BrightS i386 Kernel - Visible Flash

BITS 32

ORG 0x10000

start:
    mov esp, 0x90000
    mov edi, 0xB8000
    
    xor bl, bl
    
.loop:
    ; Alternate between 'B' and 'O'
    test bl, 1
    jz .show_B
    
.show_O:
    mov ah, 0x00
    mov al, ' '
    jmp .write
    
.show_B:
    mov ah, 0x0F
    mov al, 'B'
    
.write:
    mov [edi], ax
    
    ; Toggle
    xor bl, 1
    
    ; Delay
    push ecx
    mov ecx, 0x40000
.d1:
    loop .d1
    pop ecx
    
    jmp .loop