[org 0x7c00]

start:
    mov si, message
    call print

hang:
    jmp hang

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

message db "Bootloader Stage 1 OK", 0

times 510 - ($ - $$) db 0
dw 0xaa55