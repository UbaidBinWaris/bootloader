# UbaidOS — Protected Mode & the GDT

## What Is Protected Mode?

The x86 processor starts in **Real Mode**: a 16-bit execution environment that
closely mimics the original 8086. It offers direct physical memory access via
20-bit segmented addressing (segment × 16 + offset) but provides no memory
protection, ring levels, or paging.

**Protected Mode** is the 32-bit operating mode introduced with the 80286 and
standardised on the 80386. Key differences:

| Feature          | Real Mode              | Protected Mode          |
|------------------|------------------------|-------------------------|
| Address bus      | 20-bit (~1 MB)         | 32-bit (4 GB)           |
| Registers        | 16-bit                 | 32-bit (EAX, EBX, …)   |
| Memory model     | Segment × 16 + Offset  | GDT/LDT descriptor      |
| Memory protection| None                   | Ring 0–3, segment limits|
| Paging           | Not available          | Optional (CR0.PG)       |
| BIOS interrupts  | Available              | **Not available**       |

UbaidOS uses **32-bit protected mode with a flat memory model** — all
segments cover the full 4 GB address space, effectively making virtual
addresses equal to physical addresses.

---

## The Global Descriptor Table (GDT)

The GDT is an array of 8-byte **segment descriptors** stored anywhere in
memory. The CPU finds it via the **GDTR** register, loaded with `lgdt`.

### UbaidOS GDT Layout

```
GDT Index  Selector  Descriptor
─────────────────────────────────────────
0          0x0000    Null descriptor (required by x86 architecture)
1          0x0008    Code segment — ring 0, 32-bit, 4 GB flat
2          0x0010    Data segment — ring 0, 32-bit, 4 GB flat
```

A **selector** is the value loaded into a segment register (e.g. `CS`, `DS`).
It encodes: `index × 8 | TI bit | RPL`. For ring-0 GDT entries:
- Code segment selector: `0x08` (index 1, TI=0, RPL=0)
- Data segment selector: `0x10` (index 2, TI=0, RPL=0)

---

## Segment Descriptor Format

Each descriptor is exactly 8 bytes with a split layout for historical
compatibility with 80286:

```
 63      56 55  52 51   48 47    40 39      16 15       0
┌──────────┬──────┬───────┬────────┬──────────┬──────────┐
│ Base 31:24│Flags │Limit  │ Access │ Base 23:0│ Limit    │
│          │      │19:16  │  Byte  │          │  15:0    │
└──────────┴──────┴───────┴────────┴──────────┴──────────┘
  1 byte    4 bits  4 bits  1 byte   3 bytes    2 bytes
```

### Access Byte (bits 47–40)

```
  Bit:  7     6  5    4    3    2    1    0
       ┌─────┬─────┬─────┬────┬────┬────┬────┐
       │  P  │ DPL │  S  │  E │ DC │ RW │ A  │
       └─────┴─────┴─────┴────┴────┴────┴────┘
```

| Bit | Name | Meaning |
|-----|------|---------|
| 7   | P    | Present — must be 1 for valid descriptors |
| 6–5 | DPL  | Descriptor Privilege Level (0 = kernel, 3 = user) |
| 4   | S    | Segment type: 1 = code/data, 0 = system |
| 3   | E    | Executable: 1 = code segment, 0 = data |
| 2   | DC   | Direction/Conforming |
| 1   | RW   | Readable (code) / Writable (data) |
| 0   | A    | Accessed — CPU sets this automatically |

UbaidOS code descriptor: `0x9A` = `1001_1010b`
- P=1, DPL=00 (ring 0), S=1, E=1 (code), DC=0, R=1, A=0

UbaidOS data descriptor: `0x92` = `1001_0010b`
- P=1, DPL=00 (ring 0), S=1, E=0 (data), DC=0, W=1, A=0

### Flags Nibble (bits 55–52)

```
  Bit:  3    2    1    0
       ┌────┬────┬────┬────┐
       │ G  │ DB │  L │    │
       └────┴────┴────┴────┘
```

| Bit | Name | Meaning |
|-----|------|---------|
| 3   | G    | Granularity: 0 = byte, 1 = 4 KB pages |
| 2   | DB   | Default operation size: 0 = 16-bit, 1 = 32-bit |
| 1   | L    | 64-bit code segment (0 for 32-bit mode) |
| 0   | —    | Reserved, must be 0 |

