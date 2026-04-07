/* =============================================================================
 * UbaidOS — Kernel
 * Architecture : x86 32-bit protected mode
 * Linked at    : 0x2000 (flat binary)
 * Compiler     : gcc -m32 -ffreestanding -fno-pie -O2
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 *  SECTION 1 — VGA TEXT-MODE TERMINAL DRIVER
 * ============================================================ */

#define VGA_BASE       ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH      80
#define VGA_HEIGHT     25

typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

static inline uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)(fg | (bg << 4));
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* Terminal state */
static size_t   terminal_row;
static size_t   terminal_col;
static uint8_t  terminal_color;

static void terminal_scroll(void) {
    volatile uint16_t *vga = VGA_BASE;
    /* Shift every row up by one */
    for (size_t row = 0; row < VGA_HEIGHT - 1; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            vga[row * VGA_WIDTH + col] = vga[(row + 1) * VGA_WIDTH + col];
        }
    }
    /* Clear the last row */
    for (size_t col = 0; col < VGA_WIDTH; col++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = vga_entry(' ', terminal_color);
    }
    if (terminal_row > 0) terminal_row--;
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

void terminal_init(void) {
    terminal_clear();
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
    } else if (c == '\t') {
        /* Advance to next 8-column tab stop */
        do {
            terminal_putchar(' ');
        } while (terminal_col % 8 != 0);
        return;
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
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_println(const char *str) {
    terminal_print(str);
    terminal_putchar('\n');
}

void terminal_print_uint(uint32_t n) {
    if (n == 0) { terminal_putchar('0'); return; }
    char buf[12];
    int  pos = 0;
    while (n > 0) { buf[pos++] = (char)('0' + (n % 10)); n /= 10; }
    for (int i = pos - 1; i >= 0; i--) terminal_putchar(buf[i]);
}

void terminal_print_hex(uint32_t n) {
    terminal_print("0x");
    const char *digits = "0123456789ABCDEF";
    int leading = 1;
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (uint8_t)((n >> i) & 0xF);
        if (nibble || !leading || i == 0) {
            terminal_putchar(digits[nibble]);
            leading = 0;
        }
    }
}

/* ============================================================
 *  SECTION 2 — I/O PORT HELPERS
 * ============================================================ */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0x00);
}

/* ============================================================
 *  SECTION 3 — PS/2 KEYBOARD DRIVER  (polling, scancode set 1)
 * ============================================================ */

static const char scancode_to_ascii[] = {
    0,    0,   '1', '2', '3', '4', '5', '6',    /* 0x00 – 0x07 */
    '7', '8', '9', '0', '-', '=',  '\b', '\t',  /* 0x08 – 0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',     /* 0x10 – 0x17 */
    'o', 'p', '[', ']', '\n',  0,  'a', 's',     /* 0x18 – 0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     /* 0x20 – 0x27 */
    '\'','`',  0,  '\\','z', 'x', 'c', 'v',     /* 0x28 – 0x2F */
    'b', 'n', 'm', ',', '.', '/',  0,   '*',     /* 0x30 – 0x37 */
    0,   ' '                                      /* 0x38 – 0x39 */
};

static const char scancode_to_shifted[] = {
    0,    0,   '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+',  '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n',  0,  'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~',  0,  '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?',  0,   '*',
    0,   ' '
};

static int shift_active = 0;
static int caps_active  = 0;

char keyboard_getchar(void) {
    while (1) {
        /* Wait until output buffer is full (bit 0 of status port 0x64) */
        if (!(inb(0x64) & 0x01)) continue;

        uint8_t sc = inb(0x60);

        /* Key-release: high bit set */
        if (sc & 0x80) {
            uint8_t released = sc & 0x7F;
            if (released == 0x2A || released == 0x36) shift_active = 0;
            continue;
        }

        /* Left/right shift press */
        if (sc == 0x2A || sc == 0x36) { shift_active = 1; continue; }
        /* Caps Lock toggle */
        if (sc == 0x3A) { caps_active = !caps_active; continue; }

        /* Ignore unmapped scancodes */
        if (sc >= sizeof(scancode_to_ascii)) continue;

        char c = scancode_to_ascii[sc];
        if (c == 0) continue;

        /* Apply shift / caps for letters */
        if (shift_active) {
            c = scancode_to_shifted[sc];
        } else if (caps_active && c >= 'a' && c <= 'z') {
            c = (char)(c - 32);
        }

        return c;
    }
}

/* Read a full line (echoed); returns number of chars written (excl. NUL) */
int keyboard_readline(char *buf, int max) {
    int pos = 0;
    while (1) {
        char c = keyboard_getchar();
        if (c == '\n' || c == '\r') {
            terminal_putchar('\n');
            buf[pos] = '\0';
            return pos;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                terminal_putchar('\b');
            }
        } else if (pos < max - 1) {
            buf[pos++] = c;
            terminal_putchar(c);
        }
    }
}

