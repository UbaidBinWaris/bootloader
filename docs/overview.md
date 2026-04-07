# 🧠 System Overview

## What is UbaidOS?

UbaidOS is a hand-built, 32-bit protected-mode operating system written entirely from scratch in x86 NASM assembly and C. It has no dependency on any BIOS interrupt after the boot stage — all hardware communication in the kernel happens through direct port I/O and CPU instructions.

---

## Full Boot Chain

```
Power on
 └─ BIOS POST
     └─ BIOS loads MBR (512 bytes) into 0x7C00, jumps
         └─ stage_1.asm (Real Mode)
             • prints boot message
             • uses INT 0x13 to load stage2 sectors → 0x1000
             • far-jumps to 0x1000
         └─ stage2.asm (Real Mode → Protected Mode transition)
             • loads GDT descriptor
             • enables A20 line (keyboard controller method)
             • sets CR0[PE] = 1  (protected mode ON)
             • far-jump to 0x08:pm_entry  (flushes pipeline)
         └─ stage2.asm (32-bit protected mode)
             • segment registers → 0x10 (data descriptor)
             • loads kernel sectors from disk → 0x2000
             • jumps to 0x2000
         └─ kernel_entry.asm
             • sets DS/ES/FS/GS/SS = 0x10
             • sets ESP = 0x90000 (stack top)
             • calls kernel_main  (C symbol)
         └─ kernel_main()  (kernel.c)
             idt_init()        — build 256 IDT entries, load IDTR
             pic_remap(0x20, 0x28) — remap 8259A PIC vectors
             keyboard_init()   — unmask IRQ1, init ring buffer
             timer_init(100)   — set PIT to 100 Hz (IRQ0)
             sti               — enable hardware interrupts
             hlt loop          — CPU idles; hardware drives execution
```

---

## Privilege Architecture

| Ring | Used for | Notes |
|------|----------|-------|
| Ring 0 | Kernel + all drivers | Full CPU access |
| Ring 1–2 | Not used | — |
| Ring 3 | Future userspace | Not yet implemented |

All code currently runs in **ring 0 (kernel mode)**.

---

## Hardware Drivers

| Driver | File(s) | IRQ | Vector |
|--------|---------|-----|--------|
| PIT timer | `timer.c` | IRQ0 | `0x20` |
| PS/2 Keyboard | `keyboard.c` | IRQ1 | `0x21` |
| CPU exceptions | `isr.asm` | — | `0x00`–`0x1F` |

---

## Key Design Decisions

- **Flat binary kernel** — linker outputs a raw binary at origin `0x2000`; no ELF loader needed
- **No standard library** — `gcc -ffreestanding -nostdlib -nostdinc`; no `printf`, `malloc`, etc.
- **Interrupt-driven I/O** — keyboard input via IRQ1 ring buffer, not `INT 0x16` polling
- **No virtual memory** — paging off; physical = virtual addresses throughout
- **32-bit only** — `nasm -f elf32`, `gcc -m32`, `ld -m elf_i386`; 64-bit long mode not used
