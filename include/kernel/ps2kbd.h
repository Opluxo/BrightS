#ifndef BRIGHTS_PS2KBD_H
#define BRIGHTS_PS2KBD_H

int brights_ps2kbd_init(void);
int brights_ps2kbd_read_char(char *out_ch);

/* Called from interrupt handler with scancode */
void brights_ps2kbd_irq_handler(void);

/* Non-blocking: check if a character is available */
int brights_ps2kbd_has_char(void);

#endif