/* ============================================================
 *  SECTION 4 — STRING UTILITIES
 * ============================================================ */

static size_t kstrlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static int kstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static int kstrncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    return n == (size_t)-1 ? 0 : (unsigned char)*a - (unsigned char)*b;
}

static const char *kskip_spaces(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/* ============================================================
 *  SECTION 5 — SHELL
 * ============================================================ */

#define CMD_BUF_SIZE 128

static void cmd_help(void) {
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_println("UbaidOS Shell Commands:");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_println("  help              - show this help");
    terminal_println("  clear             - clear the screen");
    terminal_println("  echo [text]       - print text to screen");
    terminal_println("  color [fg] [bg]   - set colors (0-15)");
    terminal_println("  info              - show system information");
    terminal_println("  reboot            - reboot the system");
}

static void cmd_echo(const char *args) {
    terminal_println(args);
}

static void cmd_color(const char *args) {
    args = kskip_spaces(args);
    if (*args == '\0') {
        terminal_println("Usage: color [fg 0-15] [bg 0-15]");
        return;
    }
    int fg = 0, bg = 0;
    while (*args >= '0' && *args <= '9') fg = fg * 10 + (*args++ - '0');
    args = kskip_spaces(args);
    while (*args >= '0' && *args <= '9') bg = bg * 10 + (*args++ - '0');
    if (fg > 15) fg = 15;
    if (bg > 15) bg = 15;
    terminal_set_color((vga_color_t)fg, (vga_color_t)bg);
    terminal_println("Color changed.");
}

static void cmd_info(void) {
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_println("=== UbaidOS System Information ===");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_println("  OS       : UbaidOS v1.0");
    terminal_println("  Arch     : x86 32-bit Protected Mode");
    terminal_print  ("  VGA Base : ");
    terminal_print_hex(0xB8000);
    terminal_putchar('\n');
    terminal_print  ("  Kernel   : loaded at ");
    terminal_print_hex(0x2000);
    terminal_putchar('\n');
    terminal_println("  Stack    : 0x90000");
    terminal_println("  Author   : Ubaid");
    terminal_println("==================================");
}

static void cmd_reboot(void) {
    terminal_println("Rebooting...");
    io_wait();
    /* Pulse the reset line via keyboard controller */
    outb(0x64, 0xFE);
    io_wait();
    /* Triple-fault fallback */
    __asm__ volatile (
        "lidt %0\n\t"
        "int $3\n\t"
        : : "m"(*(uint64_t[1]){{ 0 }})
    );
}

static void shell_dispatch(const char *line) {
    line = kskip_spaces(line);

    if (*line == '\0') return;

    if (kstrcmp(line, "help") == 0) {
        cmd_help();
    } else if (kstrcmp(line, "clear") == 0) {
        terminal_clear();
    } else if (kstrncmp(line, "echo ", 5) == 0) {
        cmd_echo(kskip_spaces(line + 5));
    } else if (kstrncmp(line, "color ", 6) == 0) {
        cmd_color(line + 6);
    } else if (kstrcmp(line, "info") == 0) {
        cmd_info();
    } else if (kstrcmp(line, "reboot") == 0) {
        cmd_reboot();
    } else {
        terminal_print("Unknown command: ");
        terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_println(line);
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_println("Type 'help' for available commands.");
    }
}

static void shell_run(void) {
    static char cmd_buf[CMD_BUF_SIZE];

    while (1) {
        terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        terminal_print("ubaid");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_print("@");
        terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        terminal_print("UbaidOS");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_print(":~$ ");
        terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        keyboard_readline(cmd_buf, CMD_BUF_SIZE);

        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        shell_dispatch(cmd_buf);
    }
}

/* ============================================================
 *  SECTION 6 — BOOT BANNER
 * ============================================================ */

static void print_banner(void) {
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_println("");
    terminal_println("  _   _ _           _     ____  ____");
    terminal_println(" | | | | |__   __ _(_) __| __ )|  _ \\");
    terminal_println(" | | | | '_ \\ / _` | |/ _|  _ \\| | | |");
    terminal_println(" | |_| | |_) | (_| | | (_| |_) | |_| |");
    terminal_println("  \\___/|_.__/ \\__,_|_|\\__|____/|____/");
    terminal_println("");
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_println("  UbaidOS v1.0 — x86 Protected Mode OS");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_println("  Type 'help' to see available commands.");
    terminal_println("");
}

/* ============================================================
 *  SECTION 7 — KERNEL ENTRY
 * ============================================================ */

void kernel_main(void) {
    terminal_init();
    print_banner();
    shell_run();

    /* Should never reach here */
    __asm__ volatile ("cli; hlt");
}
