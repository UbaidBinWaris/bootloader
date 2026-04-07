[bits 16]
[org 0x7C00]

%define NAME_MAX 32

start:
    jmp 0x0000:.entry

.entry:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    call clear_screen
    mov si, msg_banner
    call print_str
    mov si, msg_prompt
    call print_str

    mov di, name_buf
    call read_line

    call print_newline
    mov si, msg_hello
    call print_str
    mov si, name_buf
    call print_str
    mov si, msg_exclaim
    call print_str
    call print_newline

    mov si, msg_loading
    call print_str
    call load_stage2

    mov dl, [boot_drive]
    jmp 0x0000:0x1000

.halt:
    cli
    hlt
    jmp .halt

load_stage2:
    mov cx, 3
.retry:
    push cx
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x1000
    int 0x13
    pop cx
    jnc .ok

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13
    dec cx
    jnz .retry

    mov si, msg_disk_err
    call print_str
    jmp .halt

.ok:
    mov si, msg_ok
    call print_str
    call print_newline
    ret

print_str:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

print_newline:
    mov ah, 0x0E
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

read_line:
    xor cx, cx
.key:
    xor ah, ah
    int 0x16
    cmp al, 0x0D
    je .done
    cmp al, 0x08
    je .backspace
    cmp cx, NAME_MAX
    jae .key

    stosb
    inc cx
    mov ah, 0x0E
    int 0x10
    jmp .key

.backspace:
    test cx, cx
    jz .key
    dec di
    dec cx
    mov ah, 0x0E
    mov al, 0x08
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 0x08
    int 0x10
    jmp .key

.done:
    mov al, 0
    stosb
    ret

msg_banner   db 13,10,"UbaidOS Stage 1",13,10,0
msg_prompt   db "Enter your name: ",0
msg_hello    db "Hello, ",0
msg_exclaim  db "!",0
msg_loading  db "Loading stage2...",0
msg_ok       db " OK",0
msg_disk_err db 13,10,"[ERROR] stage2 disk read failed",13,10,0

name_buf   times NAME_MAX+1 db 0
boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xAA55