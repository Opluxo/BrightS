/* Minimal x86_64 user mode test program for BrightS OS */
/* This will be converted to binary and embedded in ramfs */

/* Syscall numbers */
#define SYS_WRITE  2
#define SYS_GETPID 9
#define SYS_EXIT   5

/* File descriptors */
#define STDIN  0
#define STDOUT 1
#define STDERR 2

/* Exit codes */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void _start(void) __attribute__((noreturn));

void _start(void) {
    /* Write "User test: Hello from user mode!\n" to stdout */
    const char *msg1 = "User test: Hello from user mode!\n";
    int len1 = 32;
    
    __asm__ __volatile__(
        "mov $1, %%rax\n\t"        /* syscall number: write */
        "mov %1, %%rdi\n\t"        /* fd: stdout */
        "mov %2, %%rsi\n\t"        /* buffer */
        "mov %3, %%rdx\n\t"        /* length */
        "syscall\n\t"
        : : "r"(SYS_WRITE), "r"(STDOUT), "r"(msg1), "r"(len1)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    
    /* Get PID */
    pid_t pid;
    __asm__ __volatile__(
        "mov $9, %%rax\n\t"        /* syscall number: getpid */
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r"(pid)
        :
        : "rax", "rcx", "r11"
    );
    
    /* Write "PID: X\n" to stdout */
    char msg2[16];
    msg2[0] = 'P';
    msg2[1] = 'I';
    msg2[2] = 'D';
    msg2[3] = ':';
    msg2[4] = ' ';
    msg2[5] = '0' + (pid % 10);
    msg2[6] = '\n';
    msg2[7] = 0;
    
    __asm__ __volatile__(
        "mov $2, %%rax\n\t"        /* syscall number: write */
        "mov $1, %%rdi\n\t"        /* fd: stdout */
        "mov %1, %%rsi\n\t"        /* buffer */
        "mov $7, %%rdx\n\t"        /* length */
        "syscall\n\t"
        : : "r"(SYS_WRITE), "r"(msg2)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    
    /* Exit with success */
    __asm__ __volatile__(
        "mov $5, %%rax\n\t"        /* syscall number: exit */
        "mov $0, %%rdi\n\t"        /* exit code: 0 */
        "syscall\n\t"
        :
        :
        : "rax", "rdi", "rcx", "r11"
    );
    
    /* Should never reach here */
    while(1);
}

typedef int pid_t;
