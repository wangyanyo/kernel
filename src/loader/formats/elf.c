#include "elf.h"

void *elf_get_entry_ptr(struct elf_header *elf_header)
{
    // e_entry 是程序要被加载到的虚拟地址
    return (void *)elf_header->e_entry;
}

uint32_t elf_get_entry(struct elf_header *elf_header)
{
    return (uint32_t)elf_header->e_entry;
}