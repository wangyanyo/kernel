#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "task.h"
#include "config.h"

struct process
{
    uint16_t id;

    char filename[KERNEL_MAX_PATH];

    struct task* task;

    void* allocations[KERNEL_MAX_PROGRAM_ALLOCATIONS];

    void* ptr;

    void* stack;

    uint32_t size;
};

#endif