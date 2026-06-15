.global brights_enter_user

// void brights_enter_user(void *user_rip, void *user_rsp, uint64_t user_cr3)
brights_enter_user:
  // rdi = user_rip, rsi = user_rsp, rdx = user_cr3
  movq %rdx, %cr3            // Switch to user page tables
  pushq $0x23                // SS = user DS selector (0x20 | 3) = 0x23
  pushq %rsi                 // RSP = user stack pointer
  pushfq                     // RFLAGS
  pushq $0x1B                // CS = user CS selector (0x18 | 3) = 0x1B
  pushq %rdi                 // RIP = user entry point
  iretq
