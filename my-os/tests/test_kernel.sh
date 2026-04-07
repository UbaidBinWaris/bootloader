#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="$ROOT_DIR/build/test-logs"
LOG_FILE="$LOG_DIR/kernel.log"
IMG="$ROOT_DIR/build/os.img"

mkdir -p "$LOG_DIR"

if [[ ! -f "$IMG" ]]; then
  echo "[ERROR] Disk image missing: $IMG"
  echo "Run: make build"
  exit 1
fi

timeout 8s qemu-system-i386 \
  -drive format=raw,file="$IMG",if=floppy \
  -boot a \
  -m 32M \
  -display none \
  -serial stdio \
  -monitor none \
  -no-reboot \
  -no-shutdown >"$LOG_FILE" 2>&1 || true

if ! grep -q "KERNEL_OK" "$LOG_FILE"; then
  echo "[FAIL] Kernel test: missing kernel ready marker"
  exit 1
fi

if ! grep -q "UbaidOS" "$LOG_FILE"; then
  echo "[FAIL] Kernel test: boot banner marker not found"
  exit 1
fi

if grep -qi "KERNEL PANIC" "$LOG_FILE"; then
  echo "[FAIL] Kernel test: panic detected"
  exit 1
fi

if grep -qi "triple fault" "$LOG_FILE"; then
  echo "[FAIL] Kernel test: fatal fault detected"
  exit 1
fi

echo "[PASS] Kernel Output"
