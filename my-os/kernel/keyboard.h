#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "idt.h"

/* Size of the key-event ring buffer (must be a power of 2) */
#define KB_BUF_SIZE 256

/*
 * keyboard_init()
 *   Registers the IRQ1 handler with the PIC dispatch table and
 *   clears the internal ring buffer.  Call BEFORE sti().
 */
void keyboard_init(void);

/*
 * keyboard_readline_irq(buf, max)
 *   Blocking readline — waits for characters arriving via IRQ1,
 *   echoes them to the screen, and returns when Enter is pressed.
 *   Writes at most (max-1) characters and appends a NUL terminator.
 */
void keyboard_readline_irq(char *buf, uint32_t max);

#endif /* KEYBOARD_H */
