#include "task.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "kernel.h"
#include "config.h"
#include "memory/memory.h"

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