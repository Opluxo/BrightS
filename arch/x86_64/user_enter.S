.global brights_enter_user

// void brights_enter_user(void *user_rip, void *user_rsp, uint64_t user_cr3)
brights_enter_user:
  // rdi = user_rip, rsi = user_rsp, rdx = user_cr3
  movq %rdx, %cr3            // Switch to user page tables
  pushq $0x23                // SS = user DS selector (0x20 | 3)
  pushq %rsi                 // RSP = user stack pointer
  pushfq                     // RFLAGS
  pushq $0x2B                // CS = user CS selector (0x28 | 3)
  pushq %rdi                 // RIP = user entry point
  iretq
