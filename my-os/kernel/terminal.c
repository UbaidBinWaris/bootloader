#include <stddef.h>
#include <stdint.h>
#include "terminal.h"

#define VGA_BASE ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static size_t terminal_row;
static size_t terminal_col;
static uint8_t terminal_color;

static inline uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)(fg | (bg << 4));
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void terminal_scroll(void) {
    volatile uint16_t *vga = VGA_BASE;

    for (size_t row = 0; row < VGA_HEIGHT - 1; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            vga[row * VGA_WIDTH + col] = vga[(row + 1) * VGA_WIDTH + col];
        }
    }

    for (size_t col = 0; col < VGA_WIDTH; col++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = vga_entry(' ', terminal_color);
    }

    if (terminal_row > 0) {
        terminal_row--;
    }
}

void terminal_init(void) {
    terminal_clear();
}

void terminal_clear(void) {
    volatile uint16_t *vga = VGA_BASE;

    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_row = 0;
    terminal_col = 0;

    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = vga_entry(' ', terminal_color);
    }
}

void terminal_set_color(vga_color_t fg, vga_color_t bg) {
    terminal_color = vga_entry_color(fg, bg);
}

void terminal_putchar(char c) {
    volatile uint16_t *vga = VGA_BASE;

    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
    } else if (c == '\r') {
        terminal_col = 0;
    } else if (c == '\b') {
        if (terminal_col > 0) {
            terminal_col--;
            vga[terminal_row * VGA_WIDTH + terminal_col] = vga_entry(' ', terminal_color);
        }
    } else {
        vga[terminal_row * VGA_WIDTH + terminal_col] = vga_entry(c, terminal_color);
        terminal_col++;

        if (terminal_col >= VGA_WIDTH) {
            terminal_col = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
    }
}

void terminal_print(const char *str) {
    while (*str) {
        terminal_putchar(*str++);
    }
}

void terminal_println(const char *str) {
    terminal_print(str);
    terminal_putchar('\n');
}

void terminal_print_hex(uint32_t n) {
    const char *digits = "0123456789ABCDEF";
    int leading = 1;

    terminal_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (uint8_t)((n >> i) & 0xF);
        if (nibble || !leading || i == 0) {
            terminal_putchar(digits[nibble]);
            leading = 0;
        }
    }
}