UbaidOS both descriptors use flags `0xC` = `1100b` (G=1 → 4 KB granularity, DB=1 → 32-bit).

---

## GDT in NASM Source

```nasm
gdt_start:
    ; Null descriptor (8 zero bytes — required)
    dd 0x0
    dd 0x0

gdt_code:
    ; Code segment: base=0, limit=0xFFFFF, ring 0, 32-bit
    dw 0xFFFF        ; Limit 15:0
    dw 0x0000        ; Base  15:0
    db 0x00          ; Base  23:16
    db 0x9A          ; Access byte (P=1, DPL=0, S=1, E=1, RW=1)
    db 0xCF          ; Flags (G=1, DB=1) | Limit 19:16 (0xF)
    db 0x00          ; Base  31:24

gdt_data:
    ; Data segment: base=0, limit=0xFFFFF, ring 0, 32-bit
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92          ; Access byte (P=1, DPL=0, S=1, E=0, RW=1)
    db 0xCF
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; GDT size (limit) — 1 less than actual
    dd gdt_start                  ; GDT linear base address
```

The **GDTR** structure loaded by `lgdt` is 6 bytes: a 16-bit limit followed by
a 32-bit linear base address.

---

## The Protected Mode Switch Sequence

Stage 2 performs the switch in this exact order:

```nasm
; 1. Load the GDT
lgdt [gdt_descriptor]

; 2. Disable interrupts (BIOS IVT is 16-bit; IDT not yet set up)
cli

; 3. Set CR0 Protection Enable (PE) bit
mov  eax, cr0
or   eax, 0x1       ; bit 0 = PE
mov  cr0, eax

; 4. Far jump (flushes instruction pipeline, reloads CS from GDT)
jmp  CODE_SEG:init_pm       ; CODE_SEG = 0x08

; --- CPU is now in 32-bit protected mode ---

[bits 32]
init_pm:
    ; 5. Reload all data segment registers
    mov  ax, DATA_SEG       ; DATA_SEG = 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax

    ; 6. Set up a valid stack
    mov  esp, 0x90000

    ; 7. Jump to kernel
    jmp  CODE_SEG:0x2000
```

### Why the Far Jump Is Critical

After setting CR0.PE, the CPU pipeline may contain instructions decoded in
real mode. The far jump (`jmp CODE_SEG:target`) forces a **pipeline flush**
and loads `CS` from the GDT instead of using the real-mode × 16 calculation.
Without it, the CPU would execute garbage.

---

## Why BIOS Interrupts Stop Working

In protected mode:
- The 16-bit IVT (Interrupt Vector Table) at `0x0000` is no longer valid
- Interrupt handling requires an **IDT** (Interrupt Descriptor Table) loaded via `lidt`
- UbaidOS does not set up an IDT, so **interrupts remain disabled** (`cli` stays in effect)
- The PS/2 keyboard driver therefore uses **polling** (busy-wait on port `0x64`) rather than IRQ 1

---

## Flat Memory Model Explained

UbaidOS uses a **flat memory model** where all segment descriptors have:
- Base address = `0x00000000`
- Limit = `0xFFFFF` with G=1 (granularity = 4 KB page), so effective limit = `0xFFFFFFFF` (4 GB)

This means all segment registers (CS, DS, ES, SS) map to the same flat
4 GB physical address space. A C pointer value equals its physical address —
no segmentation arithmetic needed.

```
Logical Address     Effective Physical Address
───────────────    ────────────────────────────
DS:0x00002000   →  0x00000000 + 0x00002000 = 0x00002000
CS:0x00002000   →  0x00000000 + 0x00002000 = 0x00002000
```

This makes it straightforward to write `kernel.c` using ordinary C pointers
while still technically using protected-mode segmentation.

---

## Next Steps (Not Yet Implemented)

To extend UbaidOS toward a real OS, these pieces would follow:

| Feature | Mechanism Required |
|---|---|
| Interrupt handling | IDT + `lidt` + ISR stubs + `sti` |
| Timer / preemption | IRQ 0 (PIT) handler via IDT |
| Keyboard interrupts | IRQ 1 handler instead of polling |
| Virtual memory / ASLR | CR3 + page directory/tables + CR0.PG |
| User-space processes | Ring 3 segments + TSS + `iret` |
| System calls | `int 0x80` or `syscall` instruction |
| File system | Disk driver + FAT or custom FS |
