#include "fat16.h"
#include "string/string.h"

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);
int fat16_resolve(struct disk* disk);

struct filesystem fat16_fs = 
{
    .open = fat16_open,
    .resolve = fat16_resolve
};

struct filesystem* fat16_init() 
{
    strcpy(fat16_fs.name, "fat16");
    return &fat16_fs;
}

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    return 0;
}

int fat16_resolve(struct disk* disk)
{
    return 0;
}

