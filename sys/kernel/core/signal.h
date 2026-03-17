#ifndef BRIGHTS_SIGNAL_H
#define BRIGHTS_SIGNAL_H

#include <stdint.h>

void brights_signal_init(void);
int brights_signal_raise(uint32_t signo);
uint32_t brights_signal_pending(void);

#endif
