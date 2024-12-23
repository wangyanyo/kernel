[BITS 32]

global _start
global kernel_registers
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ; Enable the A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Remap the master PIC
    mov al, 0x11
    out 0x20, al    ; Tell master PIC

    ; Remap the slave PIC
    mov al, 0x11
    mov 0xA0, al

    mov al, 0x20    ; Interrupt 0x20 is where master ISR should start
    out 0x21, al

    mov al, 0x28    ; Interrupt 0x28 is where slave ISR should start
    out 0xA1, al

    mov al ,4       ; 主从PIC如何相连
    out 0x21, al

    mov al, 2       ; 主从PIC如何相连
    out 0xA1, al

    mov al, 0x1     ; matser PIC 8086 mode
    out 0x21, al

    mov al, 0x1     ; slave PIC 8086 mode
    out 0xA1, al
    ; End remap of ther master PIC

    call kernel_main

    jmp $

kernel_registers:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret

times 512-($ - $$) db 0