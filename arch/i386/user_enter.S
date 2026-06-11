; BrightS i386 user mode entry
BITS 32

global brights_enter_user

; void brights_enter_user(void *user_eip, void *user_esp, uint32_t user_cr3)
brights_enter_user:
    mov eax, [esp + 4]    ; user_eip
    mov ecx, [esp + 8]    ; user_esp
    mov edx, [esp + 12]   ; user_cr3

    mov cr3, edx          ; Switch to user page tables

    push dword 0x1B       ; SS = user DS (0x18 | 3)
    push ecx              ; ESP = user stack pointer
    pushfd                ; EFLAGS
    push dword 0x23       ; CS = user CS (0x20 | 3)
    push eax              ; EIP = user entry point
    iret
