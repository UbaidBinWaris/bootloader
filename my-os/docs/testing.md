# Testing Guide

## Overview

The test suite boots UbaidOS in headless QEMU mode and validates behavior by checking deterministic serial debug markers.

## Test Files

- `tests/test_boot.sh`
- `tests/test_kernel.sh`
- `tests/test_shell.sh`
- `scripts/test_all.sh`

## How Tests Work

Each test runs:

```bash
qemu-system-i386 -display none -serial stdio -monitor none ...
```

QEMU output is captured to `build/test-logs/*.log`, then validated with `grep`.

## Markers

- `S2_START`, `S2_LOAD_OK`, `BOOT_OK`: boot path
- `KERNEL_OK`, `UbaidOS`: kernel init path
- `SHELL_HELP_OK`, `SHELL_ECHO_OK`, `SHELL_CLEAR_OK`, `INPUT_OK`: shell path

## Run Tests

```bash
make test
```

or

```bash
bash scripts/test_all.sh
```

## Expected Output

```text
[PASS] Boot Test
[PASS] Kernel Output
[PASS] Shell Commands
Summary: 3 passed, 0 failed
```
