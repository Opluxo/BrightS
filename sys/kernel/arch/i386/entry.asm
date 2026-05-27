; BrightS i386 Kernel Entry

BITS 32

ORG 0x10000

start:
    mov esp, 0x90000
    
    mov edi, 0xB8000
    mov ah, 0x0F
    mov al, 'K'
    mov [edi], ax

done:
    hlt
    jmp done