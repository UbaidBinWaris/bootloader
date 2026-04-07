; =============================================================================
; UbaidOS — Stage 2 Loader
; Loaded by Stage 1 at : 0x0000:0x1000
; Runs in              : 16-bit real mode, then switches to 32-bit protected mode
;
; Responsibilities:
;   1. Load the kernel image from disk into 0x2000
;   2. Enable the A20 address line (via fast A20 port 0x92)
;   3. Install the Global Descriptor Table (GDT)
;   4. Switch the CPU into 32-bit protected mode
;   5. Transfer control to kernel_entry (_start at 0x2000)
;
; Memory map:
;   0x0000:0x7C00 — Stage 1 MBR
;   0x0000:0x1000 — Stage 2 (this file)
;   0x0000:0x2000 — Kernel binary
;   0x0009:0x0000 — Stack top in protected mode (ESP = 0x90000)
; =============================================================================

[bits 16]
[org  0x1000]

; ---------------------------------------------------------------------------
; Constants
; ---------------------------------------------------------------------------
KERNEL_OFFSET   equ 0x2000   ; physical address where kernel is loaded
KERNEL_SECTORS  equ 16       ; sectors to read (16 x 512 = 8 KiB)
KERNEL_START_S  equ 3        ; kernel starts at disk sector 3 (1-based)

CODE_SEG  equ gdt_code - gdt_start
DATA_SEG  equ gdt_data - gdt_start

; ---------------------------------------------------------------------------
; Entry point — DL = boot drive number passed by Stage 1
; ---------------------------------------------------------------------------
stage2_start:
    ; Re-establish safe segment environment
    xor  ax, ax
    mov  ds, ax
    mov  es, ax
    mov  ss, ax
    mov  sp, 0x1000         ; temporary stack below our own code

    ; Save boot drive before we clobber DL
    mov  [boot_drive], dl

    ; -----------------------------------------------------------------------
    ; Status messages
    ; -----------------------------------------------------------------------
    mov  si, msg_stage2
    call print_str
    call print_newline

    ; -----------------------------------------------------------------------
    ; Load kernel from disk
    ; -----------------------------------------------------------------------
    mov  si, msg_kernel
    call print_str

    call load_kernel

    mov  si, msg_ok
    call print_str
    call print_newline

    ; -----------------------------------------------------------------------
    ; Enable A20 gate via "fast A20" (port 0x92)
    ; Bit 1 = A20 enable.  Bit 0 = system reset — MUST remain 0.
    ; -----------------------------------------------------------------------
    in   al, 0x92
    or   al, 0x02           ; set A20 enable bit
    and  al, 0xFE           ; clear reset bit (never set this!)
    out  0x92, al

    ; -----------------------------------------------------------------------
    ; Load GDT and switch to protected mode
    ; -----------------------------------------------------------------------
    mov  si, msg_pm
    call print_str
    call print_newline

    cli                     ; no interrupts during mode switch
    lgdt [gdt_descriptor]

    ; Set PE (Protection Enable) bit in CR0
    mov  eax, cr0
    or   eax, 0x1
    mov  cr0, eax

    ; Far jump: flush pipeline and load CS with CODE_SEG descriptor index
    jmp  CODE_SEG:init_pm

; ---------------------------------------------------------------------------
; load_kernel — INT 13h / AH=02h with up to 3 retries
; ---------------------------------------------------------------------------
load_kernel:
    mov  cx, 3              ; retry counter
.retry:
    push cx

    mov  ah, 0x02
    mov  al, KERNEL_SECTORS
    mov  ch, 0              ; cylinder 0
    mov  cl, KERNEL_START_S ; starting sector (1-based)
    mov  dh, 0              ; head 0
    mov  dl, [boot_drive]
    xor  bx, bx
    mov  es, bx             ; ES = 0
    mov  bx, KERNEL_OFFSET  ; ES:BX = 0x0000:0x2000
    int  0x13

    pop  cx
    jnc  .ok                ; carry clear = success

    ; Reset disk controller and retry
    xor  ah, ah
    mov  dl, [boot_drive]
    int  0x13
    dec  cx
    jnz  .retry

    ; All retries exhausted — print error and halt
    mov  si, msg_disk_err
    call print_str
    jmp  $

.ok:
    ret

; ---------------------------------------------------------------------------
; print_str   — print NUL-terminated string pointed to by SI (BIOS INT 10h)
; print_newline — emit CR+LF
; ---------------------------------------------------------------------------
print_str:
    mov  ah, 0x0E
.loop:
    lodsb
    test al, al
    jz   .done
    int  0x10
    jmp  .loop
.done:
    ret

print_newline:
    mov  ah, 0x0E
    mov  al, 0x0D           ; CR
    int  0x10
    mov  al, 0x0A           ; LF
    int  0x10
    ret

; ---------------------------------------------------------------------------
; Data — declared BEFORE the padding so they live inside our sector
; ---------------------------------------------------------------------------
msg_stage2   db "  [Stage 2] Loader active", 0
msg_kernel   db "  [Stage 2] Loading kernel...", 0
msg_pm       db "  [Stage 2] Entering protected mode...", 0
msg_ok       db " OK", 0
msg_disk_err db 13, 10, "  [ERROR] Kernel disk read failed — system halted.", 0

boot_drive   db 0

; ===========================================================================
; 32-bit protected-mode initialisation
; CS is already CODE_SEG after the far jump above
; ===========================================================================
[bits 32]
init_pm:
    ; Load all data-segment registers with our flat data descriptor
    mov  ax, DATA_SEG
    mov  ds, ax
    mov  ss, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    ; Protected-mode stack: 576 KiB mark, well above the kernel
    mov  esp, 0x90000

    ; Transfer control to the kernel entry point
    jmp  CODE_SEG:KERNEL_OFFSET

; ===========================================================================
; Global Descriptor Table (flat 4 GiB model, ring 0)
; ===========================================================================
[bits 16]

gdt_start:

; Null descriptor (CPU requirement — must be 8 zero bytes)
gdt_null:
    dd 0x00000000
    dd 0x00000000

; Code segment  Base=0  Limit=4GiB  Ring-0  32-bit  Execute/Read
gdt_code:
    dw 0xFFFF       ; limit[15:0]
    dw 0x0000       ; base[15:0]
    db 0x00         ; base[23:16]
    db 10011010b    ; P=1 DPL=00 S=1 Type=1010 (exec/read)
    db 11001111b    ; G=1 D/B=1 L=0 AVL=0 limit[19:16]=1111
    db 0x00         ; base[31:24]

; Data segment  Base=0  Limit=4GiB  Ring-0  32-bit  Read/Write
gdt_data:
    dw 0xFFFF       ; limit[15:0]
    dw 0x0000       ; base[15:0]
    db 0x00         ; base[23:16]
    db 10010010b    ; P=1 DPL=00 S=1 Type=0010 (read/write)
    db 11001111b    ; G=1 D/B=1 L=0 AVL=0 limit[19:16]=1111
    db 0x00         ; base[31:24]

gdt_end:

; GDTR value — 6 bytes: 2-byte limit + 4-byte linear base address
gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; GDT size in bytes minus 1
    dd gdt_start                 ; linear (physical) address of GDT

; ---------------------------------------------------------------------------
; Pad to exactly 512 bytes (one disk sector).
; If Stage 2 grows beyond 512 bytes, increase the seek in the Makefile and
; adjust KERNEL_START_S accordingly.
; ---------------------------------------------------------------------------
times 512 - ($ - $$) db 0