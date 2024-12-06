section .asm

extern int21h_handler
extern no_interrupt_handler
extern isr80h_handler

global int21h
global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

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


int21h:
    pushad
    call int21h_handler
    popad
    iret

no_interrupt:
    pushad
    call no_interrupt_handler
    popad
    iret

isr80h_wrapper:
    ; 压入当前的寄存器状态作为中断帧
    pushad

    ; 当前的esp就是中断帧的地址，放进栈里相当于isr80h_handler的第二个参数
    push esp

    ; eax存储着中断号，相当于isr80h_handler的第一个参数
    push eax
    call isr80h_handler

    ; 将返回值寄存，避免接下来用到eax破坏返回值
    mov dword[tmp_res], eax
    
    ; 因为压了两个参数，为了popad正常运行所以esp减8
    add esp, 8

    ; popad 恢复任务
    popad

    ; 恢复返回值
    mov eax, dword[tmp_res]
    iretd

section .data
; isr80h_handler 返回值寄存处
tmp_res: dd 0