#include "string.h"

int strlen(const char* ptr) 
{
    int len = 0;
    while(*ptr != 0) {
        len++;
        ptr += 1;
    }
    return len;
}

int strnlen(const char* ptr, int max) 
{
    int i = 0;
    for(i = 0; i < max; ++i) {
        if(ptr[i] == 0) {
            break;
        }
    }

    return i;
} 

int isdigit(char c) 
{
    return c >= '0' && c <= '9';
}

int tonumericdigit(char c) 
{
    return c - '0';
}

char* strcpy(char* dest, char* src) 
{
    char* res = dest;
    while(*src != 0) {
        *dest = *src;
        dest += 1;
        src += 1;
    }
    *dest = 0x00;
    return res;
}