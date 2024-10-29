#ifndef IDT_H
#define IDT_H

#include<stdint.h>

struct idt_desc {
    uint16_t offset_1;  // 中断处理函数的低16位地址
    uint16_t selector;  // CODE_SEG的值，作用未知
    uint8_t zero;       // 零值
    uint8_t type_attr;  // type，包含权限级别以及其他信息
    uint16_t offset_2;  // 中断处理函数的高16位地址
}__attribute__((packed));

struct idtr_desc {
    uint16_t limit;     // 中断函数的数量-1
    uint32_t base;      // 中断函数表的地址
}__attribute__((packed));

void idt_init();

void enable_interrupts();
void disable_interrupts();

#endif