#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

extern void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;

struct paging_4gb_chunk *paging_new_4gb(uint8_t flags)
{
    #warning 之所以使用二级页表，不就是因为要节省页表空间吗？但是该代码却没有体现这一点，它初始化了所有页表项
    uint32_t *directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t *entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++)
        {
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
        }
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_4gb_chunk *chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

void paging_free_4gb(struct paging_4gb_chunk* chunk)
{
    if(chunk == 0) return;
    for(int i = 0; i < 1024; ++i)
    {
        uint32_t entry = chunk->directory_entry[i];
        uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);
        kfree(table);
    }
    kfree(chunk->directory_entry);
    kfree(chunk);
}

void paging_switch(struct paging_4gb_chunk* directory) {
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
}

uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk) {
    return chunk->directory_entry;
}

bool paging_is_aligned(void* virtual_address) {
    return (uint32_t)virtual_address % PAGING_PAGE_SIZE == 0;
}

int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out) {
    int res = 0;
    if(!paging_is_aligned(virtual_address)) {
        res = -EINVARG;
        goto out;
    }

    *directory_index_out = (uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
    *table_index_out = (uint32_t)virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE;

out:
    return res;
}

int paging_set(uint32_t* directory, void* virt, uint32_t val) {
    if(!paging_is_aligned(directory)) {
        return -EINVARG;
    }
    
    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    int res = paging_get_indexes(virt, &directory_index, &table_index);
    if(res < 0) {
        return res;
    }

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);
    table[table_index] = val; 

    return res;
}

void* paging_align_address(void* ptr)
{
    if((uint32_t)ptr % PAGING_PAGE_SIZE)
    {
        return (void*)((uint32_t)ptr + PAGING_PAGE_SIZE - ((uint32_t)ptr % PAGING_PAGE_SIZE));
    }
    return ptr;
}

int paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int flags)
{
    if(((unsigned int)virt % PAGING_PAGE_SIZE) || ((unsigned int)phys % PAGING_PAGE_SIZE))
    {
        return -EINVARG;
    }

    return paging_set(directory->directory_entry, virt, (uint32_t)phys | flags);
}

int paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int count, int flags)
{
    int res = 0;
    for(int i = 0; i < count; ++i)
    {
        res = paging_map(directory, virt, phys, flags);
        // 作者这里写的是 res == 0, 很明显他是错的
        if(res < 0)
            break;

        virt += PAGING_PAGE_SIZE;
        phys += PAGING_PAGE_SIZE;
    }
out:
    return res;
}

int paging_map_to(struct paging_4gb_chunk* directory, void* virt, void* phys, void* phys_end, int flags)
{
    int res = 0;
    if((uint32_t)virt % PAGING_PAGE_SIZE != 0)
    {
        res = -EINVARG;
        goto out;
    }
    if((uint32_t)phys % PAGING_PAGE_SIZE != 0)
    {
        res = -EINVARG;
        goto out;
    }
    if((uint32_t)phys_end % PAGING_PAGE_SIZE != 0)
    {
        res = -EINVARG;
        goto out;
    }

    if((uint32_t)phys_end < (uint32_t)phys)
    {
        res = -EINVARG;
        goto out;
    }

    // learn 如果以后换成不连续的内存分配, 那么这里还要改一下, 准确的说有很多地方都要改，因为在内核代码中申请完空间没有建立
    // 内存映射, 因此在内核代码看来申请的空间是不连续的, 而用户态由于有页表映射，所以没有这个问题。那是不是说内核必须申请连续
    // 的空间, 而用户程序则无所谓呢? 日后更新内存管理的时候要考虑到这一点.
    uint32_t total_bytes = phys_end - phys;
    int total_pages = total_bytes / PAGING_PAGE_SIZE;
    res = paging_map_range(directory, virt, phys, total_pages, flags);

out:
    return res;
}