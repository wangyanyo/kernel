#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"

uint16_t* vedio_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

uint16_t terminal_make_char(char c, char color) {
    return color << 8 | c;
}

void terminal_putchar(int x, int y, char c, char color) {
    vedio_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, color);
}

void terminal_writechar(char c, char color) {
    // 它甚至连\n都没法处理，这也太底层了吧
    if(c == '\n') {
        terminal_col = 0;
        terminal_row += 1;
        return;
    }
    terminal_putchar(terminal_col, terminal_row, c, color);
    terminal_col++;
    if(terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        terminal_row += 1;
    }
}

void terminal_initialize() {
    vedio_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for(int y = 0; y < VGA_HEIGHT; ++y) {
        for(int x = 0; x < VGA_WIDTH; ++x) {
            terminal_putchar(x, y, ' ', 0x0);
        }
    }
}

size_t strlen(const char* str) {
    size_t len = 0;
    while(str[len]) {
        len++;
    }
    return len;
}

void print(const char* str) {
    size_t len = strlen(str);
    for(int i = 0; i < len; ++i) {
        terminal_writechar(str[i], 15);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0;
void kernel_main() {
    // 初始化终端
    terminal_initialize();

    print("Hello World!\ntest\n");

    // 初始化内存管理
    kheap_init();

    // 初始化磁盘驱动
    disk_search_and_init();

    // 初始化中断描述符表
    idt_init();

    // 初始化分页虚拟内存
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PERSENT | PAGING_ACCESS_FROM_ALL);

    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));

    enable_paging();

    // 恢复中断
    enable_interrupts();
}