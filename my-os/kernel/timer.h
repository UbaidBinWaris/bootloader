#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "idt.h"

/*
 * timer_init(hz)
 *   Programs the 8253/8254 PIT channel 0 to fire at the requested
 *   frequency (e.g. 100 Hz) and registers the IRQ0 handler.
 *   Call BEFORE sti().
 */
void timer_init(uint32_t hz);

/*
 * timer_get_ticks()
 *   Returns the number of PIT interrupts that have fired since
 *   timer_init() was called.
 */
uint32_t timer_get_ticks(void);

#endif /* TIMER_H */
