# UbaidOS — Boot Process

## Overview

The boot process is the sequence of events from BIOS firmware handoff to the
running C kernel. UbaidOS uses a classic two-stage bootloader to work around
the 512-byte Stage 1 size limit.

---

## Step-by-Step Sequence

```
Power On
   │
   ▼
BIOS POST & Hardware Init
   │  Finds bootable disk
   │  Reads LBA 0 (512 bytes) → physical 0x7C00
   ▼
Stage 1 — boot.asm  (0x7C00)        [16-bit real mode]
   │  Far jump to normalise CS = 0x0000
   │  Set up DS, ES, SS = 0x0000 ; SP = 0x7C00
   │  Save boot drive number (DL)
   │  Print ASCII banner + name prompt
   │  Read name from keyboard (INT 16h)
   │  Print greeting: "Hello <name>!"
   │  INT 13h: load sector 1 → 0x0000:0x1000  (Stage 2)
   │  Far jump → 0x0000:0x1000
   ▼
Stage 2 — stage2.asm  (0x1000)      [16-bit real mode → 32-bit protected]
   │  INT 10h: print status messages
   │  INT 13h: load sectors 2–17 → 0x0000:0x2000  (kernel, 16 sectors)
   │  Enable A20 via port 0x92
   │  lgdt [gdt_descriptor]
   │  cli
   │  mov cr0, eax  (set PE bit)
   │  far jmp CODE_SEG:init_pm
   ▼
Stage 2 — init_pm:                  [32-bit protected mode]
   │  Set all segment regs → DATA_SEG (0x10)
   │  mov esp, 0x90000
   │  jmp CODE_SEG:0x2000
   ▼
kernel_entry.asm  (_start at 0x2000) [32-bit]
   │  cld
   │  Zero BSS: rep stosb from _bss_start → _bss_end
   │  call kernel_main
   │  cli + hlt  (safety halt if main returns)
   ▼
kernel.c — kernel_main()
   │  terminal_init()      ← clear VGA buffer, set default colour
   │  print_banner()       ← ASCII art welcome
   │  shell_run()          ← interactive prompt loop
   └─ (never returns)
```

---

## Disk Image Layout

The final `os.img` is a 1.44 MB floppy image assembled by `dd`:

| Sector(s) | LBA  | Content     | Physical Address |
|-----------|------|-------------|------------------|
| 0         | 0    | `boot.bin`  | Loaded to `0x7C00` by BIOS |
| 1         | 1    | `stage2.bin`| Loaded to `0x1000` by Stage 1 |
| 2–17      | 2–17 | `kernel.bin`| Loaded to `0x2000` by Stage 2 |
| 18–2879   | …    | (unused)    | — |

```
Disk Image (os.img)
┌──────────┬──────────┬─────────────────────────┬──────────────────────┐
│ Sector 0 │ Sector 1 │    Sectors 2–17         │   Sectors 18–2879   │
│ boot.bin │stage2.bin│      kernel.bin         │       (empty)       │
│  512 B   │  ~512 B  │  up to 8 KB (16 × 512) │                     │
└──────────┴──────────┴─────────────────────────┴──────────────────────┘
  0x0000      0x0200       0x0400                   0x2400
```

---

## Stage 1 Detail — `boot.asm`

### Entry Conditions (set by BIOS)
- `CS:IP` = `0xF000:0xFFF0` initially then jumps to `0x0000:0x7C00`
- `DL` = boot drive number (e.g. `0x00` for floppy, `0x80` for first HDD)
- CPU is in 16-bit real mode

### Segment Normalisation
The BIOS may load us as `0x07C0:0x0000` or `0x0000:0x7C00`. A far jump
fixes `CS` to a known value:
```nasm
jmp 0x0000:start          ; normalise CS = 0, IP = offset of start
```

### Disk Read with Retry
```
for attempt in 0..3:
    INT 13h AH=0x02  (read sectors)
    if CF clear → success, break
    INT 13h AH=0x00  (reset drive)
on failure: print error + jmp $
```

### Boot Signature
The assembler fills the last two bytes of the 512-byte output with `0x55 0xAA`
via:
```nasm
times 510-($-$$) db 0
dw 0xAA55
```

---

## Stage 2 Detail — `stage2.asm`

### A20 Line (Fast Method)
The A20 gate historically prevented address line 21 from propagating,
wrapping memory access at 1 MB. Modern systems support "Fast A20" via port `0x92`:
```nasm
in   al, 0x92
or   al, 0x02     ; set bit 1 (A20 enable)
and  al, 0xFE     ; clear bit 0 (avoid fast reset)
out  0x92, al
```

### Global Descriptor Table
Three 8-byte descriptors are defined in the data section:

| Index | Selector | Base    | Limit   | Type             |
|-------|----------|---------|---------|------------------|
| 0     | `0x00`   | —       | —       | Null (required)  |
| 1     | `0x08`   | `0x0`   | `0xFFFFF` | Code, ring 0, 32-bit |
| 2     | `0x10`   | `0x0`   | `0xFFFFF` | Data, ring 0, 32-bit |

### Protected Mode Switch
```nasm
cli                      ; disable interrupts
lgdt [gdt_descriptor]    ; load GDTR
mov  eax, cr0
or   eax, 0x1            ; set PE (Protection Enable) bit
mov  cr0, eax
jmp  CODE_SEG:init_pm    ; far jump flushes the instruction pipeline
                         ; and reloads CS from GDT
```

After the far jump, the CPU is in 32-bit protected mode and BIOS interrupts
are no longer available.

---

## Kernel Entry Detail — `kernel_entry.asm`

### BSS Zero-Init
The C standard requires zero-initialised storage for global and static
variables. The linker script exposes `_bss_start` and `_bss_end` symbols:
```nasm
mov  edi, _bss_start
mov  ecx, _bss_end
sub  ecx, edi             ; byte count
xor  eax, eax             ; fill value = 0
rep  stosb                ; zero-fill
```

Without this step, uninitialised global variables would contain garbage
from whatever was in memory before the kernel loaded.

---

## Common Boot Failures

| Symptom | Likely Cause |
|---|---|
| `No bootable device` / triple fault instantly | `0xAA55` signature missing or boot.bin > 512 bytes |
| Hangs after Stage 1 banner | Disk read failed; check `STAGE2_SECTOR` value in boot.asm |
| Triple fault in protected mode | GDT selector values wrong, or stack not set before entering pm |
| Kernel code executes garbage | Kernel sectors not loaded, or linker origin address ≠ load address |
| Screen blank after kernel entry | `terminal_init()` not called or VGA base address wrong (`0xB8000`) |
