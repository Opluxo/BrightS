#ifndef BRIGHTS_SERIAL_H
#define BRIGHTS_SERIAL_H

#include <stdint.h>
#include "../kernel/printf.h"

#define BRIGHTS_COM1_PORT 0x3F8

void brights_serial_init(uint16_t port);
void brights_serial_console_init(brights_console_t *con, uint16_t port);
int brights_serial_read_byte(uint16_t port, uint8_t *out);
uint8_t brights_serial_read_byte_blocking(uint16_t port);
void brights_serial_write_ascii(uint16_t port, const char *s);
void brights_serial_write(uint16_t port, const void *buf, uint64_t len);

#endif
