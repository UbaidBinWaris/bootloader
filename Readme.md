# 🚀 Ubaid OS Bootloader Project

## 📘 Overview

This project is a **custom x86 bootloader** built from scratch using Assembly on **Arch Linux**.
It demonstrates low-level system programming, BIOS interaction, and the foundations of operating system development.

---

## 🛠️ Environment Setup (Arch Linux)

Install required tools:

```bash
sudo pacman -S nasm qemu gcc make gdb xorriso grub
sudo pacman -S gcc-multilib
```

### 📦 Tool Purpose

| Tool         | Purpose                       |
| ------------ | ----------------------------- |
| nasm         | Assembly compiler             |
| qemu         | Emulator to run OS safely     |
| gcc          | Compile kernel (future)       |
| gdb          | Debugging                     |
| xorriso      | Create bootable ISO           |
| grub         | Advanced bootloader reference |
| gcc-multilib | 32-bit support                |

---

## 📂 Project Structure

```
my-os/
├── boot/
│   ├── boot.asm
│   └── boot.bin
├── kernel/
├── build/
```

---

## ⚙️ Build & Run

```bash
cd boot
nasm -f bin my-os/boot/boot.asm -o my-os/build/boot.bin
qemu-system-x86_64 -drive format=raw,file=my-os/build/boot.bin
```

---

## 🧠 Boot Process Explained

```
BIOS → loads first 512 bytes → executes at memory address 0x7C00
```

### 📍 Memory Layout

```
0x0000:0x7C00 → Bootloader loaded here
Buffer         → Stores user input
Stack          → Default (not manually defined yet)
```

---

## 🧾 Bootloader Code (Stage 1 Advanced)

```asm
[org 0x7c00]

start:
    call clear_screen

    mov si, welcome_msg
    call print_string

    call new_line

    mov si, input_msg
    call print_string

    call get_input

    call new_line
    mov si, result_msg
    call print_string

    mov si, buffer
    call print_string

hang:
    jmp hang

; =========================
; PRINT STRING
print_string:
    mov ah, 0x0e
    mov bh, 0x00
    mov bl, 0x0A   ; Light green color
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

; =========================
; NEW LINE
new_line:
    mov ah, 0x0e
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    ret

; =========================
; CLEAR SCREEN
clear_screen:
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    ret

; =========================
; GET INPUT
get_input:
    mov di, buffer

.input_loop:
    mov ah, 0x00
    int 0x16

    cmp al, 13
    je .done

    stosb

    mov ah, 0x0e
    int 0x10

    jmp .input_loop

.done:
    mov al, 0
    stosb
    ret

; =========================
; DATA
welcome_msg db "=== UBAID OS BOOTLOADER ===",0
input_msg   db "Enter your name: ",0
result_msg  db "Hello, ",0

buffer times 64 db 0

; =========================
; BOOT SIGNATURE
times 510 - ($ - $$) db 0
dw 0xaa55
```

---

## 🔍 Code Explanation (Line by Line Concepts)

### `[org 0x7c00]`

* Tells assembler that code will run at memory address `0x7C00`

---

### BIOS Interrupts Used

#### 🎯 Display Output

```asm
int 0x10
```

* Prints characters to screen

#### 🎯 Keyboard Input

```asm
int 0x16
```

* Reads key from keyboard

---

### Registers Used

| Register | Purpose           |
| -------- | ----------------- |
| SI       | Points to string  |
| DI       | Points to buffer  |
| AH       | Function selector |
| AL       | Data register     |

---

### Buffer

```asm
buffer times 64 db 0
```

* Stores user input in memory
* Max length: 64 characters

---

## 🎨 Colors (Text Mode)

| Color  | Code |
| ------ | ---- |
| Black  | 0x00 |
| Blue   | 0x01 |
| Green  | 0x02 |
| Cyan   | 0x03 |
| Red    | 0x04 |
| Yellow | 0x0E |
| White  | 0x0F |

---

## 💡 Features Implemented

* ✅ Text output
* ✅ Colored text
* ✅ User input
* ✅ String handling
* ✅ Screen clearing
* ✅ Memory buffer

---

## ⚔️ Bootloader Comparison

### 🪟 Windows Bootloader

* Closed source
* UEFI-based
* Complex & abstracted

---

### 🐧 GRUB Bootloader

* Multi-stage loader
* Supports file systems
* Loads multiple OS

---

### 🚀 Ubaid Bootloader

* Fully custom
* Minimal & fast
* Direct hardware control
* Built for learning + extension

---

## 🔥 Why This Project Matters

Most developers:

* ❌ Never interact with BIOS
* ❌ Don’t understand memory at low level

You:

* ✅ Control hardware directly
* ✅ Understand boot process deeply
* ✅ Can build your own OS

---

## 🧭 Next Steps

### 🔹 Short Term

* Add command system (mini terminal)
* Implement backspace support
* Add cursor control

### 🔹 Mid Term

* Load second stage bootloader
* Read disk sectors

### 🔹 Advanced

* Switch to protected mode
* Write kernel in C
* Build full OS

---

## 🧠 Learning Outcome

This project teaches:

* Assembly programming
* Memory management
* Hardware interaction
* OS fundamentals

---

## 📌 Author

**Ubaid Bin Waris**
Full Stack Engineer → Systems Programmer 🚀

---

## ⭐ Final Note

This is not just a project.

This is:

> Transition from **web developer → system engineer**

---
