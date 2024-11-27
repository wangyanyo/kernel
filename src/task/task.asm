[BITS 32]
section .asm

; struct registers
; {
;     uint32_t edi;
;     uint32_t esi;
;     uint32_t ebp;
;     uint32_t ebx;
;     uint32_t edx;
;     uint32_t ecx;
;     uint32_t eax;
    
;     uint32_t ip;
;     uint32_t cs;
;     uint32_t flags;
;     uint32_t esp;
;     uint32_t ss;
; };

global task_return
global restore_general_purpose_registers
global user_registers


; void task_return(struct registers* regs)
task_return:
    mov ebp, esp
    ; PUSH THE DATA SEGMENT (SS WILL BE FINE)
    ; PUSH THE STACK ADDRESS
    ; PUSH THE FLAGS
    ; PUSH THE CODE SEGMENT
    ; PUSH IP

    ; regs
    mov ebx, [ebp + 4]
    ;warning 为什么要设置段寄存器，我们明明没有用到段，而且这里存的应该是gdt的偏移量 + CPL
    ; push the data segment
    push dword [ebx + 44]
    ; push the stack address
    push dword [ebx + 40]
    
    ; push the flags
    ;warning 那 registers 结构中的 flags 有什么用?
    pushf
    pop eax
    or eax, 0x200
    push eax

    ; push the code segment
    push dword [ebx + 32]

    ; push the IP
    push dword [ebx + 28]

    ; Setup some segment registers
    mov ax, [ebx + 44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;warning 这里原作者写的是 ebx + 4, 很明显他是错的
    push dword [ebp + 4]
    call restore_general_purpose_registers
    add esp, 4

    ; Let's leave kernel land and execute in user land!
    iretd

; void restore_general_purpose_registers(struct registers* regs)
restore_general_purpose_registers:
    push ebp
    mov ebp, esp

    mov ebx, [ebp + 8]
    mov edi, [ebx]
    mov esi, [ebx + 4]
    mov ebp, [ebx + 8]
    mov edx, [ebx + 16]
    mov ecx, [ebx + 20]
    mov eax, [ebx + 24]
    mov ebx, [ebx + 12]

    ;warning 这里真的要 pop ebp 吗？这不是把 ebp 破坏了吗？
    pop ebp
    ret

; void user_registers()
user_registers:
    ; user data segment
    ; 这个0x23是0x20 + 3, 0x20是因为user data segment的gdt是第5个，所以偏移量是0x20
    ; 因为gdt的偏移量是8的倍数，所以低三位都是0，所以用低两位来存储CPL，user land的CPL是3，所以是0x20 + 3 = 0x23
    mov ax, 0x23
    ;warning 其实我不懂这些段寄存器都是用来干什么的，但应该是忽略了段这个东西
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret
    