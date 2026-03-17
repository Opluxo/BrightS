.global brights_enter_user

// void brights_enter_user(void *user_rip, void *user_rsp)
brights_enter_user:
  // rdi = user_rip, rsi = user_rsp
  pushq $0x23
  pushq %rsi
  pushfq
  pushq $0x1B
  pushq %rdi
  iretq
