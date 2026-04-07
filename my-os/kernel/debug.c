#include <stdint.h>
#include "io.h"
#include "debug.h"

#define COM1 0x3F8

static int debug_ready(void) {
    return inb(COM1 + 5) & 0x20;
}

static void debug_putc(char c) {
    while (!debug_ready()) {
    }
    outb(COM1, (uint8_t)c);
}

void debug_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void debug_print(const char *s) {
    while (*s) {
        debug_putc(*s++);
    }
}

void debug_println(const char *s) {
    debug_print(s);
    debug_putc('\r');
    debug_putc('\n');
}
