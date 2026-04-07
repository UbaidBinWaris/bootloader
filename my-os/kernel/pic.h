#ifndef PIC_H
#define PIC_H

#include <stdint.h>
#include "idt.h"

/* ---- PIC port addresses ---------------------------------------- */
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

/* ---- PIC commands ---------------------------------------------- */
#define PIC_EOI     0x20    /* End-of-interrupt command              */

/* ---- ICW1 bits -------------------------------------------------- */
#define ICW1_INIT   0x10    /* Initialization — required             */
#define ICW1_ICW4   0x01    /* ICW4 needed                           */

/* ---- ICW4 bits -------------------------------------------------- */
#define ICW4_8086   0x01    /* 8086/88 mode (vs MCS-80/85)          */

/*
 * pic_remap(offset1, offset2)
 *   offset1 — new INT base for IRQ 0-7  (use 0x20)
 *   offset2 — new INT base for IRQ 8-15 (use 0x28)
 *
 * Must be called before sti().  Sends the full ICW1-4 sequence to
 * both master (0x20/0x21) and slave (0xA0/0xA1) PICs.
 */
void pic_remap(uint8_t offset1, uint8_t offset2);

/*
 * pic_send_eoi(irq)
 *   Send the End-Of-Interrupt signal.
 *   IRQ 8-15 require an EOI to the slave PIC first.
 */
void pic_send_eoi(uint8_t irq);

/*
 * pic_set_mask(irq) / pic_clear_mask(irq)
 *   Mask (disable) or unmask (enable) a specific IRQ line.
 */
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

/*
 * isr_handler(regs) — called from isr_common_stub (isr.asm)
 *   Prints exception info to the screen and halts.
 *   Declared here because isr.asm declares extern isr_handler.
 */
void isr_handler(registers_t *regs);

/*
 * irq_handler(regs) — called from irq_common_stub (irq.asm)
 *   Dispatches to per-IRQ C handlers, sends EOI.
 *   Declared here because irq.asm declares extern irq_handler.
 */
void irq_handler(registers_t *regs);

/*
 * irq_register(irq, handler)
 *   Register a C function to be called when IRQ fires.
 *   Replaces any existing handler for that line.
 */
typedef void (*irq_handler_t)(registers_t *regs);
void irq_register(uint8_t irq, irq_handler_t handler);

#endif /* PIC_H */
