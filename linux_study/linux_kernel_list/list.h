#ifndef LIST_H
#define LIST_H

struct list_head{
    struct list_head *next, *prev; 
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head* list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static inline void list_add(struct list_head *_new, struct list_head *head)
{  
    __list_add(_new, head, head->next);
}

#define prefetch(x) __builtin_prefetch(x)

// list是一个双向循环链表，所以采用下面这种遍历方式，prefectch用来预加载
#define list_for_each(pos, head) \
    for(pos = (head)->next; prefetch(pos->next), pos != (head); \   
        pos = pos->next)

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE*)0)->MEMBER)

#define container_of(ptr, type, member) ( { \
    const typeof(((type*)0)->member)* __mptr = (ptr);   \ 
            (type*)((char*)__mptr - offsetof(type, member)); })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

// 这里的 pos 指向的是容器结构，因此才有了下面的写法，这也符合 list_for_each_entry 的语义
#define list_for_each_entry(pos, head, member) \
    for(pos = list_entry((head)->next, typeof(*pos), member); \
        prefetch(pos->member.next), &pos->member != (head);  \
        pos = list_entry(pos->member.next, typeof(*pos), member))

#define LIST_POISON1 0
#define LIST_POISON2 0


static void __list_del(struct list_head* prev, struct list_head* next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}

#endif