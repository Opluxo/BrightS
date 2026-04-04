.global brights_isr_stub_0
.global brights_isr_stub_13
.global brights_isr_stub_14
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

ISR_NOERR 0
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 128

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

  movq %rsp, %rdi
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
