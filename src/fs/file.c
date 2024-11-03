#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "kernel.h"
#include "fs/fat/fat16.h"

struct filesystem* filesystems[KERNEL_MAX_FILESYSTEM];
struct file_descriptor* file_descriptors[KERNEL_MAX_FILE_DESCRIPTORS];

static struct filesystem** fs_get_free_filesystem() 
{
    for(int i = 0; i < KERNEL_MAX_FILESYSTEM; ++i) 
    {
        if(filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }
    return 0;
}

void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs = fs_get_free_filesystem();
    if(!fs) 
    {
        print("Problem inserting filesystem");
        while(1){}
    }
    *fs = filesystem;
}

static void fs_static_load() 
{
    fs_insert_filesystem(fat16_init());
}

static void fs_load()
{
    memset(filesystems, 0x00, sizeof(filesystems));
    fs_static_load();
}

void fs_init() 
{
    memset(file_descriptors, 0x00, sizeof(file_descriptors));
        
    fs_load();
}

int fopen(const char* filename, const char* mode) 
{
    return -EIO;
}

struct filesystem* fs_resolve(struct disk* disk) 
{
    for(int i = 0; i < KERNEL_MAX_FILESYSTEM; ++i) 
    {
        if(filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0)
        {
            return filesystems[i];
        }
    }

    return 0;
}

static int file_new_descriptor(struct file_descriptor** desc_out) 
{
    int res = -ENOMEM;
    for(int i = 0; i < KERNEL_MAX_FILE_DESCRIPTORS; ++i)
    {
        if(file_descriptors[i] == 0) 
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }
    
    return res;
}