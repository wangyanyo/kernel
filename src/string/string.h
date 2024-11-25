#ifndef STRING_H
#define STRING_H

int strlen(const char* ptr);
int strnlen(const char* ptr, int max);
int isdigit(char c);
int tonumericdigit(char c);
char* strcpy(char* dest, char* src);
char* strncpy(char* dest, const char* src, int count);

int strncmp(const char* str1, const char* str2, int n);
char tolower(char s1);
int istrncmp(const char* str1, const char* str2, int n);
int strnlen_terminator(const char* str, int max, char terminator);

#endif