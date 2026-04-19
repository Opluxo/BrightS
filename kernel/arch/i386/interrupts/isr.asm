; BrightS i386 Interrupt Service Routines
; Assembly code for ISR handlers

BITS 32

section .text

global brights_i386_isr0
global brights_i386_isr1
global brights_i386_isr2
global brights_i386_isr3
global brights_i386_isr4
global brights_i386_isr5
global brights_i386_isr6
global brights_i386_isr7
global brights_i386_isr8
global brights_i386_isr9
global brights_i386_isr10
global brights_i386_isr11
global brights_i386_isr12
global brights_i386_isr13
global brights_i386_isr14
global brights_i386_isr15
global brights_i386_isr16
global brights_i386_isr17
global brights_i386_isr18
global brights_i386_isr19
global brights_i386_isr20
global brights_i386_isr21
global brights_i386_isr22
global brights_i386_isr23
global brights_i386_isr24
global brights_i386_isr25
global brights_i386_isr26
global brights_i386_isr27
global brights_i386_isr28
global brights_i386_isr29
global brights_i386_isr30
global brights_i386_isr31

global brights_i386_irq0
global brights_i386_irq1
global brights_i386_irq2
global brights_i386_irq3
global brights_i386_irq4
global brights_i386_irq5
global brights_i386_irq6
global brights_i386_irq7
global brights_i386_irq8
global brights_i386_irq9
global brights_i386_irq10
global brights_i386_irq11
global brights_i386_irq12
global brights_i386_irq13
global brights_i386_irq14
global brights_i386_irq15

brights_i386_isr0:
    cli
    push 0
    jmp common_isr

brights_i386_isr1:
    cli
    push 0
    jmp common_isr

brights_i386_isr2:
    cli
    push 0
    jmp common_isr

brights_i386_isr3:
    cli
    push 0
    jmp common_isr

brights_i386_isr4:
    cli
    push 0
    jmp common_isr

brights_i386_isr5:
    cli
    push 0
    jmp common_isr

brights_i386_isr6:
    cli
    push 0
    jmp common_isr

brights_i386_isr7:
    cli
    push 0
    jmp common_isr

brights_i386_isr8:
    cli
    push 0
    jmp common_isr

brights_i386_isr9:
    cli
    push 0
    jmp common_isr

brights_i386_isr10:
    cli
    push 0
    jmp common_isr

brights_i386_isr11:
    cli
    push 0
    jmp common_isr

brights_i386_isr12:
    cli
    push 0
    jmp common_isr

brights_i386_isr13:
    cli
    push 0
    jmp common_isr

brights_i386_isr14:
    cli
    push 0
    jmp common_isr

brights_i386_isr15:
    cli
    push 0
    jmp common_isr

brights_i386_isr16:
    cli
    push 0
    jmp common_isr

brights_i386_isr17:
    cli
    push 0
    jmp common_isr

brights_i386_isr18:
    cli
    push 0
    jmp common_isr

brights_i386_isr19:
    cli
    push 0
    jmp common_isr

brights_i386_isr20:
    cli
    push 0
    jmp common_isr

brights_i386_isr21:
    cli
    push 0
    jmp common_isr

brights_i386_isr22:
    cli
    push 0
    jmp common_isr

brights_i386_isr23:
    cli
    push 0
    jmp common_isr

brights_i386_isr24:
    cli
    push 0
    jmp common_isr

brights_i386_isr25:
    cli
    push 0
    jmp common_isr

brights_i386_isr26:
    cli
    push 0
    jmp common_isr

brights_i386_isr27:
    cli
    push 0
    jmp common_isr

brights_i386_isr28:
    cli
    push 0
    jmp common_isr

brights_i386_isr29:
    cli
    push 0
    jmp common_isr

brights_i386_isr30:
    cli
    push 0
    jmp common_isr

brights_i386_isr31:
    cli
    push 0
    jmp common_isr

brights_i386_irq0:
    cli
    push 0
    jmp common_irq

brights_i386_irq1:
    cli
    push 0
    jmp common_irq

brights_i386_irq2:
    cli
    push 0
    jmp common_irq

brights_i386_irq3:
    cli
    push 0
    jmp common_irq

brights_i386_irq4:
    cli
    push 0
    jmp common_irq

brights_i386_irq5:
    cli
    push 0
    jmp common_irq

brights_i386_irq6:
    cli
    push 0
    jmp common_irq

brights_i386_irq7:
    cli
    push 0
    jmp common_irq

brights_i386_irq8:
    cli
    push 0
    jmp common_irq

brights_i386_irq9:
    cli
    push 0
    jmp common_irq

brights_i386_irq10:
    cli
    push 0
    jmp common_irq

brights_i386_irq11:
    cli
    push 0
    jmp common_irq

brights_i386_irq12:
    cli
    push 0
    jmp common_irq

brights_i386_irq13:
    cli
    push 0
    jmp common_irq

brights_i386_irq14:
    cli
    push 0
    jmp common_irq

brights_i386_irq15:
    cli
    push 0
    jmp common_irq

common_isr:
    push gs
    push fs
    push es
    push ds
    push eax
    push ebx
    push ecx
    push edx
    push ebp
    push esi
    push edi
    mov ebp, esp
    and esp, 0xFFFFFFF0
    call brights_i386_exception_handler
    mov esp, ebp
    pop edi
    pop esi
    pop ebp
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8
    iret

common_irq:
    push gs
    push fs
    push es
    push ds
    push eax
    push ebx
    push ecx
    push edx
    push ebp
    push esi
    push edi
    mov ebp, esp
    and esp, 0xFFFFFFF0
    call brights_i386_interrupt_handler
    mov esp, ebp
    pop edi
    pop esi
    pop ebp
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ds
    pop es
    pop fs
    pop gs
    add esp, 8
    iret