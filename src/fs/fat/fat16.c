#include "fat16.h"
#include "string/string.h"
#include <stdint.h>
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "memory/memory.h"

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);
int fat16_resolve(struct disk* disk);

#define KERNEL_FAT16_SIGNATURE 0x29
#define KERNEL_FAT16_FAT_ENTRY_SIZE 0x02
#define KERNEL_FAT16_BAD_SECTOR 0xFF7
#define KERNEL_FAT16_UNUSED 0x00

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

// Fat directory entry attributes bitmask
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80

struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
}__attribute__((packed));

struct fat_header
{
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_setors;
    uint32_t sectors_big;
}__attribute__((packed));

struct fat_h
{
    struct fat_header primary_header;
    union 
    {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_item
{
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

struct fat_directory
{
    struct fat_directory_item* item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item
{
    union
    {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    };

    FAT_ITEM_TYPE type;
};

struct fat_item_descriptor
{
    struct fat_item* item;
    uint32_t pos;
};

struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    struct disk_stream* cluster_read_stream;
    struct disk_stream* fat_read_stream;
    struct disk_stream* directory_stream;
};

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

static int fat_init_private(struct disk* disk, struct fat_private* private) {
    int res = 0;
    memset(private, 0x00, sizeof(struct fat_private));
    private->cluster_read_stream = diskstreamer_new(disk->id);
    private->directory_stream = diskstreamer_new(disk->id);
    private->fat_read_stream = diskstreamer_new(disk->id);
    if(private->cluster_read_stream == 0 || private->directory_stream == 0 || private->fat_read_stream == 0)
    {
        res = -ENOMEM;
        goto out;
    }

out:
    return res;
}

static int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector)
{
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0x00, sizeof(empty_item));

    struct fat_private* fat_private = disk->fs_private;

    int res = 0;
    int i = 0;
    int directory_start_pos = disk->sector_size * directory_start_sector;
    struct disk_stream* stream = fat_private->directory_stream;
    if(diskstreamer_seek(stream, directory_start_pos) != KERNEL_ALL_OK)
    {
        res = -EIO;
        goto out;
    }
    
    while(1)
    {
        if(diskstreamer_read(stream, &item, sizeof(item)) != KERNEL_ALL_OK)
        {
            res = -EIO;
            goto out;
        }

        if(item.filename[0] == 0x00)
        {
            break;
        }

        // unused item
        if(item.filename[0] == 0xE5)
        {
            continue;
        }

        i++;
    }

    res = i;
out:
    return res;
}

static int fat16_sector_to_absolute(struct disk* disk, int sector)
{
    return sector * disk->sector_size;
}

static int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory)
{
    int res = 0;

    struct fat_header* primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entries = primary_header->root_dir_entries;
    int root_dir_size = root_dir_entries * sizeof(struct fat_directory_item);
    int total_sectors = root_dir_size / disk->sector_size;
    if(root_dir_size % disk->sector_size) 
    {
        total_sectors += 1;
    }

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);

    struct fat_directory_item* dir = kzalloc(root_dir_size);
    if(!dir)
    {
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream* stream = fat_private->directory_stream;
    if(diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != KERNEL_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    if(diskstreamer_read(stream, dir, root_dir_size) != KERNEL_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    directory->item = dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + total_sectors;

out:
    return res;
}

static void fat_free_private(struct fat_private* fat_private, struct disk* disk) 
{
    kfree(fat_private->cluster_read_stream);
    kfree(fat_private->directory_stream);
    kfree(fat_private->fat_read_stream);
    kfree(fat_private);
    disk->fs_private = 0;
    disk->filesystem = 0;
}

int fat16_resolve(struct disk* disk)
{
    int res = 0;
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));

    if(fat_init_private(disk, fat_private) != KERNEL_ALL_OK)
    {
        res = -ENOMEM;
        goto out;
    }

    disk->fs_private = fat_private;
    disk->filesystem = &fat16_fs;    

    struct disk_stream* stream = diskstreamer_new(disk->id);
    if(!stream) 
    {
        res = -ENOMEM;
        goto out;
    }

    if(diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != KERNEL_ALL_OK) 
    {
        res = -EIO;
        goto out;
    }

    if(fat_private->header.shared.extended_header.signature != 0x29) 
    {
        res = -EFSNOTUS;
        goto out;
    }

    if(fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != KERNEL_ALL_OK) 
    {
        res = -EIO;
        goto out;
    }

    
    
out:
    if(stream) 
    {
        diskstreamer_close(stream);
    }

    if(res < 0)
    {
        fat_free_private(fat_private, disk);
    }

    return res;
}

