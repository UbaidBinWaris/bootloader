[org 0x7c00]

start:
    call clear_screen

    mov si, welcome_msg
    call print_string

    call new_line

    mov si, input_msg
    call print_string

    call get_input

    call new_line
    mov si, result_msg
    call print_string

    mov si, buffer
    call print_string

hang:
    jmp hang

; =========================
; PRINT STRING
print_string:
    mov ah, 0x0e
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

; =========================
; NEW LINE
new_line:
    mov ah, 0x0e
    mov al, 0x0D   ; carriage return
    int 0x10
    mov al, 0x0A   ; line feed
    int 0x10
    ret

; =========================
; CLEAR SCREEN
clear_screen:
    mov ah, 0x00
    mov al, 0x03   ; text mode
    int 0x10
    ret

; =========================
; GET INPUT
get_input:
    mov di, buffer

.input_loop:
    mov ah, 0x00
    int 0x16       ; keyboard input

    cmp al, 13     ; Enter key
    je .done

    stosb          ; store char in buffer

    mov ah, 0x0e   ; echo char
    int 0x10

    jmp .input_loop

.done:
    mov al, 0
    stosb
    ret

; =========================
; DATA
welcome_msg db "=== UBAID OS BOOTLOADER ===",0
input_msg   db "Enter your name: ",0
result_msg  db "Hello, ",0

buffer times 64 db 0

; =========================
; BOOT SIGNATURE
times 510 - ($ - $$) db 0
dw 0xaa55