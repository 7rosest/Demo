#include "cmsis_os.h"
#include "usart.h"
#include "../../Middlewares/global.h"
#include "../../Middlewares/Log/log.h"
#include <string.h>

uint8_t log_buffer[256] = {0};
extern osMessageQId logQueueHandle;

static void log_task(void *pvParameters) {
    LOG_I(MODULE_SYS, "Log Task Start.\r\n");
    
    while (1) {
        if (xQueueReceive(logQueueHandle, log_buffer, portMAX_DELAY) == pdTRUE) { 
            dbg_uart_send(log_buffer, sizeof(log_buffer)); 
            
            while (xQueueReceive(logQueueHandle, log_buffer, 0) == pdTRUE) { 
                dbg_uart_send(log_buffer, sizeof(log_buffer)); 
            } 
        } 
    }
}
MODULE_TASK(log_task, "LOG_Task", configMINIMAL_STACK_SIZE * 2, 1);
