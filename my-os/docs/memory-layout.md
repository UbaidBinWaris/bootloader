# UbaidOS — Memory Layout

## Physical Address Space (x86 Real Mode — first 1 MB)

```
Physical Address    Size        Owner / Content
─────────────────────────────────────────────────────────────────────
0x00000–0x003FF     1 KB        IVT — Interrupt Vector Table (BIOS)
0x00400–0x004FF     256 B       BDA — BIOS Data Area
0x00500–0x007BFF    ~30 KB      Free conventional memory (unused)
0x007C00–0x007DFF   512 B       ★ Stage 1 — boot.asm (MBR)
0x007E00–0x00FFFF   ~32 KB      Free (stack grows down from 0x7C00)
0x01000–0x01FFF     4 KB        ★ Stage 2 — stage2.asm
0x02000–0x03FFF     8 KB        ★ Kernel binary (16 sectors × 512)
0x04000–0x7FFFF     ~480 KB     Free conventional memory
0x80000–0x9FFFF     128 KB      EBDA — Extended BIOS Data Area
0xA0000–0xAFFFF     64 KB       VGA graphics framebuffer
0xB0000–0xB7FFF     32 KB       Monochrome text framebuffer
0xB8000–0xBFFFF     32 KB       ★ VGA colour text mode (80×25)
0xC0000–0xC7FFF     32 KB       VGA BIOS ROM
0xC8000–0xEFFFF     160 KB      Expansion ROMs
0xF0000–0xFFFFF     64 KB       System BIOS ROM
```

> **Note**: Addresses above `0xFFFFF` (1 MB) require the A20 line to be enabled.

---

## UbaidOS Load Map

```
Physical Memory (simplified)
┌─────────────────────────────────────┐  0xFFFFF (1 MB boundary)
│         BIOS ROM / Reserved         │
├─────────────────────────────────────┤  0xB8000
│   VGA Text Framebuffer (80×25)      │  ← terminal writes here
│         2 bytes per cell            │
│     word = [char][attribute]        │
├─────────────────────────────────────┤  0xA0000
│         VGA Graphics (unused)       │
├─────────────────────────────────────┤  0x90000
│   ★ Kernel Stack Top (ESP)          │  ← stack grows downward ↓
│                                     │
│         (stack space)               │
│                                     │
├─────────────────────────────────────┤  0x04000
│         Free                        │
│                                     │
├─────────────────────────────────────┤  0x02000 + kernel size
│   kernel .bss                       │  ← zero-initialised globals
├─────────────────────────────────────┤
│   kernel .data                      │  ← initialised globals
├─────────────────────────────────────┤
│   kernel .rodata                    │  ← string literals, const data
├─────────────────────────────────────┤
│   kernel .text                      │  ← executable code
├─────────────────────────────────────┤  0x02000  ← kernel entry (_start)
│                                     │
├─────────────────────────────────────┤  0x01000  ← stage2 entry
│   Stage 2 — stage2.asm              │
├─────────────────────────────────────┤
│         Free                        │
├─────────────────────────────────────┤  0x007E00
│   Stage 1 — boot.asm (MBR)          │
├─────────────────────────────────────┤  0x007C00
│         Free / Stack area           │
├─────────────────────────────────────┤  0x000500
│   BIOS Data Area (BDA)              │
├─────────────────────────────────────┤  0x000400
│   Interrupt Vector Table (IVT)      │
└─────────────────────────────────────┘  0x000000
```

---

## Key Addresses Reference

| Symbol / Label       | Address    | Description |
|----------------------|------------|-------------|
| `0x7C00`             | `0x007C00` | BIOS loads MBR here; Stage 1 entry |
| `STAGE2_LOAD`        | `0x001000` | Stage 2 binary loaded here by Stage 1 |
| `KERNEL_LOAD`        | `0x002000` | Kernel binary loaded here by Stage 2 |
| `_start`             | `0x002000` | Kernel entry (`kernel_entry.asm`) |
| `kernel_main`        | `≥0x002000`| C kernel entry (linked after `_start`) |
| `_bss_start`         | dynamic    | Start of zero-init segment (linker symbol) |
| `_bss_end`           | dynamic    | End of zero-init segment (linker symbol) |
| `_kernel_end`        | dynamic    | First byte after kernel image |
| Stack bottom (ESP)   | `0x090000` | Kernel stack pointer initialised here |
| VGA framebuffer      | `0x0B8000` | 80×25 colour text mode output |

---

## VGA Text Mode Buffer Format

The VGA colour text buffer begins at physical address `0xB8000`.  
Each character cell is **2 bytes**:

```
Offset  Byte 0 (low)    Byte 1 (high)
        ASCII character  Colour attribute
```

### Colour Attribute Byte

```
  Bit:  7     6  5  4     3  2  1  0
       ┌─────┬────────────┬──────────┐
       │ Blink│  BG (3 bits)│ FG (4 bits) │
       └─────┴────────────┴──────────┘
```

| Bits | Field | Description |
|------|-------|-------------|
| 7    | Blink | 1 = blinking (or bright background on some modes) |
| 6–4  | Background | 3-bit background colour (0–7) |
| 3–0  | Foreground | 4-bit foreground colour (0–15) |

### VGA Colour Values

| Value | Colour         | Value | Colour              |
|-------|----------------|-------|---------------------|
| 0x0   | Black          | 0x8   | Dark Grey           |
| 0x1   | Blue           | 0x9   | Light Blue          |
| 0x2   | Green          | 0xA   | Light Green         |
| 0x3   | Cyan           | 0xB   | Light Cyan          |
| 0x4   | Red            | 0xC   | Light Red           |
| 0x5   | Magenta        | 0xD   | Light Magenta       |
| 0x6   | Brown          | 0xE   | Yellow              |
| 0x7   | Light Grey     | 0xF   | White               |

Default kernel colour: **Light Green (0xA) on Black (0x0)** → attribute `0x0A`

### Calculating Cell Offset
```c
// (row, col) → byte offset in the VGA buffer
uint16_t offset = (row * VGA_WIDTH + col) * 2;

// write character + attribute
vga_buffer[offset]     = ascii_char;
vga_buffer[offset + 1] = colour_attribute;
```

---

## Stack Layout

The kernel stack occupies memory between the kernel's BSS end and `0x90000`.
Because x86 stacks grow downward, `ESP` is initialised to `0x90000`
(the high address) and grows toward lower addresses:

```
0x90000  ← ESP initial value (set in stage2.asm init_pm)
   │
   │  grows downward on PUSH / CALL
   ▼
   ??? (stack bottom — never reaches kernel image at 0x2000 in practice)
```

**Stack size available**: `0x90000 − 0x4000 = ~352 KB`, far more than needed
for a simple shell kernel.

---

## Linker Script Section Map

The linker script `kernel/linker.ld` places sections in this order at `0x2000`:

```
0x2000 ──────────────────────────────────
         .text        executable code
         (ALIGN 4)
         .rodata      read-only data (string literals)
         (ALIGN 4)
         .data        initialised global variables
         (ALIGN 4)
_bss_start:
         .bss         zero-initialised globals
         *(COMMON)    tentative definitions
_bss_end:
_kernel_end:
```

The symbols `_bss_start`, `_bss_end`, and `_kernel_end` are consumed by
`kernel_entry.asm` to zero the BSS before calling `kernel_main`.
