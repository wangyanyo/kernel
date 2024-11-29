#include "list.h"
#include <stdio.h>

struct person
{
    int age;
    int id;
    struct list_head list;
};

// 这里解释了为什么offsetof是正确的，&p->age不会真的访问内存，而是返回成员的偏移量
// 同样的，再编译阶段，&((TYPE*)0)->MEMBER 就会被识别成上面格式，并且只被翻译成一个leaq指令，即地址计算指令
static void for_test(struct person* p)
{
    printf("%u\n%u\n", &p->age, &p->id);
}

int main(int argc, char* argv[])
{
    struct person p;
    for_test(NULL);
}