[BITS 32]

section .asm

global _start

_start:

label:

    push 20
    push 30     ; 30 + 20
    mov eax, 0
    int 0x80
    add esp, 8

    jmp $