#ifndef GLOBAL_H
#define GLOBAL_H

#include "FreeRTOS.h"
#include "task.h"

#define BOOT_VERSION "0.0.3"

typedef struct {
    uint32_t ota_boot_crc;        // OTA引导CRC32校验值
    uint32_t ota_app_crc;         // OTA应用CRC32校验值
    uint16_t ota_boot_size;       // OTA引导大小
    uint16_t ota_app_size;        // OTA应用大小
    uint8_t  app_valid;           // 应用是否有效
    uint8_t  boot_valid;          // 引导是否有效
    uint8_t  dog_reset_cnt;       // 狋门狗重置次数
    uint8_t  keep_boot;           // 保持在boot模式
} ota_status_t;

typedef struct {
    uint32_t reboot_count;        // 重启次数
    ota_status_t ota_status;
} param_config_t;

/* 定义一个初始化描述结构体 */
typedef struct {
    const char *name;
    int (*init)(void);     /* 初始化函数 */
} init_entry_t;

/* 定义一个任务描述结构体 */
typedef struct {
    const char *name;
    void (*task)(void*);   /* 任务函数，RTOS风格 */
    uint16_t stack_size;   /* 栈大小 */
    UBaseType_t priority;  /* 优先级 */
} task_entry_t;

/* 黑魔法宏：把结构体放到自定义段 */
#define MODULE_INIT(fn, module_name)                  \
    __attribute__((used, section(".init_table")))      \
    static const init_entry_t _init_##fn = {          \
        .name = module_name,                           \
        .init = fn,                                    \
    }

/* 任务注册宏：把任务结构体放到自定义段 */
#define MODULE_TASK(fn, task_name, task_stack_size, task_priority) \
    __attribute__((used, section(".task_table")))         \
    static const task_entry_t _task_##fn = {              \
        .name = task_name,                                \
        .task = fn,                                       \
        .stack_size = task_stack_size,                         \
        .priority = task_priority,                             \
    }

extern param_config_t config;

#endif // GLOBAL_H