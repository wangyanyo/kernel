#include "pparser.h"
#include "config.h"
#include "string/string.h"
#include "status.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "kernel.h"

static int pathparser_path_valid_format(const char* filename) {
    int len = strnlen(filename, KERNEL_MAX_PATH);
    return (len >= 3 && isdigit(filename[0]) && memcmp((void*)&filename[1], ":/", 2) == 0);
}

static int pathparser_get_drive_by_path(const char** path) {
    if(!pathparser_path_valid_format(*path)) {
        return -EBADPATH;
    }

    int dirve_no = tonumericdigit(*path[0]);

    *path += 3;
    return dirve_no;
}

static struct path_root* pathparser_create_root(int drive_no) {
    struct path_root* path_root = kzalloc(sizeof(struct path_root));
    path_root->drive_no = drive_no;
    path_root->first = 0;
    return path_root;
}

void pathparser_free(struct path_root* path_root) {
    struct path_part* part = path_root->first;
    while(part) {
        struct path_part* next_part = part->next;
        kfree((void*)part->part);
        kfree(part);
        part = next_part;
    }
    kfree(path_root);
}

static const char* pathparser_get_path_part(const char** path) {
    char* result_path_part = kzalloc(KERNEL_MAX_PATH);
    int i = 0;
    while(**path != '/' && **path != 0x00) {
        result_path_part[i] = **path;
        *path += 1;
        i++;
    }

    if(**path == '/') {
        *path += 1;
    }

    if(i == 0) {
        kfree(result_path_part);
        result_path_part = 0;
    }

    return result_path_part;
}

static struct path_part* pathparser_parse_path_part(struct path_part* last_part, const char** path) {
    const char* path_part_str = pathparser_get_path_part(path);
    if(!path_part_str) {
        return 0;
    }

    struct path_part* part = kzalloc(sizeof(struct path_part));
    part->part = path_part_str;
    part->next = 0x00;

    if(last_part) {
        last_part->next = part;
    }

    return part;
}

struct path_root* pathparser_parse(const char* path, const char* current_directory_path) {
    struct path_root* path_root = 0;
    const char* tmp_path = path;

    if(strlen(path) > KERNEL_MAX_PATH) {
        goto out;
    }

    int res = pathparser_get_drive_by_path(&tmp_path);
    if(res < 0) {
        goto out;
    }

    path_root = pathparser_create_root(res);
    if(!path_root) {
        goto out;
    }

    struct path_part* part = pathparser_parse_path_part(0, &tmp_path);
    if(!part) {
        goto out;
    }

    path_root->first = part;
    // 尽量不用递归
    while(part) {
        part = pathparser_parse_path_part(part, &tmp_path);
    }

out:
    return path_root;
}