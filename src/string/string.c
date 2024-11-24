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

char* strncpy(char* dest, char* src, int n)
{
    char* res = dest;
    for(int i = 0; i < n && *src != 0; ++i)
    {
        *dest = *src;
        dest += 1;
        src += 1;
    }
    *dest = 0x00;
    return res;
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

int strncmp(const char* str1, const char* str2, int n) 
{
    unsigned char u1, u2;
    while(n-- > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if(u1 != u2)
            return u1 - u2;

        if(u1 == '\0')
            return 0;
    }

    return 0;
}

char tolower(char s1)
{
    if(s1 >= 'A' && s1 <= 'Z')
        s1 = s1 - 'A' + 'a';
    return s1;
}

int istrncmp(const char* str1, const char* str2, int n)
{
    unsigned char u1, u2;
    while(n-- > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if(u1 != u2 && tolower(u1) != tolower(u2))
            return u1 - u2;

        if(u1 == '\0')
            return 0;
    }

    return 0;
}

int strnlen_terminator(const char* str, int max, char terminator)
{
    int i = 0;
    for(i = 0; i < max; ++i)
    {
        if(str[i] == '\0' || str[i] == terminator)
            break;
    }

    return i;
}