#ifndef BRIGHTS_SIGNAL_H
#define BRIGHTS_SIGNAL_H

#include <stdint.h>

/* Signal numbers (POSIX-ish) */
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1  10
#define SIGSEGV  11
#define SIGUSR2  12
#define SIGPIPE  13
#define SIGALRM  14
#define SIGTERM  15
#define SIGCHLD  17
#define SIGCONT  18
#define SIGSTOP  19
#define SIGTSTP  20
#define SIGWINCH 28

/* Signal action */
typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

#define BRIGHTS_SIGNAL_MAX 32

/* Per-process signal state */
typedef struct {
  uint32_t pending;         /* Pending signal bitmask */
  uint32_t blocked;         /* Blocked signal bitmask */
  sighandler_t handlers[BRIGHTS_SIGNAL_MAX]; /* Signal handlers */
} brights_signal_state_t;

/* Initialize global signal subsystem */
void brights_signal_init(void);

/* Initialize per-process signal state */
void brights_signal_proc_init(brights_signal_state_t *state);

/* Raise a signal to a specific process */
int brights_signal_raise_proc(brights_signal_state_t *state, uint32_t signo);

/* Raise a global signal (legacy) */
int brights_signal_raise(uint32_t signo);

/* Check pending signals for a process (unblocked) */
uint32_t brights_signal_pending_proc(brights_signal_state_t *state);

/* Get global pending (legacy) */
uint32_t brights_signal_pending(void);

/* Consume (clear) a pending signal */
int brights_signal_consume(brights_signal_state_t *state, uint32_t signo);

/* Clear all pending signals */
void brights_signal_clear(brights_signal_state_t *state);

/* Block/unblock signals */
void brights_signal_block(brights_signal_state_t *state, uint32_t mask);
void brights_signal_unblock(brights_signal_state_t *state, uint32_t mask);

/* Set signal handler */
sighandler_t brights_signal_sethandler(brights_signal_state_t *state, uint32_t signo, sighandler_t handler);

/* Get global signal state (for kernel shell) */
brights_signal_state_t *brights_signal_global(void);

#endif
