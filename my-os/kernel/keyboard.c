#include <stdint.h>
#include "io.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"

/* ------------------------------------------------------------------ */
/*  Hardware constants                                                  */
/* ------------------------------------------------------------------ */
#define KB_DATA_PORT    0x60    /* Read scancode / write command data  */
#define KB_STATUS_PORT  0x64    /* Read status / write command         */

/* ------------------------------------------------------------------ */
/*  Forward declarations for terminal helpers in kernel.c              */
/* ------------------------------------------------------------------ */
extern void terminal_putchar(char c);

/* ------------------------------------------------------------------ */
/*  Scancode Set 1 → ASCII tables                                       */
/*  Non-printable or unmapped keys translate to 0 and are ignored.     */
/* ------------------------------------------------------------------ */

/* Unshifted */
static const char scancode_to_ascii[128] = {
/*00*/  0,    0,   '1', '2', '3', '4', '5', '6',
/*08*/ '7', '8', '9', '0', '-', '=',  '\b', '\t',
/*10*/ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
/*18*/ 'o', 'p', '[', ']', '\n',  0,  'a', 's',
/*20*/ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
/*28*/ '\'', '`', 0,  '\\','z', 'x', 'c', 'v',
/*30*/ 'b', 'n', 'm', ',', '.', '/', 0,   '*',
/*38*/  0,   ' ',  0,   0,   0,   0,   0,   0,
/*40*/  0,   0,   0,   0,   0,   0,   0,  '7',
/*48*/ '8', '9', '-', '4', '5', '6', '+', '1',
/*50*/ '2', '3', '0', '.', 0,   0,   0,   0,
/*58*/  0,   0,   0,   0,   0,   0,   0,   0,
/*60*/  0,   0,   0,   0,   0,   0,   0,   0,
/*68*/  0,   0,   0,   0,   0,   0,   0,   0,
/*70*/  0,   0,   0,   0,   0,   0,   0,   0,
/*78*/  0,   0,   0,   0,   0,   0,   0,   0,
};

/* Shifted (Shift held) */
static const char scancode_to_shifted[128] = {
/*00*/  0,    0,   '!', '@', '#', '$', '%', '^',
/*08*/ '&', '*', '(', ')', '_', '+',  '\b', '\t',
/*10*/ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
/*18*/ 'O', 'P', '{', '}', '\n',  0,  'A', 'S',
/*20*/ 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
/*28*/ '"', '~',  0,  '|', 'Z', 'X', 'C', 'V',
/*30*/ 'B', 'N', 'M', '<', '>', '?',  0,  '*',
/*38*/  0,   ' ',  0,   0,   0,   0,   0,   0,
/*40*/  0,   0,   0,   0,   0,   0,   0,  '7',
/*48*/ '8', '9', '-', '4', '5', '6', '+', '1',
/*50*/ '2', '3', '0', '.', 0,   0,   0,   0,
/*58*/  0,   0,   0,   0,   0,   0,   0,   0,
/*60*/  0,   0,   0,   0,   0,   0,   0,   0,
/*68*/  0,   0,   0,   0,   0,   0,   0,   0,
/*70*/  0,   0,   0,   0,   0,   0,   0,   0,
/*78*/  0,   0,   0,   0,   0,   0,   0,   0,
};

/* Scancode values for modifier keys */
#define SC_LSHIFT_DOWN  0x2A
#define SC_RSHIFT_DOWN  0x36
#define SC_LSHIFT_UP    0xAA
#define SC_RSHIFT_UP    0xB6
#define SC_CAPS_DOWN    0x3A

/* ------------------------------------------------------------------ */
/*  Ring buffer                                                         */
/* ------------------------------------------------------------------ */
static volatile char    kb_buf[KB_BUF_SIZE];
static volatile uint8_t kb_head = 0;    /* write index (IRQ side)     */
static volatile uint8_t kb_tail = 0;    /* read  index (consumer side)*/

static inline void kb_buf_push(char c)
{
    uint8_t next = (uint8_t)((kb_head + 1) & (KB_BUF_SIZE - 1));
    if (next != kb_tail) {          /* drop character if buffer full  */
        kb_buf[kb_head] = c;
        kb_head = next;
    }
}

/* Returns 0 if buffer empty, non-zero if a character is available */
static inline int kb_buf_empty(void)
{
    return kb_head == kb_tail;
}

static inline char kb_buf_pop(void)
{
    char c = kb_buf[kb_tail];
    kb_tail = (uint8_t)((kb_tail + 1) & (KB_BUF_SIZE - 1));
    return c;
}

/* ------------------------------------------------------------------ */
/*  Modifier state                                                      */
/* ------------------------------------------------------------------ */
static volatile uint8_t shift_held = 0;
static volatile uint8_t caps_lock  = 0;

/* ------------------------------------------------------------------ */
/*  IRQ1 handler — called from irq_handler() in pic.c                  */
/* ------------------------------------------------------------------ */
static void keyboard_irq_handler(registers_t *regs)
{
    (void)regs;  /* unused */

    uint8_t scancode = inb(KB_DATA_PORT);

    /* Key-release events have bit 7 set */
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == (SC_LSHIFT_DOWN & 0x7F) ||
            released == (SC_RSHIFT_DOWN & 0x7F))
            shift_held = 0;
        return;     /* ignore all other release events */
    }

    /* Modifier key down */
    if (scancode == SC_LSHIFT_DOWN || scancode == SC_RSHIFT_DOWN) {
        shift_held = 1;
        return;
    }
    if (scancode == SC_CAPS_DOWN) {
        caps_lock = !caps_lock;
        return;
    }

    /* Translate scancode → ASCII */
    if (scancode >= 128)
        return;     /* extended or unknown */

    char c;
    int use_upper = shift_held ^ caps_lock;

    if (use_upper)
        c = scancode_to_shifted[scancode];
    else
        c = scancode_to_ascii[scancode];

    if (c != 0)
        kb_buf_push(c);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */
void keyboard_init(void)
{
    kb_head    = 0;
    kb_tail    = 0;
    shift_held = 0;
    caps_lock  = 0;

    irq_register(1, keyboard_irq_handler);
}

void keyboard_readline_irq(char *buf, uint32_t max)
{
    uint32_t pos = 0;

    while (1) {
        /* Halt until next interrupt arrives — saves power and avoids
           spinning on a tight loop burning the CPU */
        __asm__ volatile ("sti; hlt");

        /* Drain all characters the IRQ handler deposited */
        while (!kb_buf_empty()) {
            char c = kb_buf_pop();

            if (c == '\n' || c == '\r') {
                terminal_putchar('\n');
                buf[pos] = '\0';
                return;
            }

            if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');  /* terminal_putchar handles erase */
                }
                continue;
            }

            if (pos < max - 1) {
                buf[pos++] = c;
                terminal_putchar(c);
            }
        }
    }
}
