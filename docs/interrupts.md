# ⚡ BIOS Interrupts Guide

## 📘 Overview

Interrupts allow software to communicate with hardware.

In Real Mode, BIOS provides built-in interrupt services.

---

## 🧠 What is an Interrupt?

```
Interrupt → CPU stops → Executes handler → Returns
```

---

## 🎯 Used in Bootloader

---

### 🖥️ INT 0x10 — Video Services

Used for display output.

#### Function: Print Character

```asm
mov ah, 0x0e
mov al, 'A'
int 0x10
```

---

### ⌨️ INT 0x16 — Keyboard Services

Used for input.

#### Function: Read Key

```asm
mov ah, 0x00
int 0x16
```

Result:

* AL = ASCII character

---

### 💾 INT 0x13 — Disk Services (Next Step)

Used for reading disk sectors.

```asm
mov ah, 0x02
int 0x13
```

---

## 📊 Summary Table

| Interrupt | Purpose  |
| --------- | -------- |
| 0x10      | Display  |
| 0x16      | Keyboard |
| 0x13      | Disk     |

---

## ⚠️ Important Notes

* Only works in Real Mode
* Slow but simple
* Replaced later by direct hardware drivers

---

## 🚀 Advanced Direction

Later you will:

* Replace BIOS interrupts
* Write your own drivers
* Control hardware directly

---

## 💡 Real Insight

BIOS interrupts are:

> Training wheels for OS development

Real OS:
❌ Doesn’t use BIOS
✅ Uses custom drivers

---
