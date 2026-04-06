[org 0x1000]
[bits 16]

start:
    mov [boot_drive], dl

    ; load kernel (sector 3)
    mov ah, 0x02
    mov al, 4
    mov ch, 0
    mov cl, 3
    mov dh, 0
    mov dl, [boot_drive]

    mov bx, 0x2000
    int 0x13

    jc $

    cli
    lgdt [gdt_descriptor]

    ; enable A20
    in al, 0x92
    or al, 00000010b
    out 0x92, al

    ; enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; FAR jump (IMPORTANT)
    jmp CODE_SEG:init_pm


; =========================
; GDT
gdt_start:
    dq 0
gdt_code:
    dq 0x00cf9a000000ffff
gdt_data:
    dq 0x00cf92000000ffff
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start


; =========================
[bits 32]

init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov esp, 0x90000

    ; DEBUG PRINT (important)
    mov dword [0xb8000], 0x2f50324d   ; "M2P" (to confirm mode switch)

    ; jump to kernel (NOW CORRECT)
    jmp 0x2000

hang:
    jmp hang

times 512 - ($ - $$) db 0
boot_drive db 0