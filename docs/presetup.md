# 🛠️ Environment Setup (Arch Linux)

## Install Required Tools

```bash
sudo pacman -S nasm qemu gcc make gdb xorriso grub
sudo pacman -S gcc-multilib
```

## Tools Explanation

| Tool    | Purpose              |
| ------- | -------------------- |
| nasm    | Assembly compiler    |
| qemu    | Emulator             |
| gcc     | Kernel compilation   |
| gdb     | Debugging            |
| xorriso | ISO creation         |
| grub    | Reference bootloader |

---

## Folder Setup

```bash
mkdir my-os
cd my-os
mkdir boot kernel build docs
```
