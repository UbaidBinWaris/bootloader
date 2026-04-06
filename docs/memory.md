# 🧠 Memory Architecture (Bootloader)

## 📘 Overview

Understanding memory is critical in OS development.

This bootloader runs in **Real Mode (16-bit)**.

---

## 📍 Memory Map (Real Mode)

```
+------------------------+
| 0x00000 - 0x003FF      | Interrupt Vector Table
+------------------------+
| 0x00400 - 0x004FF      | BIOS Data Area
+------------------------+
| 0x00500 - 0x07BFF      | Free Memory
+------------------------+
| 0x07C00 - 0x07DFF      | Bootloader (512 bytes)
+------------------------+
| 0x07E00 - ...          | Free Space
+------------------------+
```

---

## 🎯 Bootloader Location

```
0x0000:0x7C00 → Bootloader starts here
```

---

## 🧩 Segmentation Concept

Real mode uses:

```
Physical Address = Segment * 16 + Offset
```

Example:

```
0x07C0:0x0000 → 0x7C00
```

---

## 🧠 Stack (Important)

Currently:

* Stack is not explicitly set
* Uses BIOS default

Later you should define:

```asm
mov bp, 0x9000
mov sp, bp
```

---

## 🧠 Buffer Memory

```
buffer times 64 db 0
```

Stored in RAM after code section.

---

## ⚠️ Limitations

* Max memory: 1MB
* No protection
* No virtual memory

---

## 🚀 Future Upgrade

Switch to:

* Protected Mode (32-bit)
* Paging
* Virtual memory

---

## 💡 Key Insight

Memory control = OS power

This is what separates:

* Normal developers ❌
* System engineers ✅

---
