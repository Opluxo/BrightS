BITS 32
ORG 0x08048000

; ELF header
ehdr:
db 0x7F, 'E', 'L', 'F'  ; magic
db 1                      ; ELFCLASS32
db 1                      ; ELFDATA2LSB
db 1                      ; EV_CURRENT
db 0                      ; ELFOSABI_SYSV
times 8 db 0              ; padding
dw 2                      ; ET_EXEC
dw 3                      ; EM_386
dd 1                      ; EV_CURRENT
dd _start                 ; e_entry
dd phdr - ehdr            ; e_phoff
dd 0                      ; e_shoff
dd 0                      ; e_flags
dw ehdrsz                 ; e_ehsize
dw phdrsz                 ; e_phentsize
dw 1                      ; e_phnum
dw 0                      ; e_shentsize
dw 0                      ; e_shnum
dw 0                      ; e_shstrndx
ehdrsz equ $ - ehdr

; Program header
phdr:
dd 1                      ; PT_LOAD
dd 0                      ; p_offset
dd ehdr                   ; p_vaddr
dd ehdr                   ; p_paddr
dd filesz                 ; p_filesz
dd filesz                 ; p_memsz
dd 5                      ; PF_R | PF_X
dd 0x1000                 ; p_align
phdrsz equ $ - phdr

; Code
_start:
mov eax, 2                ; sys_write
mov ebx, 1                ; fd = 1
mov ecx, msg              ; buf
mov edx, 7                ; len
int 0x80
mov eax, 5                ; sys_exit
xor ebx, ebx              ; status = 0
int 0x80
hlt

msg db "Hello!", 10
filesz equ $ - ehdr
