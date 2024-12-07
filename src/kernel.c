#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "string/string.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "config.h"
#include "memory/memory.h"
#include "task/tss.h"
#include "task/process.h"
#include "status.h"
#include "isr80h/isr80h.h"

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
        terminal_row += 1;
        terminal_col = 0;
        return;
    }

    terminal_putchar(terminal_col, terminal_row, c, color);
    terminal_col += 1;
    if(terminal_col >= VGA_WIDTH) 
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}

void terminal_initialize() {
    vedio_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for(int y = 0; y < VGA_HEIGHT; ++y) 
    {
        for(int x = 0; x < VGA_WIDTH; ++x) 
        {
            terminal_putchar(x, y, ' ', 0x0);
        }
    }
}

void print(const char* str) {
    size_t len = strlen(str);
    for(int i = 0; i < len; ++i) 
    {
        terminal_writechar(str[i], 15);
    }
}

void print_num(int num) {
    char str[15];
    int cnt = 0;
    if(num == 0) str[cnt++] = '0';
    while(num) {
        str[cnt++] = num % 10 + '0';
        num /= 10;
    }
    for(int i = cnt - 1; i >= 0; --i) {
        terminal_writechar(str[i], 15);
    }
}

void panic(const char *msg)
{
    print(msg);
    while(1){}
}

static struct paging_4gb_chunk* kernel_chunk = 0;

struct tss tss;
struct gdt gdt_real[KERNEL_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[KERNEL_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}      // TSS Segment
};

void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_chunk);
}

void kernel_main() {
    // 初始化终端
    terminal_initialize();

    print("Hello World!\ntest\n");

    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, KERNEL_TOTAL_GDT_SEGMENTS);
    gdt_load(gdt_real, sizeof(gdt_real));

    // 初始化内存管理
    kheap_init();

    // 加载文件系统
    fs_init();

    // 初始化磁盘驱动
    disk_search_and_init();

    // 初始化中断描述符表
    idt_init();

    // 初始化tss
    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    // 这个 KERNEL_DATA_SELECTOR 应该是GDT第三个entry，即内核数据段的意思
    tss.ss0 = KERNEL_DATA_SELECTOR;
    // 为什么这里是0x28，0x28 = 40，0号进程前面有5个gdt项，5 * 8 = 40
    tss_load(0x28);

    // 初始化分页虚拟内存
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    paging_switch(kernel_chunk);

    enable_paging();

    // 注册系统调用
    isr80h_register_commands();

    struct process* process = 0;
    int res = process_load("0:/blank.bin", &process);
    if(res != KERNEL_ALL_OK)
    {
        panic("Failed to load blank.bin\n");
    }
    
    task_run_first_ever_task();

    while(1){}

    // // 恢复中断
    // enable_interrupts();
}