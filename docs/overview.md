# 🧠 System Overview

## Boot Process

```
BIOS → Bootloader → Kernel → OS
```

---

## Memory Layout

```
0x7C00 → Bootloader loaded here
Buffer → User input
```

---

## BIOS Role

* Loads first 512 bytes
* Transfers control to bootloader

---

## Interrupts Used

### Display

```
int 0x10
```

### Keyboard

```
int 0x16
```

---

## Key Learning

This project teaches:

* Low-level programming
* Hardware communication
* OS architecture fundamentals
