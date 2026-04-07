; =============================================================================
; UbaidOS — Kernel Entry Point
; Linked at  : 0x2000 (flat binary, entry symbol _start)
; Called from: Stage 2 loader via  jmp CODE_SEG:0x2000
; CPU state  : 32-bit protected mode, flat segments, ESP = 0x90000
;
; Responsibilities:
;   1. Clear the DF flag (so string ops go forward)
;   2. Zero the BSS section (C standard guarantees this)
;   3. Call kernel_main()
;   4. If kernel_main() ever returns (it shouldn't), halt the CPU
; =============================================================================

[bits 32]

global _start          ; linker / objcopy entry symbol
extern kernel_main     ; defined in kernel.c

; ---------------------------------------------------------------------------
; BSS zero-fill bounds supplied by the linker script
; ---------------------------------------------------------------------------
extern _bss_start
extern _bss_end

_start:
    ; Clear direction flag so STOSD / MOVSD work in the expected direction
    cld

    ; ---------------------------------------------------------------------------
    ; Zero the BSS section (uninitialized global / static data)
    ; ---------------------------------------------------------------------------
    mov  edi, _bss_start    ; destination
    mov  ecx, _bss_end
    sub  ecx, edi           ; byte count
    xor  eax, eax
    rep  stosb              ; fill with 0x00

    ; ---------------------------------------------------------------------------
    ; Call the C kernel
    ; ---------------------------------------------------------------------------
    call kernel_main

    ; ---------------------------------------------------------------------------
    ; kernel_main returned — this should never happen.
    ; Disable interrupts and spin forever (BSOD equivalent).
    ; ---------------------------------------------------------------------------
.halt:
    cli
    hlt
    jmp  .halt