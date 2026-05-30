#include "signal.h"
#include <stdint.h>

/* Global fallback signal state (for kernel processes) */
static brights_signal_state_t global_signal_state;

void brights_signal_init(void)
{
  brights_signal_proc_init(&global_signal_state);
}

void brights_signal_proc_init(brights_signal_state_t *state)
{
  if (!state) return;
  state->pending = 0;
  state->blocked = 0;
  for (int i = 0; i < BRIGHTS_SIGNAL_MAX; ++i) {
    state->handlers[i] = SIG_DFL;
    state->sigactions[i].sa_handler = SIG_DFL;
    state->sigactions[i].sa_flags = 0;
    state->sigactions[i].sa_restorer = 0;
    state->sigactions[i].sa_mask = 0;
  }
}

int brights_signal_raise_proc(brights_signal_state_t *state, uint32_t signo)
{
  if (!state || signo == 0 || signo >= BRIGHTS_SIGNAL_MAX) return -1;
  
  if (signo == SIGKILL || signo == SIGSTOP) {
    return -1;
  }
  
  state->pending |= (1u << signo);
  return 0;
}

int brights_signal_raise(uint32_t signo)
{
  return brights_signal_raise_proc(&global_signal_state, signo);
}

uint32_t brights_signal_pending_proc(brights_signal_state_t *state)
{
  if (!state) return 0;
  return state->pending & ~state->blocked;
}

uint32_t brights_signal_pending(void)
{
  return brights_signal_pending_proc(&global_signal_state);
}

int brights_signal_consume(brights_signal_state_t *state, uint32_t signo)
{
  if (!state || signo == 0 || signo >= BRIGHTS_SIGNAL_MAX) return -1;
  if ((state->pending & (1u << signo)) == 0) return -1;
  state->pending &= ~(1u << signo);
  return 0;
}

void brights_signal_clear(brights_signal_state_t *state)
{
  if (state) state->pending = 0;
}

void brights_signal_block(brights_signal_state_t *state, uint32_t mask)
{
  if (state) state->blocked |= mask;
}

void brights_signal_unblock(brights_signal_state_t *state, uint32_t mask)
{
  if (state) state->blocked &= ~mask;
}

sighandler_t brights_signal_sethandler(brights_signal_state_t *state, uint32_t signo, sighandler_t handler)
{
  if (!state || signo == 0 || signo >= BRIGHTS_SIGNAL_MAX) return SIG_ERR;
  if (signo == SIGKILL || signo == SIGSTOP) return SIG_ERR;
  
  sighandler_t old = state->handlers[signo];
  state->handlers[signo] = handler;
  state->sigactions[signo].sa_handler = handler;
  return old;
}

int brights_signal_sigaction(uint32_t signo, const brights_sigaction_t *act, brights_sigaction_t *oldact)
{
  if (signo == 0 || signo >= BRIGHTS_SIGNAL_MAX) return -1;
  if (signo == SIGKILL || signo == SIGSTOP) return -1;
  
  brights_signal_state_t *state = &global_signal_state;
  
  if (oldact) {
    *oldact = state->sigactions[signo];
  }
  
  if (act) {
    state->sigactions[signo] = *act;
    state->handlers[signo] = act->sa_handler;
  }
  
  return 0;
}

int brights_signal_deliver(brights_signal_state_t *state, uint32_t signo)
{
  if (!state || signo == 0 || signo >= BRIGHTS_SIGNAL_MAX) return -1;
  
  sighandler_t handler = state->handlers[signo];
  
  if (handler == SIG_DFL) {
    switch (signo) {
      case SIGCHLD:
      case SIGURG:
      case SIGWINCH:
        return 0;
      case SIGSTOP:
      case SIGTSTP:
      case SIGTTIN:
      case SIGTTOU:
        return -1;
      case SIGINT:
      case SIGTERM:
      case SIGKILL:
        return -1;
      default:
        return -1;
    }
  }
  
  if (handler == SIG_IGN) {
    brights_signal_consume(state, signo);
    return 0;
  }
  
  return 0;
}

brights_signal_state_t *brights_signal_global(void)
{
  return &global_signal_state;
}
