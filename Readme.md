# UbaidOS — x86 Protected-Mode Operating System

## Overview

UbaidOS is a fully hand-written x86 operating system built from scratch in Assembly and C.
It boots through a two-stage real-mode loader, enters 32-bit protected mode, and runs a
freestanding C kernel with a complete hardware-interrupt subsystem — including a remapped 8259A PIC,
a 256-entry IDT, an interrupt-driven keyboard driver, and a PIT-based timer.

---

## Environment Setup

### Arch Linux

```bash
sudo pacman -S nasm qemu gcc make gdb gcc-multilib
```

### Ubuntu / Debian

```bash
sudo apt install nasm qemu-system-x86 gcc make gdb gcc-multilib
```

### Tool Reference

| Tool         | Purpose                                      |
| ------------ | -------------------------------------------- |
| nasm         | Assembler — flat binary and ELF32 objects    |
| gcc          | Cross-compiler for 32-bit freestanding C     |
| gcc-multilib | 32-bit (`-m32`) support on 64-bit hosts      |
| ld           | Linker — produces flat binary kernel         |
| make         | Build system                                 |
| qemu         | x86 system emulator                          |
| gdb          | Source-level debugger (via QEMU remote stub) |

---

## Project Structure

```
my-os/
├── boot/
│   ├── stage_1.asm          # Stage 1 — 512-byte MBR, loads stage 2 from disk
│   ├── stage2.asm           # Stage 2 — GDT, A20, protected-mode entry, jumps to kernel
│   ├── keyboard-input.asm   # Legacy real-mode keyboard helper (stage 2 only)
│   └── print_my_name.asm    # Legacy real-mode print helper (stage 2 only)
│
├── kernel/
│   ├── kernel_entry.asm     # Protected-mode entry point, calls kernel_main
│   ├── kernel.c             # Kernel main, shell, VGA terminal
│   ├── linker.ld            # Flat binary layout — load/exec at 0x2000
│   │
│   ├── io.h                 # inb / outb / io_wait inline helpers
│   │
│   ├── idt.h                # idt_entry_t, idt_ptr_t, registers_t, idt_set_gate()
│   ├── idt.c                # IDT table init — idt_init(), hooks all 256 entries
│   │
│   ├── isr.asm              # CPU exception stubs ISR0–ISR31 (NASM ELF32 macros)
│   ├── irq.asm              # Hardware IRQ stubs IRQ0–IRQ15 (NASM ELF32 macros)
│   │
│   ├── pic.h                # PIC port constants, pic_remap(), pic_send_eoi()
│   ├── pic.c                # 8259A PIC init — ICW1-4 sequence, EOI
│   │
│   ├── keyboard.h           # Key definitions, keyboard_init(), keyboard_readline_irq()
│   ├── keyboard.c           # IRQ1 ring-buffer driver, scancode → ASCII
│   │
│   ├── timer.h              # timer_init(), timer_get_ticks()
│   └── timer.c              # IRQ0 PIT driver — 100 Hz tick counter
│
└── build/                   # Generated — all .o files + kernel.bin + os.img
```

---

## Build and Run

All commands run from the `my-os/` directory.

```bash
# Build everything (bootloader + kernel → floppy image)
make

# Run in QEMU
make run

# Open QEMU GDB stub on localhost:1234
make debug

# Remove all build artifacts
make clean
```

### QEMU Invocation (what `make run` executes)

```bash
qemu-system-i386 \
  -drive format=raw,file=build/os.img,if=floppy \
  -boot a \
  -m 32M
```

---

## Boot Sequence

```
BIOS
 └─ loads 512-byte MBR (stage_1.asm) at 0x7C00
     └─ reads stage2 sectors into 0x1000; jumps
         └─ stage2 (real mode): loads GDT, enables A20, sets CR0[PE], far-jumps
             └─ stage2 (32-bit): reads kernel sectors into 0x2000, jumps
                 └─ kernel_entry.asm: sets DS/ES/FS/GS/SS = 0x10, ESP = 0x90000
                     └─ kernel_main(): idt_init → pic_remap → keyboard_init → timer_init → sti → hlt loop
```

---

## Interrupt Architecture

| Component | Detail |
|-----------|--------|
| IDT | 256 entries; gate flags `0x8E` (32-bit interrupt gate, ring-0, present) |
| PIC remap | IRQ0–7 → INT `0x20`–`0x27` (master), IRQ8–15 → INT `0x28`–`0x2F` (slave) |
| IRQ0 | PIT timer — 100 Hz (`timer_init(100)`); `timer_get_ticks()` |
| IRQ1 | PS/2 Keyboard — ring buffer driver; `keyboard_readline_irq()` |
| ISR 0–31 | CPU exceptions (divide-by-zero, GPF, page fault …) |
| EOI | `outb(0x20, 0x20)`; slave also `outb(0xA0, 0x20)` |

---

## Memory Map

| Address | Content |
|---------|---------|
| `0x00000–0x003FF` | Real-mode IVT (BIOS) |
| `0x07C00–0x07DFF` | Stage 1 — 512-byte MBR |
| `0x01000–0x01FFF` | Stage 2 — loader / protected-mode entry |
| `0x02000–…` | Kernel flat binary (load + exec address) |
| `0x0B8000` | VGA text buffer — 80×25 colour cells |
| `0x90000` | Stack top (`ESP` initial value) |

GDT selectors: `0x08` = 32-bit code segment, `0x10` = data segment (base `0x0`, limit `0xFFFFFFFF`, ring-0).

---

## Author

**Ubaid Bin Waris** — Systems Programmer

[github.com/UbaidBinWaris](https://github.com/UbaidBinWaris)

---
