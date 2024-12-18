#include "keyboard/classic.h"
#include "keyboard/keyboard.h"
#include <stdint.h>
#include <stddef.h>
#include "io/io.h"
#include "kernel.h"
#include "idt/idt.h"
#include "task/task.h"

static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    0x0d, 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`', 
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.'
};

static int classic_keyboard_init();

struct keyboard classic_keyboard = {
    .name = {"Classic"},
    .init = classic_keyboard_init
};

void classic_keyboard_handle_interrupt();

static int classic_keyboard_init()
{
    // 如果要在classic_keyboard_init()注册中断的话，那有多个键盘只能注册链表中最后一个键盘的处理函数了
    idt_register_interrupt_callback(ISR_KEYBOARD_INTERRUPT, classic_keyboard_handle_interrupt);
    // enable ps2
    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT);
    return 0;
}

struct keyboard *classic_init()
{
    return &classic_keyboard;
}

static uint8_t classic_keyboard_scancode_to_char(uint8_t scancode)
{
    size_t size_of_keyboard_set_one = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    #warning 作者这里写的是 >, 很明显他是错的
    if(scancode >= size_of_keyboard_set_one) {
        return 0;
    }

    char c = keyboard_scan_set_one[scancode];
    return c;
}

void classic_keyboard_handle_interrupt()
{
    kernel_page();
    uint8_t scancode = insb(CLASSIC_INPUT_PORT);
    insb(CLASSIC_INPUT_PORT);

    // 键盘会发送两次中断，一次是press，一次是release, 我们忽略release
    #warning 作者这里直接return了没有切回用户态, 很明显他是错的
    if(scancode & CLASSIC_KEYBOARD_KEY_RELEASED) {
        goto out;
    }

    char c = classic_keyboard_scancode_to_char(scancode);
    if(!c) {
        goto out;
    }

    keyboard_push(c);
    
out:
    // 确实，即使c==0也不能直接退出，因为还要切回用户态
    task_page();
}
