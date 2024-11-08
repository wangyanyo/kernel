#include "memory.h"

void* memset(void* ptr, int c, size_t size) {
    int* i_ptr = (int*) ptr;
    int t = c & 0xFF;
    int res = (t << 24) | (t << 16) | (t << 8) | t;

    int i, limit = size - 3;
    for(i = 0; i < limit; i += 4) {
        *i_ptr = res;
        i_ptr++;
    }

    char* c_ptr = (char*)i_ptr;
    for(; i < size; ++i) {
        *c_ptr = (char)t;
        c_ptr++;
    }

    return ptr;
}

int memcmp(void* s1, void* s2, int count) {
    char* c1 = s1;
    char* c2 = s2;
    while(count-- > 0) {
        if(*c1++ != *c2++) {
            return (c1[-1] < c2[-1]) ? -1 : 1;
        }
    }

    return 0;
}

void* memcpy(void* dest, void* src, int len)
{
    char* d = dest;
    char* s = src;
    while(len-- > 0)
    {
        *d = *s;
    }
    return dest;
}