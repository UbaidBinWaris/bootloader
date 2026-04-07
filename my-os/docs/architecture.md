# UbaidOS — Architecture Overview

## Design Philosophy

UbaidOS is a minimal x86 operating system built from scratch to demonstrate
the complete software stack from BIOS firmware handoff to a running interactive
kernel shell. Every layer is hand-written with no external runtime dependencies.

---

## Component Diagram

```
┌─────────────────────────────────────────────────────┐
│                     HARDWARE                        │
│   CPU (x86)  │  RAM  │  Disk  │  VGA  │  PS/2 KB   │
└────────┬─────┴───┬───┴───┬────┴───┬───┴────┬────────┘
         │         │       │        │        │
         ▼         │       │        │        │
┌────────────────┐ │       │        │        │
│  BIOS Firmware │ │       │        │        │
│  INT 13h disk  │ │       │        │        │
│  INT 10h video │ │       │        │        │
└───────┬────────┘ │       │        │        │
        │loads     │       │        │        │
        ▼          │       │        │        │
┌────────────────┐ │       │        │        │
│  Stage 1 MBR   │ │       │        │        │
│  boot.asm      │ │       │        │        │
│  0x7C00–0x7DFF │ │       │        │        │
│  512 bytes     │ │       │        │        │
└───────┬────────┘ │       │        │        │
        │loads     │       │        │        │
        ▼          │       │        │        │
┌────────────────┐ │       │        │        │
│  Stage 2 Loader│ │       │        │        │
│  stage2.asm    │ │       │        │        │
│  0x1000        │ │       │        │        │
│  A20 + GDT +   │ │       │        │        │
│  Protected Mode│ │       │        │        │
└───────┬────────┘ │       │        │        │
        │jumps to  │       │        │        │
        ▼          ▼       ▼        ▼        ▼
┌─────────────────────────────────────────────────────┐
│                  KERNEL (32-bit Protected Mode)      │
│  kernel_entry.asm → kernel.c                        │
│  Load address: 0x2000                               │
│                                                     │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────┐ │
│  │ VGA Terminal│  │ PS/2 Keyboard│  │   Shell    │ │
│  │  Driver     │  │   Driver     │  │  (6 cmds)  │ │
│  └─────────────┘  └──────────────┘  └────────────┘ │
└─────────────────────────────────────────────────────┘
```

---

## Layers

### Layer 0 — BIOS Firmware
The BIOS performs Power-On Self-Test (POST), initialises hardware, and loads
the first 512-byte sector of the boot disk to physical address `0x7C00`.
Execution then jumps there. The BIOS provides 16-bit interrupt services
(INT 10h for video, INT 13h for disk) which are available only in real mode.

### Layer 1 — Stage 1 (MBR)  `boot/boot.asm`
- **Size limit**: exactly 512 bytes (sector 0 of disk image)
- **Mode**: 16-bit real mode
- **Responsibility**:
  - Normalise `CS:IP` to `0x0000:0x7C00` via far jump
  - Set up segments and stack (`SS:SP = 0x0000:0x7C00`)
  - Save the BIOS boot drive number from `DL`
  - Display ASCII banner and prompt for user name
  - Load Stage 2 from sector 1 into `0x1000` using INT 13h (with 3-retry loop)
  - Far jump to `0x0000:0x1000`
- **Boot signature**: last 2 bytes must be `0x55 0xAA`

### Layer 2 — Stage 2 (Loader)  `boot/stage2.asm`
- **Size**: no hard limit (loaded by Stage 1, up to several sectors)
- **Mode**: starts 16-bit real mode → transitions to 32-bit protected mode
- **Responsibility**:
  - Load kernel binary (16 sectors, 8 KB) into physical `0x2000`
  - Enable A20 line via port `0x92` (fast A20 method)
  - Define and load a flat-model GDT (`lgdt`)
  - Set CR0 PE bit and far jump to 32-bit code segment
  - In protected mode: initialise segment registers and stack (`ESP = 0x90000`)
  - Jump to kernel entry at `0x2000`

### Layer 3 — Kernel Entry  `kernel/kernel_entry.asm`
- 32-bit ASM bridge between the raw binary entry point and C
- Zero-initialises the BSS segment using `rep stosb`
- Calls `kernel_main()`
- Provides a halt loop (`cli; hlt`) as a safety fallback

### Layer 4 — Kernel  `kernel/kernel.c`
- **VGA Terminal Driver**: Manages the 80×25 VGA text-mode framebuffer at
  `0xB8000`. Supports scrolling, colour attributes, special characters
  (`\n`, `\r`, `\b`, `\t`).
- **I/O Port Helpers**: `inb()` / `outb()` wrappers around inline assembly
  for talking to memory-mapped I/O ports.
- **PS/2 Keyboard Driver**: Polls the keyboard controller (ports `0x60`/`0x64`)
  and translates Set 1 scancodes to ASCII, including Shift and Caps Lock.
- **String Utilities**: Freestanding `kstrlen`, `kstrcmp`, `kstrncmp`,
  `kskip_spaces` — no libc available.
- **Interactive Shell**: Prompt loop displaying `ubaid@UbaidOS:~$`,
  dispatches user input to built-in command handlers.

---

## Key Design Decisions

| Decision | Rationale |
|---|---|
| Two-stage bootloader | Stage 1 is size-limited to 512 bytes; Stage 2 handles the complex GDT/A20/protected-mode setup |
| Flat memory model | Simplifies addressing — code, data, and stack all share the same 4 GB descriptor |
| Freestanding GCC | No libc; only `<stdint.h>` and `<stddef.h>` from the compiler itself |
| Binary output from linker | Kernel is a raw flat binary, not ELF, so it can be `jmp`-ed into directly |
| Fast A20 (port 0x92) | Available on most modern hardware and emulators; simplest reliable method |
| Polling keyboard | Avoids needing an IDT and interrupt handlers for the demo shell |

---

## Source File Map

```
my-os/
├── Makefile                  ← build system
├── boot/
│   ├── boot.asm              ← Stage 1 MBR (16-bit, ORG 0x7C00)
│   └── stage2.asm            ← Stage 2 loader (16→32-bit, ORG 0x1000)
├── kernel/
│   ├── kernel_entry.asm      ← 32-bit entry stub, BSS zeroing
│   ├── kernel.c              ← VGA + keyboard + shell (freestanding C)
│   └── linker.ld             ← flat binary at 0x2000, BSS symbols
└── build/                    ← generated artefacts (gitignored)
```
