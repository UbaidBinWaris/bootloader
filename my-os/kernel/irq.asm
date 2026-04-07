; ============================================================
;  irq.asm — Hardware IRQ stubs (IRQ 0–15) + common handler
;
;  After PIC remapping:  IRQ 0-7  → INT 0x20-0x27
;                        IRQ 8-15 → INT 0x28-0x2F
; ============================================================
[BITS 32]

global irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7
global irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15

; irq_handler is the C dispatcher defined in kernel.c
extern irq_handler

; ------------------------------------------------------------------
;  Macro: IRQ_STUB irq_number, vector_number
;  We push 0 for the (unused) error code to keep the frame identical
;  to the isr_common_stub layout.
; ------------------------------------------------------------------
%macro IRQ_STUB 2
irq%1:
    push byte 0          ; dummy error code
    push byte %2         ; interrupt vector number (0x20 + irq)
    jmp irq_common_stub
%endmacro

IRQ_STUB  0, 0x20
IRQ_STUB  1, 0x21
IRQ_STUB  2, 0x22
IRQ_STUB  3, 0x23
IRQ_STUB  4, 0x24
IRQ_STUB  5, 0x25
IRQ_STUB  6, 0x26
IRQ_STUB  7, 0x27
IRQ_STUB  8, 0x28
IRQ_STUB  9, 0x29
IRQ_STUB 10, 0x2A
IRQ_STUB 11, 0x2B
IRQ_STUB 12, 0x2C
IRQ_STUB 13, 0x2D
IRQ_STUB 14, 0x2E
IRQ_STUB 15, 0x2F

; ------------------------------------------------------------------
;  irq_common_stub — identical layout to isr_common_stub so that
;  the same registers_t struct definition works for both.
; ------------------------------------------------------------------
irq_common_stub:
    pushad               ; save all GP registers
    push ds              ; save data segment

    ; Switch to kernel data segment selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp             ; argument: pointer to registers_t
    call irq_handler
    add esp, 4           ; discard the pointer

    pop ds               ; restore caller's data segment
    popad                ; restore GP registers
    add esp, 8           ; discard vector number + dummy error code
    iret
