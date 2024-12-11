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

    struct keyboard_buffer {
        char buffer[KERNEL_KEYBOARD_BUFFER_SIZE];
        int tail;
        int head;
    } keyboard;
};

int process_load(const char* filename, struct process** process);
struct process* process_current();
struct process* process_get(int process_id);

#endif