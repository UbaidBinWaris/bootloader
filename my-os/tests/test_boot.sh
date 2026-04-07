#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="$ROOT_DIR/build/test-logs"
LOG_FILE="$LOG_DIR/boot.log"
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

if ! grep -q "S2_START" "$LOG_FILE"; then
  echo "[FAIL] Boot test: missing stage2 start marker"
  exit 1
fi

if ! grep -q "S2_LOAD_OK" "$LOG_FILE"; then
  echo "[FAIL] Boot test: kernel load marker missing"
  exit 1
fi

if ! grep -q "BOOT_OK" "$LOG_FILE"; then
  echo "[FAIL] Boot test: kernel boot marker missing"
  exit 1
fi

if grep -q "S2_LOAD_ERR" "$LOG_FILE"; then
  echo "[FAIL] Boot test: disk load error detected"
  exit 1
fi

echo "[PASS] Boot Test"
