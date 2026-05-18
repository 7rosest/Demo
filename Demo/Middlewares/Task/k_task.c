#include "../global.h"
#include "../Event/event.h"
#include <stdint.h>

/* 链接器脚本里声明的段起止符号 */
extern const init_entry_t __start_init_table;
extern const init_entry_t __stop_init_table;

/* 任务表段符号 */
extern const task_entry_t __start_task_table;
extern const task_entry_t __stop_task_table;

/**
 * @brief 系统初始化函数
 * 
 * @return void
 * */   
void k_system_init(void) {
    // 初始化事件管理器
    event_manager_init(&g_event_mgr);

    const init_entry_t *entry;
    for (entry = &__start_init_table;
         entry < &__stop_init_table;
         entry++) {
        entry->init();
    }
}

/**
 * @brief 启动所有任务
 * 
 * @return void
 */
void k_start_tasks(void) {
    const task_entry_t *task_entry;
    for (task_entry = &__start_task_table;
         task_entry < &__stop_task_table;
         task_entry++) {
        xTaskCreate(task_entry->task, task_entry->name, task_entry->stack_size, NULL, task_entry->priority, NULL);
    }
}