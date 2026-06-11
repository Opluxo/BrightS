.global brights_user_start

brights_user_start:
  movq $2, %rax
  movq $0, %rdi
  lea user_msg(%rip), %rsi
  movq $25, %rdx
  syscall

user_loop:
  jmp user_loop

.section .rodata
user_msg:
  .ascii "user: syscall write ok\n"
