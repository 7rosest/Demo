#include "../global.h"
#include "event.h"

/* 全局事件管理器定义 */
event_manager_t g_event_mgr;

/**
 * @brief 初始化通知链
 * 
 * @param chain 待初始化的通知链
 * @return void
 */   
static void notifier_chain_init(notifier_chain_t *chain) {
    chain->head.next = &chain->head;
    chain->head.prev = &chain->head;
}

/**
 * @brief 注册观察者（按优先级插入）
 * 
 * @param chain 待注册观察者的通知链
 * @param n 观察者节点
 * @return void
 */   
static void notifier_register(notifier_chain_t *chain, notifier_node_t *n) {
    list_node_t *pos;
    notifier_node_t *entry;

    /* 找到第一个优先级比自己低的，插在它前面 */
    for (pos = chain->head.next; pos != &chain->head; pos = pos->next) {
        entry = container_of(pos, notifier_node_t, node);
        if (entry->priority < n->priority)
            break;
    }
    /* 插到pos前面 */
    n->node.next = pos;
    n->node.prev = pos->prev;
    pos->prev->next = &n->node;
    pos->prev = &n->node;
}

/**
 * @brief 发送通知：遍历链表，依次调用每个handler
 * 
 * @param chain 待发送通知的通知链
 * @param event 事件类型
 * @param data 事件数据
 * @return void
 */   
static void notifier_call(notifier_chain_t *chain, int event, void *data) {
    list_node_t *pos;
    notifier_node_t *entry;

    for (pos = chain->head.next; pos != &chain->head; pos = pos->next) {
        entry = container_of(pos, notifier_node_t, node);
        if (entry->handler(event, data) != 0)
            break;  /* handler返回非0表示"到此为止，别往下传了" */
    }
}

/**
 * @brief 初始化事件管理器
 * 
 * @param mgr 待初始化的事件管理器
 * @return void
 */   
void event_manager_init(event_manager_t *mgr) {
    notifier_chain_init(&mgr->chain);
}

/**
 * @brief 注册观察者到共享事件链
 * 
 * @param mgr 待注册观察者到的事件管理器
 * @param n 观察者节点
 * @return void
 */   
void notifier_register_event(event_manager_t *mgr, notifier_node_t *n) {
    if (!mgr || !n) return;
    notifier_register(&mgr->chain, n);
}

/**
 * @brief 发送通知到特定事件类型
 * 
 * @param mgr 事件管理器
 * @param event 事件类型
 * @param data 事件数据
 * @return void
 */
void notifier_call_event(event_manager_t *mgr, int event, void *data) {
    if (!mgr) return;
    notifier_call(&mgr->chain, event, data);
}
