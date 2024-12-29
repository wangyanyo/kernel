#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

// ELF全称Executable and Linking Format，意思是可执行和链接格式

// ELF section类型位掩码，可执行，可写，可读
#define PF_X 0x01
#define PF_W 0x02
#define PF_R 0x04

// Segment Types
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

// Section Types
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 12
#define SHT_HIPROC 13
#define SHT_LOUSER 14
#define SHT_HIUSER 15

// ELF Type
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define EI_NIDENT 16
#define EI_CLASS 4
#define EI_DATA 5
#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#define SHN_UNDEF 0

typedef uint16_t elf32_half;    // 无符号半字整数
typedef uint32_t elf32_word;    // 无符号整数
typedef int32_t elf32_sword;    // 有符号整数
typedef uint32_t elf32_addr;    // 32位地址类型
typedef int32_t elf32_off;      // 文件偏移量，无符号整数

// Program Header
struct elf32_phdr
{
    elf32_word p_type;
    elf32_off p_offset;
    elf32_addr p_vaddr;
    elf32_addr p_paddr;
    elf32_word p_filesz;
    elf32_word p_memsz;
    elf32_word p_flags;
    elf32_word p_align;
} __attribute__((packed));

// Section Header
struct elf32_shdr
{
    elf32_word sh_name;
    elf32_word sh_type;
    elf32_word sh_flags;
    elf32_addr sh_addr;
    elf32_off sh_offset;
    elf32_word sh_size;
    elf32_word sh_link;
    elf32_word sh_info;
    elf32_word sh_addralign;
    elf32_word sh_entsize;
} __attribute__((packed));

// ELF header 
struct elf_header
{
    unsigned char e_ident[EI_NIDENT];
    elf32_half e_type;
    elf32_half e_machine;
    elf32_word e_version;
    elf32_addr e_entry;
    elf32_off e_phoff;
    elf32_off e_shoff;
    elf32_word e_flags;
    elf32_half e_ehsize;
    elf32_half e_phentsize;
    elf32_half e_phnum;
    elf32_half e_shentsize;
    elf32_half e_shnum;
    elf32_half e_shstrndx;
} __attribute__((packed));

// 
struct elf32_dyn
{
    elf32_sword d_tag;
    union 
    {
        elf32_word d_val;
        elf32_addr d_ptr;
    } d_un;
    
} __attribute__((packed));


struct elf32_sym
{
    elf32_word st_name;
    elf32_addr st_value;
    elf32_word st_size;
    unsigned char st_info;
    unsigned char st_other;
    elf32_half st_shndx;
} __attribute__((packed));


#endif