#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "kernel.h"
#include "fs/fat/fat16.h"
#include "disk/disk.h"
#include "pparser.h"
#include "string/string.h"

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

static FILE_MODE file_get_mode_by_string(const char* str) 
{
    FILE_MODE mode = FILE_MODE_INVALID;
    if(strncmp(str, "r", 1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if(strncmp(str, "w", 1) == 0) 
    {
        mode = FILE_MODE_WRITE;
    }
    else if(strncmp(str, "a", 1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }
    return mode;
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

int fopen(const char* filename, const char* mode_str) 
{
    int res = 0;

    struct path_root* path_root = pathparser_parse(filename, 0);
    if(!path_root)
    {
        res = -EINVARG;
        goto out;
    }

    if(!path_root->first)
    {
        res = -EINVARG;
        goto out;
    }

    struct disk* disk = disk_get(path_root->drive_no);
    if(!disk)
    {
        res = -EIO;
        goto out;
    }

    if(!disk->filesystem)
    {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if(mode == FILE_MODE_INVALID)
    {
        res = -EINVARG;
        goto out;
    }

    void* descriptor_private_data = disk->filesystem->open(disk, path_root->first, mode);
    if(ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc);
    if(res < 0)
    {
        goto out;
    }

    desc->disk = disk;
    desc->filesystem = disk->filesystem;
    desc->private = descriptor_private_data;
    res = desc->index;

out:
    if(res < 0)
        res = 0;

    return res;
}

static struct file_descriptor* file_get_descriptor(int fd)
{
    if(fd <= 0 || fd > KERNEL_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    int index = fd - 1;
    return file_descriptors[index];
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

int fstat(int fd, struct file_stat* stat)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EINVARG;
        goto out;
    }

    res = desc->filesystem->stat(desc->disk, desc->private, stat);

out:
    return res;
}

int fseek(int fd, int offset, FILE_SEEK_MODE whence)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EINVARG;
        goto out;
    }

    res = desc->filesystem->seek(desc->private, offset, whence);

out:
    return res;
}

int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd)
{
    int res = 0;
    if(size == 0 || nmemb == 0 || fd < 1)
    {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EINVARG;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char*)ptr);

out:
    return res;
}