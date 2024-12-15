#include "process.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"
#include "fs/file.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "string/string.h"

static struct process* current_process = 0;

static struct process* processes[KERNEL_MAX_PROCESSES] = {};

static void process_init(struct process* process)
{
    memset(process, 0x00, sizeof(struct process));
}

struct process* process_current()
{
    return current_process;
}

void process_switch(struct process* process)
{
    current_process = process;
}

struct process* process_get(int process_id)
{
    if(process_id < 0 || process_id >= KERNEL_MAX_PROCESSES)
    {
        return NULL;
    }
    return processes[process_id];
}

static int process_load_binary(const char* filename, struct process* process)
{
    int res = 0;

    int fd = fopen(filename, "r");
    if(!fd)
    {
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if(res != KERNEL_ALL_OK)
    {
        goto out;
    }

    void* program_data_ptr = kzalloc(stat.filesize);
    if(!program_data_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    if(fread(program_data_ptr, stat.filesize, 1, fd) != 1)
    {
        res = -EIO;
        goto out;
    }

    process->ptr = program_data_ptr;
    process->size = stat.filesize;

out:
    fclose(fd);
    return res;
}

static int process_load_data(const char* filename, struct process* process)
{
    int res = 0;
    // 目前加载的就是二进制文件，即将PC设置好就能运行，没有考虑链接以及动态链接
    // 以后会加入ELF格式的文件并且在该函数内会有一个switch
    res = process_load_binary(filename, process);
    return res;
}

static int process_map_binary(struct process* process)
{
    int res = 0;

    #warning 原作者这里没加"res =", 很明显他是错的
    #warning 而且为什么没有定位 process->stack, 感觉这个有问题
    res = paging_map_to(process->task->page_directory, (void*)KERNEL_PROGRAM_VIRTUAL_ADDRESS, 
        process->ptr, paging_align_address(process->ptr + process->size), 
        PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);

    return res;
}

static int process_map_memory(struct process* process)
{
    int res = 0;
    // 和process_load_data一样，未来还会更新。
    res = process_map_binary(process);
    if(res < 0) {
        goto out;
    }

    // 映射用户栈，但我感觉这个栈怪怪的，这个栈是小于0x400000的，也就是小于程序初始地址
    paging_map_to(process->task->page_directory, (void *)KERNEL_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, 
        paging_align_address(process->stack + KERNEL_USER_PROGRAM_STACK_SIZE), 
        PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);

out:
    return res;
}

static void process_free(struct process* process)
{
    task_free(process->task);
    kfree(process->ptr);
    kfree(process->stack);
    kfree(process);
}

static int process_load_for_slot(const char* filename, struct process** process, int process_slot)
{
    int res = 0;
    struct process* _process = 0;
    struct task* task = 0;
    void* program_stack_ptr = 0;

    if(process_get(process_slot) != 0)
    {
        res = -EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process));
    if(!_process)
    {
        res = -ENOMEM;
        goto out;
    }
    
    process_init(_process);
    res = process_load_data(filename, _process);
    if(res < 0)
    {
        goto out;
    }

    program_stack_ptr = kzalloc(KERNEL_USER_PROGRAM_STACK_SIZE);
    if(!program_stack_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->stack = program_stack_ptr;
    _process->id = process_slot;

    task = task_new(_process);
    if(ERROR_I(task) == 0)
    {
        res = ERROR_I(task);
        _process->task = NULL;
        goto out;
    }

    _process->task = task;

    res = process_map_memory(_process);
    if(res < 0)
    {
        goto out;
    }

    *process = _process;

    processes[process_slot] = _process;
    
out:
    if(ISERR(res))
    {
        // free the process data
        // 出错后，在这里统一回收
        process_free(_process);
    }

    return res;
}

static int process_get_free_slot()
{
    for(int i = 0; i < KERNEL_MAX_PROCESSES; ++i)
    {
        if(!processes[i])
            return i;
    }
    return -EISTKN;
}

int process_load(const char* filename, struct process** process)
{
    int res = 0;

    int process_slot = process_get_free_slot();
    if(process_slot < 0)
    {   
        res = -EISTKN;
        goto out;
    }

    res = process_load_for_slot(filename, process, process_slot);

out:
    return res;
}

int process_load_switch(const char* filename, struct process** process)
{
    int res = process_load(filename, process); 
    if(res == KERNEL_ALL_OK) {
        process_switch(*process);
    } 

    return res;
}
