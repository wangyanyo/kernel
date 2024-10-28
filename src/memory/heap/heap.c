#include "heap.h"
#include "memory/memory.h"
#include "status.h"
#include <stdbool.h>

static bool heap_validate_alignment(void* ptr) {
    return ((unsigned int)ptr % KERNEL_HEAP_BLOCK_SIZE) == 0;
}

static int heap_validate_table(void* ptr, void* end, struct heap_table* table) {
    int res = 0;

    size_t table_size = (size_t)(end - ptr);
    size_t table_blocks = table_size / KERNEL_HEAP_BLOCK_SIZE;
    if(table_blocks != table->total) {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table) {
    int res = 0;

    if(!heap_validate_alignment(ptr) || !heap_validate_alignment(end)) {
        res = -EINVARG;
        goto out;
    }

    memset(ptr, 0, sizeof(struct heap));

    res = heap_validate_table(ptr, end, table);
    if(res < 0) {
        goto out;
    }

    heap->saddr = ptr;
    heap->table = table;

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

static size_t heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry) {
    return entry & 0xFF;
}

static int heap_get_start_block(struct heap* heap, int total_blocks) {
    struct heap_table* table = heap->table;
    int bc = 0;
    int bs = -1;
    for(int i = 0; i < table->total; ++i) {
        if(heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE) {
            bc = 0;
            bs = -1;
            continue;
        }
        if(bs == -1) {
            bs = i;
        }
        bc++;
        if(bc == total_blocks) {
            break;
        }
    }
    if(bs == -1) {
        return -ENOMEM;
    }
    return bs;
}

static void* heap_block_to_address(struct heap* heap, int block) {
    return heap->saddr + (block * KERNEL_HEAP_BLOCK_SIZE);
}

static void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks) {
    struct heap_table* table = heap->table;
    int end_block = start_block + total_blocks - 1;

    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TOKEN | HEAP_BLOCK_IS_FIRST;
    if(total_blocks > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for(int i = start_block; i <= end_block; ++i) {
        table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TOKEN;
        if(i != end_block - 1) {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

static void* heap_malloc_blocks(struct heap* heap, size_t total_blocks) {
    void* address = 0;

    int start_block = heap_get_start_block(heap, total_blocks);
    if(start_block < 0) {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

static size_t heap_align_value_to_upper(size_t size) {
    if(size % KERNEL_HEAP_BLOCK_SIZE == 0) {
        return size;
    }

    size -= size % KERNEL_HEAP_BLOCK_SIZE;
    size += KERNEL_HEAP_BLOCK_SIZE;

    return size;
}

void* heap_malloc(struct heap* heap, size_t size) {
    size_t aligned_size = heap_align_value_to_upper(size);
    size_t total_blocks = aligned_size / KERNEL_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

static int heap_adress_to_block(struct heap* heap, void* address) {
    return (size_t)(address - heap->saddr) / KERNEL_HEAP_BLOCK_SIZE;
}

static void heap_mark_blocks_free(struct heap* heap, int start_block) {
    struct heap_table* table = heap->table;
    for(int i = start_block; i < (int)table->total; ++i) {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        if(!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}

void heap_free(struct heap* heap, void* address) {
    heap_mark_blocks_free(heap, heap_adress_to_block(heap, address));
}