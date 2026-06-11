.global brights_syscall_entry

.extern brights_syscall_dispatch

// syscall entry uses SWAPGS + kernel stack from GS base (offset 0)
// rax = syscall number
// rdi,rsi,rdx,r10,r8,r9 = args
brights_syscall_entry:
  swapgs
  movq %rsp, %r12
  movq %gs:0, %rsp

  pushq %rcx
  pushq %r11
  pushq %rbx
  pushq %rbp
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15

  movq %r10, %rcx
  call brights_syscall_dispatch

  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbp
  popq %rbx
  popq %r11
  popq %rcx

  movq %r12, %rsp
  swapgs
  sysretq
