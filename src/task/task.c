#include "task.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "kernel.h"
#include "config.h"
#include "memory/memory.h"
#include "idt/idt.h"
#include "memory/paging/paging.h"
#include "string/string.h"

struct task* current_task = 0;
struct task* task_head = 0;
struct task* task_tail = 0;

static int task_init(struct task* task, struct process* process)
{
    memset(task, 0, sizeof(struct task));
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if(!task->page_directory)
    {
        return -EIO;
    }

    task->registers.ip = KERNEL_PROGRAM_VIRTUAL_ADDRESS;
    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.esp = KERNEL_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    task->process = process;

    return 0;
}

struct task* task_new(struct process* process)
{
    int res = 0;
    struct task* task = kzalloc(sizeof(struct task));
    if(!task)
    {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if(res != KERNEL_ALL_OK)
    {
        goto out;
    }

    if(task_head == 0)
    {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }    
    
    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;
    
out:
    if(ISERR(res) && task)
    {
        task_free(task);
        return ERROR(res);
    }
    return task;
}

struct task* task_current()
{
    return current_task;
}

struct task* task_get_next()
{
    if(!current_task->next)
    {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove(struct task* task)
{
    if(task == 0) return;
    #warning 我的链表删除和原作者的不太一样
    if(task_head == task_tail)
    {
        // 如果链表只有一个元素
        task_head = 0;
        task_tail = 0;
    }
    else if(task == task_head)
    {
        task_head = task_head->next;
        task_head->prev = 0;
    }
    else if(task == task_tail)
    {
        task_tail = task_tail->prev;
        task_tail->next = 0;
    }
    else
    {
        task->prev->next = task->next;
        task->next->prev = task->prev;
    }

    if(task == current_task)
    {
        current_task = task_get_next();
    }
}

int task_free(struct task* task)
{
    if(task == 0) return 0;
    paging_free_4gb(task->page_directory);
    task_list_remove(task);

    kfree(task);
    return 0;
}

int task_switch(struct task* task)
{
    current_task = task;
    paging_switch(task->page_directory);
    return 0;
}

int task_page()
{
    user_registers();
    task_switch(current_task);
    return 0;
}

void task_run_first_ever_task()
{
    #warning 作者这里用的是 if(!current_task) 我觉得不符合逻辑，应该用 if(!task_head)
    if(!task_head)
    {
        panic("task_run_first_ever_task(): No task_head exists!\n");
    }

    task_switch(task_head);
    task_return(&task_head->registers);
}

void task_save_state(struct task* task, struct interrupt_frame* frame)
{
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
    task->registers.eax = frame->eax;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.ecx = frame->ecx;
    task->registers.edi = frame->edi;
    task->registers.edx = frame->edx;
    task->registers.esi = frame->esi;
}

void task_current_save_state(struct interrupt_frame* frame)
{
    if(!task_current()) {
        panic("No current task to save\n");
    }   

    struct task *task = task_current();
    task_save_state(task, frame);
}

int copy_string_from_task(struct task *task, void *virtual, void *phys, int max)
{
    int res = 0;
    #warning 这里只能复制小于一个页的字符串, 我觉得可以改进
    if(max >= PAGING_PAGE_SIZE) {
        res = -EINVARG;
        goto out;
    }

    #warning 其实这里还有一种情况, 那就是tmp==virtual%4096的情况, 可以选一个virtual永远不会等于的值, 比如virtual的下一页
    void *tmp = kzalloc(max);
    if(!tmp) {
        res = -ENOMEM;
        goto out;
    }

    uint32_t old_entry = paging_get(task->page_directory, tmp);
    paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(task->page_directory);
    strncpy(tmp, virtual, max);
    kernel_page();

    res = paging_set(task->page_directory->directory_entry, tmp, old_entry);
    if(res < 0) {
        res = -EIO;
        goto out_free;
    }
    
    // 这里不用给字符串后面加0, 因为tmp一开始就全是0, 而且 max < PAGING_PAGE_SIZE
    strncpy(phys, tmp, max);

out_free:
    kfree(tmp);
out:
    return res;
}

int task_page_task(struct task *task)
{
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

void *task_get_stack_item(struct task *task, int index)
{
    void *result = 0;
    
    uint32_t* sp_ptr = (uint32_t *)task->registers.esp;

    task_page_task(task);

    #warning 如果参数不是四个字节怎么办? 比如是一个char, 还有就是result是在内核的栈里, 在用户态是访问不到这个result的 \
        所以这个result是默认使用寄存器存储的吗? 
    result = (void *) sp_ptr[index];

    kernel_page();

    return result;
}
