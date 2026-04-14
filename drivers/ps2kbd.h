#ifndef BRIGHTS_PS2KBD_H
#define BRIGHTS_PS2KBD_H

#include <stdint.h>

/* Special key codes for non-ASCII keys */
#define KBD_KEY_UP      0x80
#define KBD_KEY_DOWN    0x81
#define KBD_KEY_LEFT    0x82
#define KBD_KEY_RIGHT   0x83
#define KBD_KEY_HOME    0x84
#define KBD_KEY_END     0x85
#define KBD_KEY_PGUP    0x86
#define KBD_KEY_PGDN    0x87
#define KBD_KEY_INSERT  0x88
#define KBD_KEY_DELETE  0x89
#define KBD_KEY_F1      0x8A
#define KBD_KEY_F2      0x8B
#define KBD_KEY_F3      0x8C
#define KBD_KEY_F4      0x8D
#define KBD_KEY_F5      0x8E
#define KBD_KEY_F6      0x8F
#define KBD_KEY_F7      0x90
#define KBD_KEY_F8      0x91
#define KBD_KEY_F9      0x92
#define KBD_KEY_F10     0x93
#define KBD_KEY_F11     0x94
#define KBD_KEY_F12     0x95

int brights_ps2kbd_init(void);
int brights_ps2kbd_read_char(char *out_ch);

/* Called from interrupt handler with scancode */
void brights_ps2kbd_irq_handler(void);

/* Non-blocking: check if a character is available */
int brights_ps2kbd_has_char(void);

#endif
