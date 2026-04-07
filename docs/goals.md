# 🎯 Project Goals

## Status Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Completed |
| 🚧 | In progress |
| ☐ | Planned |

---

## Stage 1 — Real-Mode Bootloader

| Goal | Status |
|------|--------|
| BIOS boot to 0x7C00 | ✅ |
| Print string via INT 0x10 | ✅ |
| Keyboard input via INT 0x16 | ✅ |
| Screen clear / colour text | ✅ |
| Two-stage loader (stage 1 + stage 2) | ✅ |
| Read sectors from disk (INT 0x13) | ✅ |

---

## Stage 2 — Protected-Mode Entry

| Goal | Status |
|------|--------|
| Build GDT with code + data segments | ✅ |
| Enable A20 line | ✅ |
| Switch CR0[PE] to protected mode | ✅ |
| Far-jump to flush pipeline | ✅ |
| Set segment registers to flat data selector | ✅ |
| Load kernel to 0x2000, jump | ✅ |

---

## Stage 3 — C Kernel Foundation

| Goal | Status |
|------|--------|
| 32-bit protected-mode C kernel | ✅ |
| VGA text driver (direct 0xB8000 writes) | ✅ |
| Kernel shell (`help`, `clear`, `hello`, `reboot`, `uptime`) | ✅ |
| print / println / print_hex utility functions | ✅ |

---

## Stage 4 — Interrupt Subsystem

| Goal | Status |
|------|--------|
| IDT (256 entries, `lidt`) | ✅ |
| CPU exception stubs (ISR 0–31) | ✅ |
| 8259A PIC remap to 0x20/0x28 | ✅ |
| Hardware IRQ stubs (IRQ 0–15) | ✅ |
| PIT timer driver (IRQ0, 100 Hz) | ✅ |
| PS/2 keyboard driver (IRQ1, ring buffer) | ✅ |

---

## Stage 5 — Near-Term Goals

| Goal | Status |
|------|--------|
| Memory manager (physical frame allocator) | ☐ |
| Paging (4 KB pages, identity-mapped initially) | ☐ |
| ATA/PIO disk driver (replace BIOS INT 0x13) | ☐ |
| FAT16 / simple filesystem | ☐ |
| ELF executable loader | ☐ |

---

## Stage 6 — Long-Term Goals

| Goal | Status |
|------|--------|
| Userspace (ring-3 processes) | ☐ |
| System call interface | ☐ |
| Task scheduler (preemptive, round-robin) | ☐ |
| Virtual memory (`mmap`, COW fork) | ☐ |
| ACPI / PCI enumeration | ☐ |
| USB keyboard (replace PS/2) | ☐ |
| Network stack (RTL8139 driver) | ☐ |

---

## Final Goal

Build a fully self-hosted operating system capable of running user programs compiled for the UbaidOS ABI.
