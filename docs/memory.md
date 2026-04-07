# 🧠 Memory Layout

## Real Mode vs Protected Mode

| Mode | Addressing | Max RAM | Segment equation |
|------|-----------|---------|------------------|
| Real mode (16-bit) | Segment : Offset | 1 MB | `segment * 16 + offset` |
| Protected mode (32-bit) | Flat virtual | 4 GB | Descriptor-based |

UbaidOS starts in real mode and switches to 32-bit protected mode before entering the C kernel. **All kernel code runs with paging disabled** — virtual address = physical address throughout.

---

## Full Memory Map

```
+---------------------------+ 0x00100000  (1 MB)
|  Extended memory (free)   |
| ...                       |
+---------------------------+ 0x00090000
|  Stack  (grows downward)  |  ESP initial = 0x90000
+---------------------------+ 0x00020000  (or wherever kernel ends)
|  Kernel flat binary       |  loaded + executed at 0x2000
+---------------------------+ 0x00002000
|  Stage 2 loader           |  loaded at 0x1000 by stage 1
+---------------------------+ 0x00001000
|  (gap)                    |
+---------------------------+ 0x00007E00
|  Stage 1 code (MBR)       |  BIOS loads here: 512 bytes
+---------------------------+ 0x00007C00
|  Free                     |
+---------------------------+ 0x00000500
|  BIOS Data Area (BDA)     |
+---------------------------+ 0x00000400
|  Real-mode IVT            |  256 × 4-byte vectors
+---------------------------+ 0x00000000
```

---

## GDT Layout (Global Descriptor Table)

Defined in `stage2.asm`, loaded with `lgdt`. Three entries:

| Index | Selector | Type | Base | Limit | Flags |
|-------|----------|------|------|-------|-------|
| 0 | `0x00` | Null descriptor | — | — | — |
| 1 | `0x08` | Code segment | `0x00000000` | `0xFFFFFFFF` | R/X, ring-0, 32-bit, 4 KB granularity |
| 2 | `0x10` | Data segment | `0x00000000` | `0xFFFFFFFF` | R/W, ring-0, 32-bit, 4 KB granularity |

Both code and data segements span the entire 4 GB address space (flat model).

---

## IDT Location

The IDT (`idt_entries[256]`) is a BSS/data array inside the kernel binary at `0x2000+`. Its physical address is passed to the CPU via:

```c
struct { uint16_t limit; uint32_t base; } idt_ptr;
idt_ptr.limit = 256 * sizeof(idt_entry_t) - 1;  // 2047
idt_ptr.base  = (uint32_t)idt_entries;
lidt(&idt_ptr);
```

---

## VGA Text Buffer

| Address | Size | Content |
|---------|------|---------|
| `0x000B8000` | 4000 bytes | 80×25 colour character cells |

Each cell is 2 bytes: `[colour byte][ASCII byte]`. The kernel writes directly to this physical address — no BIOS call needed.

---

## Stack

| Register | Value | Note |
|----------|-------|------|
| `ESP` (initial) | `0x90000` | Set in `kernel_entry.asm` |
| Stack direction | Grows downward | Standard x86 convention |

The stack lives in the free region between the kernel image and `0x90000`. Approximately **440 KB** of stack space is available before the stack would collide with the kernel.

* Max memory: 1MB
* No protection
* No virtual memory

---

## 🚀 Future Upgrade

Switch to:

* Protected Mode (32-bit)
* Paging
* Virtual memory

---

## 💡 Key Insight

Memory control = OS power

This is what separates:

* Normal developers ❌
* System engineers ✅

---
