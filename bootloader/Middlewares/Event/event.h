#ifndef EVENT_H
#define EVENT_H

#include <stdio.h>
#include <stdint.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* 链表节点——极简，只管串联 */
typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
} list_node_t;

enum {
    EVENT_SYS_STARTUP = 0,   /* 系统启动事件 */
    EVENT_CHARGE_START,      /* 充电开始事件 */
    EVENT_CHARGE_STOP,       /* 充电停止事件 */
    EVENT_SLEEP_MODE,        /* 进入睡眠模式事件 */
};

/* 通知链节点：结构体 + 函数指针 + 链表，三件套齐了 */
typedef struct notifier_node {
    int  (*handler)(int event, void *data);  /* 函数指针：回调 */
    int  priority;                           /* 优先级 */
    list_node_t node;                        /* 链表节点 */
} notifier_node_t;

/* 通知链头 */
typedef struct {
    list_node_t head;
} notifier_chain_t;

/* 事件管理器：单个共享通知链 */
typedef struct {
    notifier_chain_t chain;
} event_manager_t;

extern event_manager_t g_event_mgr;

void event_manager_init(event_manager_t *mgr);
void notifier_register_event(event_manager_t *mgr, notifier_node_t *n);
void notifier_call_event(event_manager_t *mgr, int event, void *data);

#endif 