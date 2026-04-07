# ⚡ Interrupt System (Protected Mode)

## Overview

UbaidOS uses a **hardware interrupt-driven** architecture in 32-bit protected mode. BIOS software interrupts (`INT 0x10`, `INT 0x16`) are **not used** in the kernel — all I/O goes through direct hardware ports and the Programmable Interrupt Controller (8259A PIC).

---

## IDT — Interrupt Descriptor Table

The IDT is an array of 256 gate descriptors loaded into the CPU with the `lidt` instruction.

### Gate descriptor layout (8 bytes each)

```
 Bits 63–48  handler_high  (upper 16 bits of handler address)
 Bits 47–40  flags         (0x8E = present | ring-0 | 32-bit interrupt gate)
 Bits 39–32  always_zero   (0x00)
 Bits 31–16  selector      (0x08 = kernel code segment)
 Bits 15–0   handler_low   (lower 16 bits of handler address)
```

Gate flags `0x8E` decoded:

| Bit | Name | Value | Meaning |
|-----|------|-------|---------|
| 7 | Present | 1 | Gate is valid |
| 6–5 | DPL | 00 | Ring-0 only |
| 4 | Storage seg | 0 | Interrupt/trap gate |
| 3–0 | Gate type | 1110 | 32-bit interrupt gate |

### C struct

```c
typedef struct {
    uint16_t handler_low;   // bits 0–15 of handler
    uint16_t selector;      // 0x08 — kernel code segment
    uint8_t  always_zero;
    uint8_t  flags;         // 0x8E
    uint16_t handler_high;  // bits 16–31 of handler
} __attribute__((packed)) idt_entry_t;
```

---

## 8259A PIC Remap

By default the PIC maps IRQ0–7 to INT `0x08`–`0x0F`, which collide with CPU exception vectors. The kernel remaps them:

```c
pic_remap(0x20, 0x28);
// IRQ0–7  → INT 0x20–0x27  (master PIC)
// IRQ8–15 → INT 0x28–0x2F  (slave PIC)
```

### Initialisation sequence (ICW = Initialization Command Word)

| Port | Value | Meaning |
|------|-------|---------|
| `0x20` | `0x11` | ICW1 — cascade mode, ICW4 needed |
| `0xA0` | `0x11` | ICW1 — slave |
| `0x21` | `0x20` | ICW2 — master base vector |
| `0xA1` | `0x28` | ICW2 — slave base vector |
| `0x21` | `0x04` | ICW3 — master has slave on IRQ2 |
| `0xA1` | `0x02` | ICW3 — slave ID = 2 |
| `0x21` | `0x01` | ICW4 — 8086 mode |
| `0xA1` | `0x01` | ICW4 — slave 8086 mode |

---

## IRQ Vector Map

| IRQ | INT vector | Handler | Description |
|-----|-----------|---------|-------------|
| 0 | `0x20` | `timer_handler` | PIT — 100 Hz tick |
| 1 | `0x21` | `keyboard_handler` | PS/2 key press / release |
| 2 | `0x22` | stub | Cascade (slave PIC) |
| 3–7 | `0x23`–`0x27` | stubs | COM2, COM1, LPT2, floppy, LPT1 |
| 8 | `0x28` | stub | RTC |
| 9–15 | `0x29`–`0x2F` | stubs | Misc / PCI |

### CPU exceptions (ISR 0–31)

| Vector | Exception |
|--------|----------|
| `0x00` | Divide-by-zero |
| `0x06` | Invalid opcode |
| `0x08` | Double fault |
| `0x0D` | General protection fault |
| `0x0E` | Page fault |
| `0x0B` | Segment not present |

---

## EOI — End of Interrupt

After every IRQ handler, the PIC must be acknowledged. Failure to send EOI locks the IRQ line.

```c
// Master IRQ (0–7)
outb(0x20, 0x20);

// Slave IRQ (8–15) — acknowledge both
outb(0xA0, 0x20);
outb(0x20, 0x20);
```

---

## Keyboard Driver (IRQ1)

The keyboard handler (`keyboard.c`) reads the scancode from port `0x60`, converts it to ASCII, and writes it into a circular ring buffer. The kernel reads from the buffer via `keyboard_readline_irq()` — **no polling, no `INT 0x16`**.

---

## Timer Driver (IRQ0)

The PIT timer fires 100 times per second. The handler atomically increments `timer_ticks`. User code calls `timer_get_ticks()` to read elapsed ticks without touching the PIC.

### PIT divisor calculation

```
Base frequency: 1193182 Hz
Target:            100 Hz
Divisor:        1193182 / 100 = 11931
```

---

## 🚀 Advanced Direction

Later you will:

* Replace BIOS interrupts
* Write your own drivers
* Control hardware directly

---

## 💡 Real Insight

BIOS interrupts are:

> Training wheels for OS development

Real OS:
❌ Doesn’t use BIOS
✅ Uses custom drivers

---
