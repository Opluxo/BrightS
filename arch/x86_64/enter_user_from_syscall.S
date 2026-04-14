.global brights_enter_user_from_syscall

// void brights_enter_user_from_syscall(void *user_rip, void *user_rsp, uint64_t user_cr3)
// Used by exec syscall to return to new user program
brights_enter_user_from_syscall:
    movq %rdi, %rcx        // Save user RIP in RCX (sysretq destination)
    movq %rsi, %rsp        // Set user stack pointer

    movq %rdx, %cr3        // Switch to user page tables

    movq $0x202, %r11      // RFLAGS = IF (interrupt flag)

    swapgs                // Restore kernel GS
    sysretq               // Return to user mode
