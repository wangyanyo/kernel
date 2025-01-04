#include "elfloader.h"
#include <stdbool.h>
#include "memory/memory.h"
#include "status.h"

const char *elf_signature[] = {0x7f, 'E', 'L', 'F'};

static bool elf_vaild_signature(void *buffer)
{
    return memcpy(buffer, (void *)elf_signature, sizeof(elf_signature)) == 0;
}

static bool elf_vaild_class(struct elf_header *header)
{
    // 我们只支持32位
    return header->e_ident[EI_CLASS] == ELFCLASSNONE || header->e_ident[EI_CLASS] == ELFCLASS32;
}

static bool elf_valid_encoding(struct elf_header* header)
{
    return header->e_ident[EI_DATA] == ELFDATANONE || header->e_ident[EI_DATA] == ELFDATA2LSB;
}

static bool elf_is_executable(struct elf_header *header)
{
    return header->e_type == ET_EXEC && header->e_entry >= KERNEL_PROGRAM_VIRTUAL_ADDRESS;
}

static bool elf_has_program_header(struct elf_header *header)
{
    return header->e_phoff != 0;
}

void *elf_memory(struct elf_file* file)
{
    return file->elf_memory;
}

struct elf_header *elf_header(struct elf_file *file)
{
    return file->elf_memory;
}

struct elf32_shdr *elf_sheader(struct elf_header *header)
{
    // header->e_shoff其实就是section相对于header的偏移量，加上之后正好得到section的真实地址。
    return (struct elf32_shdr *)((int)header + header->e_shoff);
}

struct elf32_phdr *elf_pheader(struct elf_header *header)
{   
    if(header->e_phoff == 0) {
        return 0;
    }
    return (struct elf32_phdr *)((int)header + header->e_phoff);
}

struct elf32_phdr *elf_program_header(struct elf_header *header, int index)
{
    return &elf_pheader(header)[index];
}

struct elf32_shdr *elf_section(struct elf_header * header, int index)
{
    return &elf_sheader(header)[index];
}

char *elf_str_table(struct elf_header *header)
{
    // section也有头部，其主体内容相对于setion也有偏移量，于是为了获得字符串表有了下面代码
    // header + section偏移量 + section内偏移量
    return (char *)header + elf_section(header, header->e_shstrndx)->sh_offset;
}

void *elf_virtual_base(struct elf_file *file)
{
    return file->virtual_base_address;
}

void *elf_virtual_end(struct elf_file *file)
{
    return file->virtual_end_address;
}

void *elf_phys_base(struct elf_file *file)
{
    return file->physical_base_address;
}

void *elf_phys_end(struct elf_file *file)
{
    return file->physical_end_address;
}

int elf_validate_loaded(struct elf_header* header)
{
    return (elf_vaild_signature(header) && elf_vaild_class(header) && elf_valid_encoding(header) && elf_has_program_header(header)) ? 
        KERNEL_ALL_OK : -EINVARG;
}
