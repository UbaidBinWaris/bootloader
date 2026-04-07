    # UbaidOS

    > A 32-bit x86 operating system built from scratch — two-stage bootloader, protected-mode kernel, VGA text driver, PS/2 keyboard driver, and interactive shell. Zero external libraries. Zero shortcuts.

    ![Built with NASM](https://img.shields.io/badge/assembler-NASM-blue?style=flat-square)
    ![Built with GCC](https://img.shields.io/badge/compiler-GCC%20-m32-blue?style=flat-square)
    ![Runs on QEMU](https://img.shields.io/badge/emulator-QEMU-green?style=flat-square)
    ![Platform](https://img.shields.io/badge/platform-x86%2032--bit-orange?style=flat-square)
    ![License](https://img.shields.io/badge/license-MIT-lightgrey?style=flat-square)

    ---

    ## Overview

    UbaidOS is a bare-metal operating system that boots on real x86 hardware (and QEMU) entirely without any OS, runtime, or library assistance. It demonstrates full-stack low-level systems programming: from the first byte of the Master Boot Record all the way up to an interactive command-line shell running in 32-bit protected mode.

    Every component is hand-written:

    - **Stage 1** — 512-byte MBR bootloader in x86-16 assembly  
    - **Stage 2** — Second-stage loader: loads kernel from disk, enables A20, sets up GDT, switches to protected mode  
    - **Kernel** — Freestanding C kernel: VGA terminal, PS/2 keyboard, and mini shell

    This project was built as a deep-dive into how computers actually work at the hardware interface level, with no shortcuts, no GRUB, and no OS abstractions.

    ---

    ## Architecture

    ```
    ┌─────────────────────────────────────────────────────────┐
    │                    User (Shell)                         │
    │          help / clear / echo / color / info / reboot    │
    ├─────────────────────────────────────────────────────────┤
    │                  Kernel (kernel.c)                      │
    │      VGA Terminal Driver  │  PS/2 Keyboard Driver       │
    ├─────────────────────────────────────────────────────────┤
    │            Kernel Entry (kernel_entry.asm)              │
    │         BSS zero-init  →  call kernel_main()            │
    ├─────────────────────────────────────────────────────────┤
    │            Stage 2 Loader (stage2.asm)                  │
    │    Load Kernel  │  A20  │  GDT  │  Protected Mode       │
    ├─────────────────────────────────────────────────────────┤
    │             Stage 1 MBR (boot.asm)                      │
    │         User Input  │  Load Stage 2  │  Boot Sig        │
    ├─────────────────────────────────────────────────────────┤
    │                  BIOS / Firmware                        │
    └─────────────────────────────────────────────────────────┘
                        x86 Hardware
    ```

    ---

    ## Features

    ### Bootloader (Stages 1 & 2)

    | Feature | Detail |
    |---------|--------|
    | Two-stage boot | Stage 1 fits within the 512-byte MBR limit; Stage 2 handles the heavy lifting |
    | BIOS disk I/O | INT 13h with 3-attempt retry loop and disk reset on failure |
    | User input in Stage 1 | Reads user's name before boot; passed to kernel for the greeting |
    | A20 line enable | Fast A20 via port `0x92` — reliable and instant |
    | GDT setup | Flat-model 32-bit code and data descriptors covering the full 4 GB address space |
    | Protected mode switch | `cli → lgdt → CR0.PE=1 → far jmp` canonical sequence |
    | Status messages | INT 10h telemetry at each boot stage (A20, GDT, PM) |

    ### Kernel

    | Feature | Detail |
    |---------|--------|
    | Freestanding C | `-ffreestanding -nostdlib -nostdinc` — no libc, no startup runtime |
    | VGA 80×25 text driver | Direct memory-mapped writes to `0xB8000`; full colour attribute support |
    | 16 foreground + 16 background colours | Matches the complete CGA/VGA text mode palette |
    | Terminal scrolling | Hardware-free software scroll — `memmove` the screen buffer up one row |
    | Backspace, tab, newline | Full control character handling in `terminal_putchar()` |
    | PS/2 keyboard driver | Polling on ports `0x60` / `0x64`; Scancode Set 1; Shift + Caps Lock |
    | `keyboard_readline()` | Buffered line input with backspace and echo |
    | BSS zero-initialisation | Linker-exported `_bss_start` / `_bss_end` symbols; `rep stosb` zeroes segment |
    | Interactive shell | Prompt `ubaid@UbaidOS:~$`; 6 built-in commands |
    | ASCII boot banner | Colour-cycled "UbaidOS" art printed on startup |

    ### Shell Commands

    | Command | Description |
    |---------|-------------|
    | `help` | Lists all available commands |
    | `clear` | Clears the terminal screen |
    | `echo <text>` | Prints `<text>` back to the terminal |
    | `color <fg> <bg>` | Changes terminal foreground and background colours |
    | `info` | Displays OS name, version, architecture, and memory info |
    | `reboot` | Triggers a keyboard-controller CPU reset (port `0x64`, `0xFE`) |

    ---

    ## Memory Layout

    | Address | Region | Contents |
    |---------|--------|----------|
    | `0x00000–0x003FF` | IVT | BIOS interrupt vector table |
    | `0x00400–0x004FF` | BDA | BIOS data area |
    | `0x07C00–0x07DFF` | Stage 1 | MBR bootloader (512 bytes, BIOS loads here) |
    | `0x01000–0x01FFF` | Stage 2 | Second-stage loader |
    | `0x02000–0x05FFF` | Kernel | Kernel binary (16 sectors / 8 KB) |
    | `0x90000` | Stack | Kernel stack top (`ESP = 0x90000`) |
    | `0xB8000–0xB9F3F` | VGA | Text mode framebuffer (80×25×2 bytes) |

    ---

    ## Disk Image Layout

    ```
    ┌──────────────┐  ← Sector 0  (0x000)  LBA 0
    │   boot.bin   │     512 bytes — MBR Stage 1
    ├──────────────┤  ← Sector 1  (0x200)  LBA 1
    │  stage2.bin  │     512 bytes — Stage 2 loader
    ├──────────────┤  ← Sector 2  (0x400)  LBA 2
    │              │
    │  kernel.bin  │     16 sectors / 8 192 bytes — kernel
    │              │
    ├──────────────┤  ← Sector 18 (0x2400)
    │  (unused)    │
    └──────────────┘  ← 1.44 MB floppy image end
    ```

    ---

    ## Project Structure

    ```
    my-os/
    ├── Makefile                 ← Master build system (build / run / debug / clean)
    ├── README.md                ← This file
    ├── boot/
    │   ├── boot.asm             ← Stage 1: 512-byte MBR bootloader (x86-16 ASM)
    │   └── stage2.asm           ← Stage 2: A20 + GDT + protected mode switch
    ├── kernel/
    │   ├── kernel_entry.asm     ← 32-bit BSS zeroing + call to kernel_main()
    │   ├── kernel.c             ← VGA driver, keyboard driver, shell (freestanding C)
    │   └── linker.ld            ← Flat-binary link script; origin at 0x2000
    ├── build/                   ← Compiler output (created by make; git-ignored)
    └── docs/
        ├── architecture.md      ← OS layer diagram, design decisions, source map
        ├── boot-process.md      ← Boot sequence flowchart, stage-by-stage detail
        ├── memory-layout.md     ← Full physical address space map, VGA format
        └── protected-mode.md    ← GDT descriptors, PM switch, flat memory model
    ```

    ---

    ## Requirements

    **Operating System**: Linux (Arch Linux recommended; any distro works)

    **Packages required**:

    ```bash
    # Arch Linux
    sudo pacman -S nasm gcc make qemu gdb gcc-multilib

    # Ubuntu / Debian
    sudo apt install nasm gcc make qemu-system-x86 gdb gcc-multilib
    ```

    | Tool | Purpose | Version |
    |------|---------|---------|
    | `nasm` | Assemble `.asm` → `.bin` | ≥ 2.14 |
    | `gcc` | Compile freestanding C kernel | ≥ 10 |
    | `gcc-multilib` | 32-bit `-m32` support on 64-bit host | matches gcc |
    | `make` | Build orchestration | ≥ 4.0 |
    | `qemu-system-i386` | x86 emulation | ≥ 6.0 |
    | `gdb` | Debugging via QEMU GDB stub | ≥ 10 (optional) |

    ---

    ## Build & Run

    ```bash
    # Clone the repository
    git clone https://github.com/<your-username>/ubaid-os.git
    cd ubaid-os/my-os

    # Build everything — assembles, compiles, links, and creates os.img
    make build

    # Run in QEMU
    make run

    # Run with GDB debugging stub on localhost:1234
    make debug
    # In a separate terminal: gdb -ex "target remote :1234" build/kernel.elf

    # Remove all build artifacts
    make clean
    ```

    ### What `make build` does

    ```
    boot.asm        → nasm -f bin          → build/boot.bin
    stage2.asm      → nasm -f bin          → build/stage2.bin
    kernel_entry.asm → nasm -f elf         → build/kernel_entry.o
    kernel.c        → gcc -m32 -ffreestanding … → build/kernel.o
                    → ld -m elf_i386 --oformat binary → build/kernel.bin
    dd boot.bin + stage2.bin + kernel.bin  → build/os.img (1.44 MB)
    ```

    ---

    ## Screenshots

    ```
    ┌─────────────────────────────────────────────────────────────────┐
    │  ██╗   ██╗██████╗  █████╗ ██╗██████╗  ██████╗ ███████╗        │
    │  ██║   ██║██╔══██╗██╔══██╗██║██╔══██╗██╔═══██╗██╔════╝        │
    │  ██║   ██║██████╔╝███████║██║██║  ██║██║   ██║███████╗        │
    │  ██║   ██║██╔══██╗██╔══██║██║██║  ██║██║   ██║╚════██║        │
    │  ╚██████╔╝██████╔╝██║  ██║██║██████╔╝╚██████╔╝███████║        │
    │   ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝╚═════╝  ╚═════╝ ╚══════╝        │
    │                                                                 │
    │  Version 1.0  |  32-bit Protected Mode  |  x86                 │
    │                                                                 │
    │  ubaid@UbaidOS:~$ help                                          │
    │  Available commands:                                            │
    │    help    - Show this help message                             │
    │    clear   - Clear the screen                                   │
    │    echo    - Print text (usage: echo <text>)                    │
    │    color   - Change colors (usage: color <fg> <bg>)             │
    │    info    - Display system information                         │
    │    reboot  - Reboot the system                                  │
    │                                                                 │
    │  ubaid@UbaidOS:~$ _                                             │
    └─────────────────────────────────────────────────────────────────┘
    ```

    ---

    ## Technical Deep Dives

    Detailed documentation is available in the [`docs/`](docs/) directory:

    | Document | Contents |
    |----------|----------|
    | [Architecture](docs/architecture.md) | OS layer diagram, design decisions table, file map |
    | [Boot Process](docs/boot-process.md) | Full boot flowchart, disk layout, stage-by-stage analysis |
    | [Memory Layout](docs/memory-layout.md) | Physical address space map, VGA buffer format, stack layout |
    | [Protected Mode](docs/protected-mode.md) | GDT descriptor byte-fields, PM switch sequence, flat model |

    ---

    ## What This Demonstrates

    This project showcases proficiency across multiple systems programming disciplines:

    - **x86 Assembly** — MBR constraints, segment:offset addressing, BIOS interrupts, real/protected mode duality
    - **CPU Architecture** — A20 gate, GDT, descriptor privilege levels, CR0 manipulation, segment registers in PM
    - **Memory Management** — Manual memory layout design, linker scripts, BSS initialisation, stack discipline
    - **Hardware I/O** — Port-mapped I/O (`in`/`out`), PS/2 protocol, VGA memory-mapped framebuffer
    - **Freestanding C** — Kernel-mode C without libc; raw pointers to hardware; `volatile` correctness
    - **Build Systems** — GNU Make with multi-stage cross-compilation, `dd`-based disk image assembly
    - **Debugging** — QEMU + GDB remote debugging of a bare-metal kernel

    ---

    ## Roadmap

    Features planned for future versions:

    - [ ] IDT + exception handlers (division by zero, page fault, etc.)
    - [ ] PIT (8253) timer → preemptive scheduling groundwork
    - [ ] IRQ demuxing via PIC (8259A) remapping — move IRQs off reserved ints
    - [ ] Paging — 4 KB page tables, virtual address space
    - [ ] User mode (ring 3) + syscall interface
    - [ ] Simple FAT12 filesystem driver (read from floppy)
    - [ ] Serial port console (`0x3F8`) for headless debugging

    ---

    ## License

    MIT — see [LICENSE](LICENSE) for details.

    ---

    ## Author

    **Ubaid** — built as a personal deep-dive into how operating systems work at the hardware level.

    > *"Tell me and I forget. Show me and I remember. Let me build it from scratch and I understand."*
