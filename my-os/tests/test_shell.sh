#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="$ROOT_DIR/build/test-logs"
LOG_FILE="$LOG_DIR/shell.log"
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

if ! grep -q "SHELL_HELP_OK" "$LOG_FILE"; then
  echo "[FAIL] Shell test: help command failed"
  exit 1
fi

if ! grep -q "SHELL_ECHO_OK" "$LOG_FILE"; then
  echo "[FAIL] Shell test: echo command failed"
  exit 1
fi

if ! grep -q "SHELL_CLEAR_OK" "$LOG_FILE"; then
  echo "[FAIL] Shell test: clear command failed"
  exit 1
fi

if ! grep -q "INPUT_OK" "$LOG_FILE"; then
  echo "[FAIL] Shell test: input handling failed"
  exit 1
fi

echo "[PASS] Shell Commands"
