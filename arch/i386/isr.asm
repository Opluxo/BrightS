; BrightS i386 ISR stubs
BITS 32

%macro ISR_NOERR 1
global brights_isr_stub_%1
brights_isr_stub_%1:
    push dword 0
    push dword %1
    jmp brights_isr_common
%endmacro

%macro ISR_ERR 1
global brights_isr_stub_%1
brights_isr_stub_%1:
    push dword %1
    jmp brights_isr_common
%endmacro

ISR_NOERR 0
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 128

%macro SAVE_REGS 0
    pushad
%endmacro

%macro RESTORE_REGS 0
    popad
    add esp, 8
%endmacro

extern brights_trap_handler

brights_isr_common:
    pushad

    push esp
    call brights_trap_handler
    add esp, 4

    popad
    add esp, 8
    iret

global brights_isr_stub_default
brights_isr_stub_default:
    push dword 0
    push dword 0
    jmp brights_isr_common
