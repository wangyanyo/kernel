#ifndef LIST_H
#define LIST_H

// Linux的List很巧妙，它不是把数据放到List节点上，而是把List节点放到数据对象里。
// 这样做的好处是这个链表是可复用的，任何结构想要包含链表的话，就直接把list_head放进结构里即可，非常方便。

// 我有一个疑惑，既然是数据包含链表，那能不能几个不同类型的数据对象连在一个链表上？什么场景下会用到这个？
// 怎么用？会不会出什么问题？

// 还有就是这里大量的利用了宏，但是并没有进行很多检查，所以如果程序员误用了这些宏，会有什么糟糕的后果？
// 总共有几种情况？会报错吗？

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

// __builtin_prefetch 是预取指令，是gnu环境特有的，在Windows环境下用不了
#define prefetch(x) __builtin_prefetch(x)

// list是一个双向循环链表，所以采用下面这种遍历方式，prefectch用来预加载
#define list_for_each(pos, head) \
    for(pos = (head)->next; prefetch(pos->next), pos != (head); \   
        pos = pos->next)

// offserof的原理是，&p->成员 这个语句会被翻译翻译成leaq地址计算指令，因此 &(NULL)->成员 可以得到成员的偏移量
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE*)0)->MEMBER)

// typeof 能获取一个变量的类型，当然使用宏的话，也可以直接把类型传过来
// container_of 的原理就是一个成员的实际位置减去该成员在结构中的偏移量，就会得到这个对象的位置
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

#define list_for_each_entry_reverse(pos, head, member) \
    for(pos = list_entry((head)->prev, typeof(*pos), member); \
        prefetch(pos->member.prev), &pos->member != (head);  \
        pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)  \
    for(pos = list_entry((head)->next, typeof(*pos), member),    \
        n = list_entry(pos->member.next, typeof(*pos), member); \
        &pos->member != (head);                                 \
        pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)  \
    for(pos = list_entry((head)->prev, typeof(*pos), member),    \
        n = list_entry(pos->member.prev, typeof(*pos), member); \
        &pos->member != (head);                                 \
        pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)  

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

static inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

static inline void list_add_tail(struct list_head *_new, struct list_head *head)
{
    __list_add(_new, head->prev, head);
}

static inline void list_replace(struct list_head *old, struct list_head* _new)
{
    _new->next = old->next;
    old->next->prev = _new;
    _new->prev = old->prev;
    old->prev->next = _new;
}

static inline void list_replace_init(struct list_head* old, struct list_head* _new)
{
    list_replace(old, _new);
    INIT_LIST_HEAD(old);
}

static inline void list_move(struct list_head* list, struct list_head* head)
{
    __list_del(list->prev ,list->next);
    list_add(list, head);
}

static inline void list_move_tail(struct list_head* list, struct list_head* head)
{
    __list_del(list->prev, list->next);
    list_add_tail(list, head);
}

static inline int list_is_last(const struct list_head* list, const struct list_head* head)
{
    return list->next == head;
}

static inline int list_is_empty(const struct list_head* head)
{
    return head->next == head;
}

#endif