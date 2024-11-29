#include <stdio.h>
#include <stdlib.h>

#include "list.h"

struct person {
    int age;
    struct list_head list;
};

int main(int argc, char* argv[])
{
    struct person *p;
    struct person head;
    struct person *pos;

    INIT_LIST_HEAD(&head.list);

    for(int i = 0; i < 5; ++i) {
        p = (struct person*)malloc(sizeof(struct person));
        p->age = i * 10;
        list_add(p, &head.list);
    }

    list_for_each_entry(pos,&head.list,list) {
        if (pos->age == 30) {
            list_del(&pos->list);
            break;
        }
    }
    
    list_for_each_entry(pos,&head.list,list) {
        printf("age = %d\n",pos->age);
    }
    return 0;
}
