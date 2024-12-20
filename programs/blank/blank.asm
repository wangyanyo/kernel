[BITS 32]

section .asm

global _start

_start:

label:

; _loop:
;     call getkey

;     push eax 
;     mov eax, 3  ; Command 3: putchar
;     int 0x80
;     add esp, 4

;     jmp _loop

; getkey:
;     mov eax, 2
;     int 0x80
;     cmp eax, 0x00
;     je getkey
;     ret

    push message
    mov eax, 1
    int 0x80
    add esp, 4
    
    jmp $

section .data
message: db 'I can talk with kernel!', 0