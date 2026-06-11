#ifndef BRIGHTS_PRINTF_H
#define BRIGHTS_PRINTF_H

#include <stdint.h>

typedef struct brights_console brights_console_t;

typedef void (*brights_puts_fn)(brights_console_t *con, const uint16_t *msg);

struct brights_console {
  void *ctx;
  brights_puts_fn puts;
};

void brights_console_init(brights_console_t *con, void *ctx, brights_puts_fn puts_fn);
void brights_print(brights_console_t *con, const uint16_t *msg);

#endif
