; =============================================================================
; UbaidOS — Stage 1 Bootloader
; Origin : 0x7C00  (BIOS loads MBR here)
; Size   : exactly 512 bytes (enforced by boot signature padding below)
;
; Responsibilities:
;   1. Save the BIOS drive number for later disk reads
;   2. Set up a clean real-mode segment environment (CS=DS=ES=SS=0)
;   3. Display a welcome banner and greet the user by name
;   4. Load Stage 2 from sector 2 into 0x1000 and jump to it
;
; Memory map at this point:
;   0x0000:0x7C00 — this code (loaded by BIOS)
;   0x0000:0x1000 — stage2 will be loaded here
; =============================================================================

[bits 16]
[org  0x7C00]

; ---------------------------------------------------------------------------
; Entry point — BIOS jumps here with DL = boot drive number
; ---------------------------------------------------------------------------
_start:
    ; Normalise segment registers so all addresses are based on segment 0.
    ; Some BIOSes jump to 0x07C0:0000 instead of 0x0000:7C00; this fixes that.
    jmp  0x0000:.normalise
.normalise:
    xor  ax, ax
    mov  ds, ax
    mov  es, ax
    mov  ss, ax
    mov  sp, 0x7C00         ; stack grows downward from 0x7C00

    ; Preserve the BIOS-supplied drive number (DL).
    mov  [boot_drive], dl

    ; -----------------------------------------------------------------------
    ; Greet the user
    ; -----------------------------------------------------------------------
    call clear_screen

    mov  si, msg_banner
    call print_str

    mov  si, msg_prompt
    call print_str

    ; Read the user's name into `name_buf` (max NAME_MAX chars)
    mov  di, name_buf
    call read_line

    ; Print "Hello, <name>!"
    call print_newline
    mov  si, msg_hello
    call print_str
    mov  si, name_buf
    call print_str
    mov  si, msg_exclaim
    call print_str
    call print_newline

    ; -----------------------------------------------------------------------
    ; Load Stage 2 from sector 2 (CHS 0/0/2) into 0x0000:0x1000
    ; -----------------------------------------------------------------------
    mov  si, msg_loading
    call print_str

    call load_stage2

    ; Transfer control — pass drive number in DL
    mov  dl, [boot_drive]
    jmp  0x0000:0x1000

    ; Should never reach here
    jmp  $

; ---------------------------------------------------------------------------
; load_stage2 — load 1 sector from disk sector 2 to 0x0000:0x1000
;   Retries up to 3 times before giving up.
; ---------------------------------------------------------------------------
load_stage2:
    mov  cx, 3              ; retry counter
.retry:
    push cx

    mov  ah, 0x02           ; BIOS: read sectors
    mov  al, 1              ; number of sectors to read
    mov  ch, 0              ; cylinder 0
    mov  cl, 2              ; sector 2 (1-based)
    mov  dh, 0              ; head 0
    mov  dl, [boot_drive]
    mov  bx, 0x1000         ; ES:BX destination (ES=0 set above)
    int  0x13

    pop  cx
    jnc  .ok                ; carry clear → success

    ; Reset disk controller and retry
    xor  ah, ah
    mov  dl, [boot_drive]
    int  0x13
    dec  cx
    jnz  .retry

    ; All retries exhausted → print error and halt
    mov  si, msg_disk_err
    call print_str
    jmp  $                  ; hard halt

.ok:
    mov  si, msg_ok
    call print_str
    call print_newline
    ret

; ---------------------------------------------------------------------------
; print_str — print a NUL-terminated string pointed to by SI
; ---------------------------------------------------------------------------
print_str:
    mov  ah, 0x0E           ; BIOS teletype output
.loop:
    lodsb                   ; AL = [SI++]
    test al, al             ; NUL terminator?
    jz   .done
    int  0x10
    jmp  .loop
.done:
    ret

; ---------------------------------------------------------------------------
; print_newline — emit CR+LF
; ---------------------------------------------------------------------------
print_newline:
    mov  ah, 0x0E
    mov  al, 0x0D           ; carriage return
    int  0x10
    mov  al, 0x0A           ; line feed
    int  0x10
    ret

; ---------------------------------------------------------------------------
; clear_screen — switch to text mode 3 (80x25, colour) which clears display
; ---------------------------------------------------------------------------
clear_screen:
    mov  ah, 0x00
    mov  al, 0x03
    int  0x10
    ret

; ---------------------------------------------------------------------------
; read_line — read keyboard input into buffer at DI, NUL-terminated
;   Handles:  Enter (0x0D) → terminates input
;             Backspace    → deletes last character (with visual feedback)
;   Max bytes: NAME_MAX (defined below); extra chars are simply discarded
; ---------------------------------------------------------------------------
%define NAME_MAX 32

read_line:
    xor  cx, cx             ; CX = current length
.key:
    xor  ah, ah
    int  0x16               ; wait for keystroke → AL = ASCII, AH = scan

    cmp  al, 0x0D           ; Enter?
    je   .done

    cmp  al, 0x08           ; Backspace?
    je   .backspace

    ; Ignore if buffer full
    cmp  cx, NAME_MAX
    jge  .key

    ; Store character and echo it
    stosb                   ; [DI++] = AL
    inc  cx
    mov  ah, 0x0E
    int  0x10
    jmp  .key

.backspace:
    test cx, cx
    jz   .key               ; nothing to delete

    ; Move DI back one position and overwrite with space
    dec  di
    dec  cx
    mov  ah, 0x0E
    mov  al, 0x08           ; backspace
    int  0x10
    mov  al, ' '
    int  0x10
    mov  al, 0x08           ; backspace again to reposition cursor
    int  0x10
    jmp  .key

.done:
    mov  al, 0
    stosb                   ; NUL-terminate
    ret

; ---------------------------------------------------------------------------
; Data
; ---------------------------------------------------------------------------
msg_banner   db 13, 10
             db "  +---------------------------------+", 13, 10
             db "  |         U b a i d O S           |", 13, 10
             db "  |    x86 Learning Operating System |", 13, 10
             db "  +---------------------------------+", 13, 10, 13, 10, 0

msg_prompt   db "  Enter your name: ", 0
msg_hello    db "  Hello, ", 0
msg_exclaim  db "!", 0
msg_loading  db "  Loading Stage 2...", 0
msg_ok       db " OK", 0
msg_disk_err db 13, 10, "  [ERROR] Disk read failed. System halted.", 0

; Uninitialised buffers — sit in the binary but contents don't matter at load
name_buf     times NAME_MAX+1 db 0
boot_drive   db 0

; ---------------------------------------------------------------------------
; Pad to 510 bytes and append the MBR boot signature (0xAA55)
; ---------------------------------------------------------------------------
times 510 - ($ - $$) db 0
dw 0xAA55

buffer times 64 db 0
boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xaa55