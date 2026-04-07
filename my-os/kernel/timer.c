#include <stdint.h>
#include "io.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"

/* ------------------------------------------------------------------ */
/*  8253/8254 PIT registers                                             */
/* ------------------------------------------------------------------ */
#define PIT_CH0_DATA    0x40    /* Channel 0 data port (R/W)          */
#define PIT_CMD         0x43    /* Mode/command register (write only) */

/*
 * Command byte:  0  0  1  1  0  1  1  0
 *                SC SC RW RW M  M  M  BCD
 *   SC  = 00  → channel 0
 *   RW  = 11  → access mode: lobyte / hibyte
 *   M   = 011 → mode 3 (square wave generator — stays accurate over time)
 *   BCD = 0   → 16-bit binary counter
 */
#define PIT_CMD_CHANNEL0_LHBYTE_MODE3  0x36

/* PIT input clock frequency, Hz */
#define PIT_BASE_HZ     1193180UL

/* ------------------------------------------------------------------ */
/*  Tick counter                                                        */
/* ------------------------------------------------------------------ */
static volatile uint32_t ticks = 0;

/* ------------------------------------------------------------------ */
/*  IRQ0 handler — registered by timer_init()                          */
/* ------------------------------------------------------------------ */
static void timer_irq_handler(registers_t *regs)
{
    (void)regs;     /* unused */
    ticks++;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */
void timer_init(uint32_t hz)
{
    if (hz == 0)
        hz = 100;   /* safe default */

    uint32_t divisor = PIT_BASE_HZ / hz;

    /* Sanity clamp: PIT uses a 16-bit counter; 0 means 65536 */
    if (divisor > 0xFFFF) divisor = 0xFFFF;
    if (divisor < 1)      divisor = 1;

    /* Send command byte first */
    outb(PIT_CMD, PIT_CMD_CHANNEL0_LHBYTE_MODE3);

    /* Send divisor low byte then high byte */
    outb(PIT_CH0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0_DATA, (uint8_t)((divisor >> 8) & 0xFF));

    /* Register our handler for IRQ0 */
    irq_register(0, timer_irq_handler);
}

uint32_t timer_get_ticks(void)
{
    return ticks;
}
