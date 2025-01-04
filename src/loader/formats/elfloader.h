#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "config.h"

struct elf_file {
    char filename[KERNEL_MAX_PATH];

    int in_memory_size;

    // ELF文件被加载到的地方，暂时没有mmap映射，因此只能全量加载。
    void *elf_memory;

    void *virtual_base_address;

    void *virtual_end_address;

    void *physical_base_address;
    
    void *physical_end_address;

};

#endif