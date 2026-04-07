; ============================================================
;  isr.asm — CPU exception stubs (ISR 0–31) + common handler
; ============================================================
[BITS 32]

; idt_load: load the IDT register using the supplied idt_ptr_t*
global idt_load
idt_load:
    mov eax, [esp+4]     ; pointer to idt_ptr_t
    lidt [eax]
    ret

; Declare all ISR symbols so C (idt.c) can take their addresses
global isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7
global isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

; isr_handler is the C dispatcher defined in kernel.c
extern isr_handler

; ------------------------------------------------------------------
;  Macros
;  ISR_NOERR num — exceptions that do NOT push an error code
;  ISR_ERR   num — exceptions that DO push an error code (CPU pushes it)
; ------------------------------------------------------------------
%macro ISR_NOERR 1
isr%1:
    push byte 0          ; dummy error code (keeps frame uniform)
    push byte %1         ; interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
isr%1:
    ; CPU already pushed the real error code
    push byte %1         ; interrupt number
    jmp isr_common_stub
%endmacro

; ------------------------------------------------------------------
;  CPU exceptions — Intel IA-32 Vol 3A §6.3.1
;  Exceptions 8, 10, 11, 12, 13, 14, 17, 21, 29, 30 push error codes
; ------------------------------------------------------------------
ISR_NOERR  0   ; #DE  Divide-by-zero
ISR_NOERR  1   ; #DB  Debug
ISR_NOERR  2   ;      NMI
ISR_NOERR  3   ; #BP  Breakpoint
ISR_NOERR  4   ; #OF  Overflow
ISR_NOERR  5   ; #BR  Bound Range Exceeded
ISR_NOERR  6   ; #UD  Invalid Opcode
ISR_NOERR  7   ; #NM  Device Not Available (FPU)
ISR_ERR    8   ; #DF  Double Fault             — error code always 0
ISR_NOERR  9   ;      Coprocessor Segment Overrun (reserved)
ISR_ERR   10   ; #TS  Invalid TSS
ISR_ERR   11   ; #NP  Segment Not Present
ISR_ERR   12   ; #SS  Stack-Segment Fault
ISR_ERR   13   ; #GP  General Protection Fault
ISR_ERR   14   ; #PF  Page Fault
ISR_NOERR 15   ;      Reserved
ISR_NOERR 16   ; #MF  x87 Floating-Point Exception
ISR_ERR   17   ; #AC  Alignment Check
ISR_NOERR 18   ; #MC  Machine Check
ISR_NOERR 19   ; #XM  SIMD Floating-Point Exception
ISR_NOERR 20   ; #VE  Virtualisation Exception
ISR_ERR   21   ; #CP  Control Protection Exception
ISR_NOERR 22   ;      Reserved
ISR_NOERR 23   ;      Reserved
ISR_NOERR 24   ;      Reserved
ISR_NOERR 25   ;      Reserved
ISR_NOERR 26   ;      Reserved
ISR_NOERR 27   ;      Reserved
ISR_NOERR 28   ;      Reserved
ISR_ERR   29   ;      Reserved / VMM Communication
ISR_ERR   30   ; #SX  Security Exception
ISR_NOERR 31   ;      Reserved

; ------------------------------------------------------------------
;  isr_common_stub
;
;  Stack at entry (after macro pushes int_no, err_code is above it):
;      [ESP+0]  int_no      (pushed by macro)
;      [ESP+4]  err_code    (pushed by macro or CPU)
;      [ESP+8]  EIP         (CPU)
;      [ESP+12] CS          (CPU)
;      [ESP+16] EFLAGS      (CPU)
;   (only when crossing privilege levels: ESP, SS)
;
;  We build a registers_t on the stack and pass a pointer to C.
; ------------------------------------------------------------------
isr_common_stub:
    pushad               ; EAX, ECX, EDX, EBX, ESP(original), EBP, ESI, EDI
    push ds              ; save data segment

    ; Switch to kernel data segment selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp             ; argument: pointer to registers_t
    call isr_handler
    add esp, 4           ; discard the pointer

    pop ds               ; restore caller's data segment
    popad                ; restore general-purpose registers
    add esp, 8           ; discard int_no + err_code
    iret                 ; restore EIP, CS, EFLAGS (and SS, ESP if needed)
