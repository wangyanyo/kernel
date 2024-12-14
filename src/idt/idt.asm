section .asm

extern isr80h_handler
extern interrupt_handler

global idt_load
global enable_interrupts
global disable_interrupts
global isr80h_wrapper
global interrupt_pointer_table

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

%macro interrupt 1
    global int%1
    int%1:
        ; INTERRUPT FRAME START
        ; ALREADY PUSHED TO US BY THE PROCESSOR UPON ENTRY TO THIS INTERRUPT
        ; uint32_t ip
        ; uint32_t cs;
        ; uint32_t flags
        ; uint32_t sp;
        ; uint32_t ss;
        ; Pushes the general purpose registers to the stack
        pushad
        ; Interrupt frame end
        push esp
        push dword %1
        call interrupt_handler
        add esp, 8
        popad
        iret
%endmacro
%assign i 0
%rep 512
    interrupt i
%assign i i+1
%endrep

section .data
; isr80h_handler 返回值寄存处
tmp_res: dd 0

%macro interrupt_array_entry 1
    dd int%1
%endmacro

interrupt_pointer_table:
%assign i 0
%rep 512
    interrupt_array_entry i
%assign i i+1
%endrep
