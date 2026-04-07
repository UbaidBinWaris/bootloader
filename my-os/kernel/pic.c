#include <stdint.h>
#include "io.h"
#include "idt.h"
#include "pic.h"

/* ------------------------------------------------------------------ */
/*  Forward-declaration of the terminal print functions defined in     */
/*  kernel.c so we can print exception messages here without a         */
/*  separate header.  kernel.c is always linked into the same binary.  */
/* ------------------------------------------------------------------ */
extern void terminal_print(const char *s);
extern void terminal_println(const char *s);
extern void terminal_print_hex(uint32_t val);

/* ------------------------------------------------------------------ */
/*  IRQ dispatch table                                                  */
/* ------------------------------------------------------------------ */
static irq_handler_t irq_handlers[16] = { 0 };

void irq_register(uint8_t irq, irq_handler_t handler)
{
    if (irq < 16)
        irq_handlers[irq] = handler;
}

/* ------------------------------------------------------------------ */
/*  PIC remapping — send ICW1-4 to both master and slave               */
/* ------------------------------------------------------------------ */
void pic_remap(uint8_t offset1, uint8_t offset2)
{
    /* Save current masks so devices don't get spurious interrupts */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* ICW1: start init sequence (cascade mode, ICW4 needed) */
    outb(PIC1_CMD,  ICW1_INIT | ICW1_ICW4);  io_wait();
    outb(PIC2_CMD,  ICW1_INIT | ICW1_ICW4);  io_wait();

    /* ICW2: vector offsets */
    outb(PIC1_DATA, offset1);  io_wait();     /* master: 0x20 */
    outb(PIC2_DATA, offset2);  io_wait();     /* slave:  0x28 */

    /* ICW3: tell master about slave on IRQ2; tell slave its cascade id */
    outb(PIC1_DATA, 0x04);  io_wait();        /* master: slave on IRQ2 (bit 2) */
    outb(PIC2_DATA, 0x02);  io_wait();        /* slave:  cascade identity = 2  */

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, ICW4_8086);  io_wait();
    outb(PIC2_DATA, ICW4_8086);  io_wait();

    /* Restore saved masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/* ------------------------------------------------------------------ */
/*  EOI — must be sent after EVERY hardware interrupt is handled       */
/* ------------------------------------------------------------------ */
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);   /* slave PIC first for IRQ 8-15 */
    outb(PIC1_CMD, PIC_EOI);
}

/* ------------------------------------------------------------------ */
/*  Individual IRQ masking                                              */
/* ------------------------------------------------------------------ */
void pic_set_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port  = PIC1_DATA;
    } else {
        port  = PIC2_DATA;
        irq  -= 8;
    }
    value = inb(port) | (uint8_t)(1 << irq);
    outb(port, value);
}

void pic_clear_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port  = PIC1_DATA;
    } else {
        port  = PIC2_DATA;
        irq  -= 8;
    }
    value = inb(port) & (uint8_t)(~(1 << irq));
    outb(port, value);
}

/* ------------------------------------------------------------------ */
/*  ISR (CPU exception) dispatcher                                      */
/*  Called from isr_common_stub in isr.asm                             */
/* ------------------------------------------------------------------ */

/* Exception names — index matches the vector number */
static const char *exception_msgs[32] = {
    "Division by zero",           /* 0  #DE */
    "Debug",                      /* 1  #DB */
    "Non-maskable interrupt",     /* 2       */
    "Breakpoint",                 /* 3  #BP */
    "Overflow",                   /* 4  #OF */
    "Bound range exceeded",       /* 5  #BR */
    "Invalid opcode",             /* 6  #UD */
    "Device not available",       /* 7  #NM */
    "Double fault",               /* 8  #DF */
    "Coprocessor seg overrun",    /* 9       */
    "Invalid TSS",                /* 10 #TS */
    "Segment not present",        /* 11 #NP */
    "Stack-segment fault",        /* 12 #SS */
    "General protection fault",   /* 13 #GP */
    "Page fault",                 /* 14 #PF */
    "Reserved",                   /* 15      */
    "x87 floating-point",         /* 16 #MF */
    "Alignment check",            /* 17 #AC */
    "Machine check",              /* 18 #MC */
    "SIMD floating-point",        /* 19 #XM */
    "Virtualisation",             /* 20 #VE */
    "Control protection",         /* 21 #CP */
    "Reserved",                   /* 22      */
    "Reserved",                   /* 23      */
    "Reserved",                   /* 24      */
    "Reserved",                   /* 25      */
    "Reserved",                   /* 26      */
    "Reserved",                   /* 27      */
    "Reserved",                   /* 28      */
    "Reserved",                   /* 29      */
    "Security exception",         /* 30 #SX */
    "Reserved"                    /* 31      */
};

void isr_handler(registers_t *regs)
{
    terminal_println("");
    terminal_print("KERNEL PANIC — Exception #");
    terminal_print_hex((uint32_t)regs->int_no);

    if (regs->int_no < 32) {
        terminal_print(": ");
        terminal_print(exception_msgs[regs->int_no]);
    }

    terminal_println("");
    terminal_print("  EIP="); terminal_print_hex(regs->eip);
    terminal_print("  CS=");  terminal_print_hex(regs->cs);
    terminal_print("  EFLAGS="); terminal_print_hex(regs->eflags);
    terminal_println("");
    terminal_print("  EAX="); terminal_print_hex(regs->eax);
    terminal_print("  EBX="); terminal_print_hex(regs->ebx);
    terminal_print("  ECX="); terminal_print_hex(regs->ecx);
    terminal_print("  EDX="); terminal_print_hex(regs->edx);
    terminal_println("");
    terminal_print("  err_code="); terminal_print_hex(regs->err_code);
    terminal_println("");
    terminal_println("System halted.");

    /* Halt permanently — do not return to broken code */
    __asm__ volatile ("cli; hlt");
    for (;;) {} /* unreachable — satisfy noreturn semantics */
}

/* ------------------------------------------------------------------ */
/*  IRQ (hardware interrupt) dispatcher                                 */
/*  Called from irq_common_stub in irq.asm                             */
/* ------------------------------------------------------------------ */
void irq_handler(registers_t *regs)
{
    /* The vector stored in int_no is 0x20-0x2F; convert to IRQ 0-15 */
    uint8_t irq = (uint8_t)(regs->int_no - 0x20);

    /* Dispatch to registered C handler if one exists */
    if (irq < 16 && irq_handlers[irq] != 0)
        irq_handlers[irq](regs);

    /* Always send EOI so the PIC can fire the next interrupt */
    pic_send_eoi(irq);
}
