#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

TESTS=(
  "$ROOT_DIR/tests/test_boot.sh"
  "$ROOT_DIR/tests/test_kernel.sh"
  "$ROOT_DIR/tests/test_shell.sh"
)

pass_count=0
fail_count=0

for test_script in "${TESTS[@]}"; do
  if bash "$test_script"; then
    pass_count=$((pass_count + 1))
  else
    fail_count=$((fail_count + 1))
  fi
done

echo
if [[ $fail_count -eq 0 ]]; then
  echo "Summary: $pass_count passed, $fail_count failed"
  exit 0
fi

echo "Summary: $pass_count passed, $fail_count failed"
exit 1
