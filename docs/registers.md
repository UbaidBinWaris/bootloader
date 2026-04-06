# 🧠 CPU Registers Used

## General Registers

| Register | Purpose             |
| -------- | ------------------- |
| AX       | Accumulator         |
| AH       | Function selector   |
| AL       | Data                |
| SI       | Source pointer      |
| DI       | Destination pointer |

---

## Usage in Bootloader

### SI

Points to string:

```asm
mov si, message
```

---

### DI

Points to buffer:

```asm
mov di, buffer
```

---

### AH / AL

```asm
mov ah, 0x0e
mov al, 'A'
int 0x10
```
