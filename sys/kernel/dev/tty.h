#ifndef BRIGHTS_TTY_H
#define BRIGHTS_TTY_H

#include <stdint.h>

void brights_tty_init(void);
void brights_tty_write(const uint16_t *s);
int brights_tty_read_char(char *out_ch);
char brights_tty_read_char_blocking(void);

#endif
