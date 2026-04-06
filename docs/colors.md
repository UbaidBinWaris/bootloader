# 🎨 Colors in Bootloader

## BIOS Text Mode Colors

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

## Example

```asm
mov ah, 0x0e
mov bl, 0x0A   ; light green
int 0x10
```

---

## How It Works

* `AH = 0x0E` → teletype mode
* `BL` → color attribute
