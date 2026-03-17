#include <stdint.h>

#define BRIGHTS_SIGNAL_MAX 32u

typedef struct {
  uint32_t pending_mask;
} brights_signal_state_t;

static brights_signal_state_t signal_state;

void brights_signal_init(void)
{
  signal_state.pending_mask = 0;
}

int brights_signal_raise(uint32_t signo)
{
  if (signo == 0 || signo >= BRIGHTS_SIGNAL_MAX) {
    return -1;
  }
  signal_state.pending_mask |= (1u << signo);
  return 0;
}

uint32_t brights_signal_pending(void)
{
  return signal_state.pending_mask;
}
