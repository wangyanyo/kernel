section .asm

global idt_load
global no_interrupt
global int21h
global enable_interrupts
global disable_interrupts

extern no_interrupt_handler
extern int21h_handler

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

idt_load:
    push ebp
    mov ebp, esp
    mov ebx, [ebp+8]
    lidt [ebx]

    pop ebp
    ret

no_interrupt:
    cli
    pushad
    call no_interrupt_handler
    popad
    sti
    iret

int21h:
    cli
    pushad
    call int21h_handler
    popad
    sti
    iret
