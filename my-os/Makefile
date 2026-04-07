# =============================================================================
# UbaidOS — Professional Makefile
# Target: x86, BIOS boot, 32-bit protected mode kernel
#
# Usage:
#   make        — build everything
#   make run    — build and launch in QEMU
#   make debug  — build and launch QEMU with GDB server on :1234
#   make clean  — remove all build artefacts
# =============================================================================

# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------
NASM    := nasm
CC      := gcc
LD      := ld
QEMU    := qemu-system-i386

# ---------------------------------------------------------------------------
# Flags
# ---------------------------------------------------------------------------
# -m32          : generate 32-bit code
# -ffreestanding: no stdlib, no startup files
# -fno-pie      : no position-independent executable
# -nostdlib     : do not link against the standard C library
# -O2           : optimize (safe for freestanding code)
# -Wall -Wextra : full warnings
CFLAGS  := -m32 -ffreestanding -fno-pie -fno-stack-protector \
           -nostdlib -nostdinc -Wall -Wextra -O2

LDFLAGS := -m elf_i386 -T kernel/linker.ld --oformat binary

NASMFLAGS := -f bin

# ---------------------------------------------------------------------------
# Directories
# ---------------------------------------------------------------------------
BUILD   := build
BOOT    := boot
KERNEL  := kernel

# ---------------------------------------------------------------------------
# Output artefacts
# ---------------------------------------------------------------------------
BOOT_BIN    := $(BUILD)/boot.bin
STAGE2_BIN  := $(BUILD)/stage2.bin
KERNEL_BIN  := $(BUILD)/kernel.bin
KERNEL_ELF  := $(BUILD)/kernel.elf
DISK_IMG    := $(BUILD)/os.img

# ---------------------------------------------------------------------------
# Kernel C sources
# ---------------------------------------------------------------------------
KERNEL_SRCS := $(KERNEL)/kernel.c    \
               $(KERNEL)/idt.c       \
               $(KERNEL)/pic.c       \
               $(KERNEL)/keyboard.c  \
               $(KERNEL)/timer.c
KERNEL_OBJS := $(patsubst $(KERNEL)/%.c, $(BUILD)/%.o, $(KERNEL_SRCS))

# ---------------------------------------------------------------------------
# Kernel ASM objects (ELF32 — ISR/IRQ stubs)
# ---------------------------------------------------------------------------
KERNEL_ENTRY_OBJ := $(BUILD)/kernel_entry.o
KERNEL_ASM_OBJS  := $(BUILD)/isr.o $(BUILD)/irq.o

# ---------------------------------------------------------------------------
# Default target
# ---------------------------------------------------------------------------
.PHONY: all run debug clean

all: $(DISK_IMG)
	@echo ""
	@echo "  ╔══════════════════════════════╗"
	@echo "  ║   UbaidOS — Build Complete   ║"
	@echo "  ╚══════════════════════════════╝"
	@echo "  Disk image: $(DISK_IMG)"
	@echo "  Run with:   make run"
	@echo ""

# ---------------------------------------------------------------------------
# Build directory
# ---------------------------------------------------------------------------
$(BUILD):
	@mkdir -p $(BUILD)

# ---------------------------------------------------------------------------
# Stage 1 Bootloader (512 bytes, MBR)
# ---------------------------------------------------------------------------
$(BOOT_BIN): $(BOOT)/boot.asm | $(BUILD)
	@echo "[NASM ] $< → $@"
	$(NASM) $(NASMFLAGS) $< -o $@

# ---------------------------------------------------------------------------
# Stage 2 Loader (lands at 0x1000, sets up protected mode)
# ---------------------------------------------------------------------------
$(STAGE2_BIN): $(BOOT)/stage2.asm | $(BUILD)
	@echo "[NASM ] $< → $@"
	$(NASM) $(NASMFLAGS) $< -o $@

# ---------------------------------------------------------------------------
# Kernel entry stub (32-bit ASM glue)
# ---------------------------------------------------------------------------
$(KERNEL_ENTRY_OBJ): $(KERNEL)/kernel_entry.asm | $(BUILD)
	@echo "[NASM ] $< → $@"
	$(NASM) -f elf32 $< -o $@

# ---------------------------------------------------------------------------
# ISR / IRQ stubs (ELF32)
# ---------------------------------------------------------------------------
$(BUILD)/isr.o: $(KERNEL)/isr.asm | $(BUILD)
	@echo "[NASM ] $< → $@"
	$(NASM) -f elf32 $< -o $@

$(BUILD)/irq.o: $(KERNEL)/irq.asm | $(BUILD)
	@echo "[NASM ] $< → $@"
	$(NASM) -f elf32 $< -o $@

# ---------------------------------------------------------------------------
# Kernel C objects
# ---------------------------------------------------------------------------
$(BUILD)/%.o: $(KERNEL)/%.c | $(BUILD)
	@echo "[CC   ] $< → $@"
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Link kernel → flat binary
# ---------------------------------------------------------------------------
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_ASM_OBJS) $(KERNEL_OBJS)
	@echo "[LD   ] → $@"
	$(LD) $(LDFLAGS) -o $@ $^

# ---------------------------------------------------------------------------
# Assemble disk image:
#   Sector 1  : boot.bin   (512 B)
#   Sector 2  : stage2.bin (512 B, padded)
#   Sector 3+ : kernel.bin
# ---------------------------------------------------------------------------
$(DISK_IMG): $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "[IMG  ] assembling disk image → $@"
	@# Create a clean 1.44 MB floppy image filled with zeros
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@# Write MBR (sector 1)
	dd if=$(BOOT_BIN)   of=$@ conv=notrunc bs=512 seek=0 status=none
	@# Write stage2 (sector 2)
	dd if=$(STAGE2_BIN) of=$@ conv=notrunc bs=512 seek=1 status=none
	@# Write kernel starting at sector 3
	dd if=$(KERNEL_BIN) of=$@ conv=notrunc bs=512 seek=2 status=none
	@echo "[IMG  ] done  (size: $$(stat -c%s $@) bytes)"

# ---------------------------------------------------------------------------
# Run in QEMU
# ---------------------------------------------------------------------------
run: $(DISK_IMG)
	@echo "[QEMU ] launching UbaidOS..."
	$(QEMU) \
		-drive format=raw,file=$(DISK_IMG),if=floppy \
		-boot a \
		-m 32M \
		-name "UbaidOS" \
		-display sdl \
		-no-reboot

# ---------------------------------------------------------------------------
# Debug mode — attach GDB with: gdb -ex "target remote :1234"
# ---------------------------------------------------------------------------
debug: $(DISK_IMG)
	@echo "[QEMU ] debug mode — GDB server on :1234"
	$(QEMU) \
		-drive format=raw,file=$(DISK_IMG),if=floppy \
		-boot a \
		-m 32M \
		-name "UbaidOS [DEBUG]" \
		-display sdl \
		-s -S

# ---------------------------------------------------------------------------
# Clean
# ---------------------------------------------------------------------------
clean:
	@echo "[CLEAN] removing build artefacts..."
	@rm -rf $(BUILD)
	@echo "[CLEAN] done"
