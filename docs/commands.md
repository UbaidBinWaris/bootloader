# 🖥️ Mini Shell (Command System)

## 📘 Overview

This bootloader is extended into a **basic command-line interface (CLI)** similar to a terminal.

Users can type commands and the system responds accordingly.

---

## 🎯 Goal

Transform bootloader from:

```
Input → Echo
```

Into:

```
Input → Parse → Execute Command
```

---

## 🧠 Command Flow

```
User Input → Buffer → Compare → Execute Function
```

---

## ⚙️ Basic Commands

| Command | Action                  |
| ------- | ----------------------- |
| help    | Show available commands |
| clear   | Clear screen            |
| hello   | Print greeting          |
| reboot  | Restart system          |

---

## 🧩 Implementation Concept

### Step 1: Read Input

Stored in buffer:

```asm
buffer times 64 db 0
```

---

### Step 2: Compare Strings

Example logic:

```asm
compare:
    mov si, buffer
    mov di, cmd_help

.loop:
    mov al, [si]
    mov bl, [di]
    cmp al, bl
    jne not_equal
    cmp al, 0
    je equal
    inc si
    inc di
    jmp .loop

equal:
    ; run help command
```

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
