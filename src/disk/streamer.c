#include "streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"
#include "kernel.h"

struct disk_stream* diskstreamer_new(int disk_id) {
    struct disk* disk = disk_get(disk_id);
    if(!disk) {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0;
    streamer->disk = disk;

    return streamer;   
}

int diskstreamer_seek(struct disk_stream* stream, int pos) {
    stream->pos = pos;
    return 0;
}

int diskstreamer_read(struct disk_stream* stream, void* out, int total) {
    int res = 0;
    char buf[KERNEL_SECTOR_SIZE];

    #warning 原作者在这里用的是递归，我改用循环代替
    while(total > 0) {
        int sector = stream->pos / KERNEL_SECTOR_SIZE;
        int offset = stream->pos % KERNEL_SECTOR_SIZE;
        #warning 一次只读一个扇区真的好吗？一次性读更多扇区会不会更高效？
        res = disk_read_block(stream->disk, sector, 1, buf);
        if(res < 0) {
            goto label;
        }
        
        // 我这里写的没问题，原作者就是没考虑即使total小于KERNEL_SECTOR_SIZE也会溢出的情况，那我直接判断offset+total会不会
        // 溢出，如果不会那total_to_read=total，如果会那total_to_read=KERNEL_SECTOR_SIZE-offset, 因为一次只读一个扇区
        // 其实取一个min也可以，即 total_to_read = min(total, KERNEL_SECTOR_SIZE - offset)
        // int total_to_read = (offset + total <= KERNEL_SECTOR_SIZE) ? total : (KERNEL_SECTOR_SIZE - offset);
        int total_to_read = MIN(total, KERNEL_SECTOR_SIZE - offset);

        for(int i = 0; i < total_to_read; ++i) {
            *(char*)out++ = buf[offset + i];
        }

        stream->pos += total_to_read;
        total -= total_to_read;
    }

label:
    return res;
}

void diskstreamer_close(struct disk_stream* stream) {
    kfree(stream);
} 