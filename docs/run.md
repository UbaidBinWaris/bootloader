# ▶️ Build & Run

## Build Bootloader

```bash
cd boot
nasm -f bin boot.asm -o boot.bin
```

## Run in Emulator

```bash
qemu-system-x86_64 -drive format=raw,file=boot.bin
```

---

## Debug Mode

```bash
qemu-system-x86_64 -s -S -drive format=raw,file=boot.bin
```

Then:

```bash
gdb
target remote localhost:1234
```
