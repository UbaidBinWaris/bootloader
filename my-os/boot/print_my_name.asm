[org 0x7c00]

mov si, message
call print

jmp $

; ------------------------
; Print function
print:
    mov ah, 0x0e
.next:
    lodsb          ; load byte from [SI] into AL
    cmp al, 0
    je done
    int 0x10
    jmp .next
done:
    ret

; ------------------------
; Data
message db "Hello Ubaid Bootloader!", 0

; ------------------------
; Boot sector padding
times 510 - ($ - $$) db 0
dw 0xaa55