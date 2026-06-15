.global brights_isr_stub_0
.global brights_isr_stub_1
.global brights_isr_stub_2
.global brights_isr_stub_3
.global brights_isr_stub_4
.global brights_isr_stub_5
.global brights_isr_stub_6
.global brights_isr_stub_7
.global brights_isr_stub_8
.global brights_isr_stub_9
.global brights_isr_stub_10
.global brights_isr_stub_11
.global brights_isr_stub_12
.global brights_isr_stub_13
.global brights_isr_stub_14
.global brights_isr_stub_15
.global brights_isr_stub_16
.global brights_isr_stub_17
.global brights_isr_stub_18
.global brights_isr_stub_19
.global brights_isr_stub_20
.global brights_isr_stub_21
.global brights_isr_stub_32
.global brights_isr_stub_33
.global brights_isr_stub_128

.extern brights_trap_handler

.macro ISR_NOERR vec
brights_isr_stub_\vec:
  pushq $0
  pushq $\vec
  jmp brights_isr_common
.endm

.macro ISR_ERR vec
brights_isr_stub_\vec:
  pushq $\vec
  jmp brights_isr_common
.endm

/* CPU Exceptions 0-21 (Intel reserved) */
ISR_NOERR 0   /* #DE  Divide Error */
ISR_NOERR 1   /* #DB  Debug */
ISR_NOERR 2   /*      NMI */
ISR_NOERR 3   /* #BP  Breakpoint */
ISR_NOERR 4   /* #OF  Overflow */
ISR_NOERR 5   /* #BR  Bound Range Exceeded */
ISR_NOERR 6   /* #UD  Invalid Opcode */
ISR_NOERR 7   /* #NM  Device Not Available */
ISR_ERR   8   /* #DF  Double Fault */
ISR_NOERR 9   /*      Coprocessor Segment Overrun (reserved) */
ISR_ERR   10  /* #TS  Invalid TSS */
ISR_ERR   11  /* #NP  Segment Not Present */
ISR_ERR   12  /* #SS  Stack-Segment Fault */
ISR_ERR   13  /* #GP  General Protection */
ISR_ERR   14  /* #PF  Page Fault */
ISR_NOERR 15  /*      reserved */
ISR_NOERR 16  /* #MF  x87 FPU Floating-Point Error */
ISR_ERR   17  /* #AC  Alignment Check */
ISR_NOERR 18  /* #MC  Machine Check */
ISR_NOERR 19  /* #XM  SIMD Floating-Point Exception */
ISR_NOERR 20  /* #VE  Virtualization Exception */
ISR_NOERR 21  /*      reserved */

/* Hardware IRQs */
ISR_NOERR 32  /* IRQ0  PIT timer */
ISR_NOERR 33  /* IRQ1  Keyboard */

/* Syscall */
ISR_NOERR 128 /* int 0x80 */

brights_isr_common:
  pushq %rax
  pushq %rbx
  pushq %rcx
  pushq %rdx
  pushq %rsi
  pushq %rdi
  pushq %rbp
  pushq %r8
  pushq %r9
  pushq %r10
  pushq %r11
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15

  movq %rsp, %rcx
  call brights_trap_handler

  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %r11
  popq %r10
  popq %r9
  popq %r8
  popq %rbp
  popq %rdi
  popq %rsi
  popq %rdx
  popq %rcx
  popq %rbx
  popq %rax

  addq $16, %rsp
  iretq
