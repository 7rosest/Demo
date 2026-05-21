#include "global.h"
#include "../Middlewares/Task/k_task.h"
#include "../Middlewares/Log/log.h"

extern volatile uint8_t g_xModemCommEnable;
extern uint32_t uart_activity_time;

#define UART_TIMEOUT_MS            (5000)  

/**
 * @brief 系统管理任务函数
 * 
 * @param pvParameters 参数
 */
static void sys_task(void *pvParameters)
{
    uint32_t last_activity_time = xTaskGetTickCount();
    uint16_t last_recv_ptr = 0;
    uint32_t timeout_ticks = UART_TIMEOUT_MS;
    
    LOG_I(MODULE_SYS, "Sys Task Started\r\n");

    while(1)
    {
        if (g_xModemCommEnable) {
            uart_activity_time = xTaskGetTickCount();
        } else {
            if ((xTaskGetTickCount() - uart_activity_time) >= timeout_ticks) {
                LOG_I(MODULE_SYS, "[TIMEOUT] no uart activity for %d seconds, jumping to app...\r\n", UART_TIMEOUT_MS / 1000);
                extern void jump_to_app(void);
                jump_to_app();
            }
        }
        
        vTaskDelay(100);
    }
}
MODULE_TASK(sys_task, "Sys_Task", 256, 1);
