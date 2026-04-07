# ▶️ Build & Run

## Prerequisites

| Tool | Purpose |
|------|---------|
| `nasm` ≥ 2.14 | Assembles `.asm` → flat binary + ELF32 objects |
| `gcc` (i686-elf or native with `-m32`) | Compiles the 32-bit C kernel |
| `ld` (binutils) | Links kernel objects into flat binary |
| `qemu-system-i386` | Runs the OS image |
| `make` | Orchestrates the entire build |

---

## Build

```bash
# from repo root  (my-os/)
make
```

What `make` does:

1. Assembles `boot/stage_1.asm` → `build/stage1.bin` (flat, 512 bytes)
2. Assembles `boot/stage2.asm` → `build/stage2.bin` (flat)
3. Assembles `kernel/kernel_entry.asm`, `kernel/isr.asm`, `kernel/irq.asm` → ELF32 `.o` objects
4. Compiles `kernel/kernel.c`, `kernel/idt.c`, `kernel/pic.c`, `kernel/keyboard.c`, `kernel/timer.c` → ELF32 `.o` objects
5. Links all kernel objects with `kernel/linker.ld` → `build/kernel.bin` (flat)
6. Concatenates `stage1.bin + stage2.bin + kernel.bin` → `build/os.img`

---

## Run

```bash
make run
```

Internally runs:

```bash
qemu-system-i386 \
  -drive format=raw,file=build/os.img,if=floppy \
  -boot a \
  -m 32M
```

---

## Debug (GDB + QEMU)

```bash
make debug
```

This starts QEMU paused and listening on port 1234:

```bash
qemu-system-i386 -s -S \
  -drive format=raw,file=build/os.img,if=floppy \
  -boot a -m 32M &
```

Attach GDB:

```bash
gdb \
  -ex "target remote :1234" \
  -ex "set arch i386" \
  -ex "symbol-file build/kernel.elf"
```

Useful GDB commands:

```
(gdb) break kernel_main      # breakpoint at C entry
(gdb) continue               # run until breakpoint
(gdb) info registers         # dump all CPU registers
(gdb) x/10i $eip             # disassemble at current PC
(gdb) x/32xb 0xb8000         # inspect VGA buffer
(gdb) stepi                  # single-step one instruction
```

---

## Clean

```bash
make clean
```

Deletes all `build/*.bin`, `build/*.o`, `build/*.img`, `build/*.elf`.

---

## Expected Boot Output

```
UbaidOS v1.0 — 32-bit Protected Mode
IDT: 256 gates loaded
PIC: remapped (IRQ0→0x20, IRQ8→0x28)
Keyboard: IRQ1 driver ready
Timer: 100 Hz PIT initialised
STI — interrupts enabled

ubaidOS> _
```

Type `help` at the prompt to list supported commands.
