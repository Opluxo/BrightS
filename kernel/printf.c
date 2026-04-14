#include "printf.h"

void brights_console_init(brights_console_t *con, void *ctx, brights_puts_fn puts_fn)
{
  con->ctx = ctx;
  con->puts = puts_fn;
}

void brights_print(brights_console_t *con, const uint16_t *msg)
{
  if (!con || !con->puts) {
    return;
  }
  con->puts(con, msg);
}
