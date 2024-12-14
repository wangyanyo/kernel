#include "keyboard/keyboard.h"
#include "task/process.h"
#include "task/task.h"
#include "status.h"
#include "keyboard/classic.h"

static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_last = 0;

void keyboard_init()
{
    keyboard_insert(classic_init());
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

#warning 原作者这里写的有问题, 我将其修改, 这个函数是按下backspace后调用的吗? 要是字符已经被打印到屏幕上了怎么办? \
    这个函数的backspace是针对缓冲区的
void keyboard_backspace(struct process* process)
{    
    process->keyboard.tail--;
    int real_index = keyboard_get_tail_index(process);
    if(process->keyboard.buffer[real_index] != 0) {
        // 缓冲区为空
        process->keyboard.tail++;
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
