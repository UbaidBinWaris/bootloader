[org 0x7c00]

start:
    mov [boot_drive], dl

    call clear_screen

    mov si, welcome_msg
    call print

    call newline

    mov si, input_msg
    call print

    call get_input

    call newline
    mov si, result_msg
    call print

    mov si, buffer
    call print

    call newline

    call load_stage2

hang:
    jmp hang

; =========================
load_stage2:
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]

    mov bx, 0x1000
    int 0x13

    jc disk_error

    mov dl, [boot_drive]
    jmp 0x0000:0x1000

disk_error:
    mov si, err
    call print
    jmp $

; =========================
print:
    mov ah, 0x0e
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

newline:
    mov ah, 0x0e
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    ret

clear_screen:
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    ret

get_input:
    mov di, buffer
.loop:
    mov ah, 0x00
    int 0x16
    cmp al, 13
    je .done
    stosb
    mov ah, 0x0e
    int 0x10
    jmp .loop
.done:
    mov al, 0
    stosb
    ret

; =========================
welcome_msg db "=== UBAID OS ===",0
input_msg   db "Enter name: ",0
result_msg  db "Hello, ",0
err         db "Disk Error!",0

buffer times 64 db 0
boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xaa55