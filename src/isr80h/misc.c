#include "isr80h/misc.h"
#include "task/task.h"
#include "kernel.h"

void *isr80h_command0_sum(struct interrupt_frame* frame)
{
    int v2 = (int)task_get_stack_item(task_current(), 1);
    int v1 = (int)task_get_stack_item(task_current(), 0);
    print_num_ln(v1 + v2);
    return (void *)(v1 + v2);
}