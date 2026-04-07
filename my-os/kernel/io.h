#ifndef IO_H
#define IO_H

#include <stdint.h>

/* ============================================================
 *  io.h — Inline x86 port I/O helpers
 *
 *  Shared across: kernel.c, pic.c, keyboard.c, timer.c
 * ============================================================ */

/* Write a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/* Read a byte from an I/O port */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/* Small delay via a dummy write to a non-existent POST port.
   Required between PIC initialisation command writes. */
static inline void io_wait(void) {
    outb(0x80, 0x00);
}

#endif /* IO_H */
