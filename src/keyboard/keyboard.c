#include "keyboard/keyboard.h"
#include "task/process.h"
#include "task/task.h"
#include "status.h"
#include "keyboard/classic.h"
#include "idt/idt.h"

static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_last = 0;

void keyboard_init()
{
    keyboard_insert(classic_init());

    #warning 如果要在classic_keyboard_init()注册中断的话，那有多个键盘只能注册链表中最后一个键盘的处理函数了 \
        于是在这里重新注册一下，选择我们的初始键盘
    idt_register_interrupt_callback(ISR_KEYBOARD_INTERRUPT, classic_keyboard_handle_interrupt);
}

int keyboard_insert(struct keyboard* keyboard)
{
    int res = 0;
    if(!keyboard || keyboard->init == 0) {
        res = -EINVARG;
        goto out;
    }

    if(keyboard_list_last) {
        keyboard_list_last->next = keyboard;
        keyboard_list_last = keyboard;
    }
    else {
        keyboard_list_head = keyboard;
        keyboard_list_last = keyboard;
    }

    res = keyboard->init();
out:
    return res;
}

static int keyboard_get_tail_index(struct process* process)
{
    return process->keyboard.tail % sizeof(process->keyboard.buffer);
}

static int keyboard_get_head_index(struct process* process)
{
    return process->keyboard.head % sizeof(process->keyboard.buffer);
}

#warning 原作者这里写的有问题, 我将其修改, 他没有判缓冲区为空的情况。但是这个函数并没被用到，这个函数本就是不合理的，因为字符 \
        已经被pop并打印到屏幕上了, 这种情况你清理缓冲区有什么用呢？还是得操作屏幕。但是如果按我下面注释那样写，或许就没问题。
void keyboard_backspace(struct process* process)
{    
    process->keyboard.tail--;
    int real_index = keyboard_get_tail_index(process);

    /* 如果末尾是0x08，那就在末尾加一个0x08，交给terminal_backspace去处理。如果末尾不是0x08，那我们就删除最后一个字符 */
    // if(process->keyboard.buffer[real_index] == 0 || process->keyboard.buffer[real_index] == 0x08) {
    if(process->keyboard.buffer[real_index] == 0) {
        // 缓冲区为空
        process->keyboard.tail++;
        /* push backspace 即 0x08 */
        // keyboard_push(0x08);
        return;
    }
    

    process->keyboard.buffer[real_index] = 0x00;   
}

void keyboard_push(char c)
{
    struct process* process = process_current();
    if(!process) {
        return;
    }

    if(c == 0) {
        return;
    }

    int real_index = keyboard_get_tail_index(process);
    if(process->keyboard.buffer[real_index] != 0) {
        // 没办法再push, 满了
        return;
    }

    process->keyboard.buffer[real_index] = c;
    process->keyboard.tail++;

    return;
}

char keyboard_pop()
{
    if(!task_current()) {
        return 0;
    }

    struct process* process = task_current()->process;
    int real_index = keyboard_get_head_index(process);
    char c = process->keyboard.buffer[real_index];

    if(c == 0x00) {
        // 没办法再pop, 空了
        return 0;
    }

    process->keyboard.buffer[real_index] = 0x00;
    process->keyboard.head++;

    return c;
}