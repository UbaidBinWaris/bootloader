# 🖥️ Kernel Shell (Mini Terminal)

## Overview

After the kernel initialises all hardware drivers, `kernel_main()` drops into a simple interactive shell. Input is read character-by-character via the IRQ1 keyboard driver (ring buffer) — **not** via BIOS `INT 0x16`.

---

## Input Pipeline

```
PS/2 keyboard scans key
  └─ IRQ1 fires → keyboard_handler() reads port 0x60
      └─ scancode_to_ascii[] converts to ASCII
          └─ ascii char written into ring buffer
              └─ kernel_readline_irq() returns on Enter
                  └─ shell compares command string
                      └─ executes matching function
```

---

## Available Commands

| Command | Action |
| ------- | ------ |
| `help` | List all commands with short descriptions |
| `clear` | Wipe the VGA text buffer (all 80×25 cells) |
| `hello` | Print a greeting message |
| `uptime` | Display elapsed timer ticks (1 tick = 10 ms at 100 Hz) |
| `reboot` | Triple-fault to force warm reboot |

---

## Implementation Notes

### String comparison

The shell uses `strcmp()` from `kernel.c` (custom, no libc):

```c
int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}
```

### Command dispatch loop

```c
while (1) {
    print("ubaidOS> ");
    keyboard_readline_irq(cmd_buf, sizeof(cmd_buf));
    if      (strcmp(cmd_buf, "help")   == 0) cmd_help();
    else if (strcmp(cmd_buf, "clear")  == 0) cmd_clear();
    else if (strcmp(cmd_buf, "hello")  == 0) cmd_hello();
    else if (strcmp(cmd_buf, "uptime") == 0) cmd_uptime();
    else if (strcmp(cmd_buf, "reboot") == 0) cmd_reboot();
    else { print("unknown command: "); println(cmd_buf); }
}
```

### Backspace

The keyboard driver handles `\b` (scancode `0x0E`): it removes the last character from the ring buffer and sends a VGA backspace sequence.

---

## Adding a New Command

1. Add a case in the dispatch loop in `kernel.c`
2. Implement the function (use `print()` / `println()` / `print_hex_uint32()`)
3. Add the command name to the `cmd_help()` list

---

### Step 3: Execute Command

```asm
cmd_help db "help",0
cmd_clear db "clear",0
```

---

## 🔥 Example Execution

```
> help
Available commands: help, clear, hello
```

---

## 🚀 Future Enhancements

* Backspace support
* Command history
* Auto-complete
* Multi-word commands

---

## 💡 Learning Outcome

You learn:

* String comparison in Assembly
* Control flow design
* Building interpreters (like shells)

---

## 🧠 Real Insight

This is how:

* Linux shell works (conceptually)
* CLI tools process input
* Interpreters are built

---
