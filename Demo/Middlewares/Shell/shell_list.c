#include "shell_list.h"
#include "../global.h"
#include "../Log/log.h"
#include "ring_buffer.h"
#include "string.h"
#include "xmode.h"
#include "main.h"

#define FLASH_APP_ADDR             (0x8000000)

volatile uint8_t g_xModemCommEnable = 0;
extern void shellPrintf(const char *fmt, ...);
extern ringbuffer_t dbg_rx_ring;

void dbg(uint8_t cmdSource, uint32_t instance, unsigned int  argc, unsigned char **argv) {
	uint8_t data[10] = {0};
    strncpy((char *)data, (char const *)argv[0], sizeof(data) - 1);
    data[sizeof(data) - 1] = '\0'; // 确保以空字符结束

    for (uint8_t i = 0; i < MODULE_MAX; i++) {
        if (strcasecmp((char *)data, debug_config[i].name) == 0) {
            char command = argv[1][0];

            switch (command | 0x20) {
                case 'd':
                    log_set_level(i, LOG_LEVEL_DEBUG);
                    break;
                case 'i':
                    log_set_level(i, LOG_LEVEL_INFO);
                    break;
                case 'e':
                    log_set_level(i, LOG_LEVEL_ERROR);
                    break;
                case 'w':
                    log_set_level(i, LOG_LEVEL_WARN);
                    break;
                default:
                    shellPrintf("invalid %s debug cmd.\r\n", debug_config[i].name);
            }

            log_show_status();
        }
    }
}

uint32_t xbin_uart_recv(uint8_t *buffer, uint32_t size) {
    uint32_t len;
    len = read_ringBuffer(buffer, size, &dbg_rx_ring);
    return len;
}

int xbin_boot(uint8_t cmdSource, uint32_t instance, unsigned int  argc, unsigned char **argv) {
    
    shellPrintf("XMODEM Flash Download Mode\r\n");
    shellPrintf("Target Address: 0x%08X\r\n", FLASH_APP_ADDR);
    shellPrintf("Waiting for data...\r\n");
    
    xmodem_context_t xmodem;
    xmodem_init(&xmodem, FLASH_APP_ADDR);

    g_xModemCommEnable = 1;
    clear_ringBuffer(&dbg_rx_ring);
    xmodem_error_t result = xmodem_run(&xmodem);
    g_xModemCommEnable = 0;

    vTaskDelay(100);

    switch (result) {
        case XMODEM_OK:
            shellPrintf("Download Complete!\r\n");
            shellPrintf("Received: %lu bytes\r\n", xmodem_get_bytes_received(&xmodem));
            return 0;
        
        case XMODEM_ERR_TIMEOUT:
            shellPrintf("Error: Timeout\r\n");
            break;
        
        case XMODEM_ERR_CRC:
            shellPrintf("Error: CRC Check Failed\r\n");
            break;
        
        case XMODEM_ERR_SEQ:
            shellPrintf("Error: Sequence Number Mismatch\r\n");
            break;
        
        case XMODEM_ERR_FLASH_ERASE:
            shellPrintf("Error: Flash Erase Failed\r\n");
            break;
        
        case XMODEM_ERR_FLASH_WRITE:
            shellPrintf("Error: Flash Write Failed\r\n");
            break;
        
        case XMODEM_ERR_HANDSHAKE:
            shellPrintf("Error: Handshake Failed\r\n");
            break;
        
        case XMODEM_ERR_RETRY_EXCEED:
            shellPrintf("Error: Maximum Retries Exceeded\r\n");
            break;
        
        default:
            shellPrintf("Error: Unknown Error\r\n");
            break;
    }

    return -1;
}

void software_reset(uint8_t cmdSource, uint32_t instance, unsigned int  argc, unsigned char **argv) {
    __disable_irq();         
    __NVIC_SystemReset();     
}

void system_info() {
    /* 使用 GCC 预定义宏获取编译时间 */
    const char *compile_date = __DATE__;   // 格式: "May 18 2026"
    const char *compile_time = __TIME__;   // 格式: "10:30:45"

    LOG_I(MODULE_SYS, "\r\n****************App****************\r\nCompile Date: %-s\r\nCompile Time: %-s\r\nSW:%s, HW:%s\r\n\r\n", 
      compile_date, compile_time, ECU_SW_VERSION, ECU_HW_VERSION);
}

void cpu_info(uint8_t cmdSource, uint32_t instance, unsigned int argc, unsigned char **argv) { 
    uint8_t cpu_info_buffer[512] = {0}; 

    LOG_I(MODULE_SYS, "\r\n| task_name | state | priority | stack | num |\r\n");
    vTaskList((char *)&cpu_info_buffer); 
    LOG_I(MODULE_SYS, "\r\n%s", cpu_info_buffer);  

    LOG_I(MODULE_SYS, "\r\n| name       | runtime        | use   |\r\n");
    
    vTaskGetRunTimeStats((char *)&cpu_info_buffer); 
    LOG_I(MODULE_SYS, "\r\n%s", cpu_info_buffer);

    uint32_t ulTotalTime; 
    TaskStatus_t *pxTaskStatusArray = pvPortMalloc(uxTaskGetNumberOfTasks() * sizeof(TaskStatus_t)); 
    if(pxTaskStatusArray == NULL) { 
        LOG_E(MODULE_SYS, "cpu_info malloc failed\n"); 
        return; 
    } 
    
    uxTaskGetSystemState(pxTaskStatusArray, uxTaskGetNumberOfTasks(), &ulTotalTime); 
    vPortFree(pxTaskStatusArray); 

    LOG_I(MODULE_SYS, "\r\n");
    LOG_I(MODULE_SYS, "RTOS Minimum Ever Free Heap: %10u bytes\r\n", xPortGetMinimumEverFreeHeapSize());
    LOG_I(MODULE_SYS, "System Tick Count:           %10u\r\n", xTaskGetTickCount());
    LOG_I(MODULE_SYS, "Total Task Runtime:          %10u\r\n", ulTotalTime);
    
    if (xTaskGetTickCount() > 0) {
        uint32_t kernel_overhead = xTaskGetTickCount() - ulTotalTime;
        uint32_t kernel_percent_milli = (kernel_overhead * 100000UL) / xTaskGetTickCount();
        
        char percent_str[16];
        snprintf(percent_str, sizeof(percent_str), "%lu.%03lu%%",
             kernel_percent_milli / 1000, 
             kernel_percent_milli % 1000);
        LOG_I(MODULE_SYS, "Kernel Overhead:             %10s\r\n", percent_str);
    } else {
        LOG_I(MODULE_SYS, "Kernel Overhead:             N/A\r\n");
    }
    
    uint64_t memory_used = (uint64_t)(configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize()) * 100;
    LOG_I(MODULE_SYS, "OS Memory Usage:             %9u%%\r\n", (uint32_t)(memory_used / configTOTAL_HEAP_SIZE));
}

void flash_show(uint8_t cmdSource, uint32_t instance, unsigned int argc, unsigned char **argv) { 
    LOG_D(MODULE_SHELL, 
        "param_config_t:\r\n"
        "reboot_count = %u\r\n"
        "ota_boot_crc = 0x%08X\r\n"
        "ota_app_crc = 0x%08X\r\n"
        "ota_boot_size = %u\r\n"
        "ota_app_size = %u\r\n"
        "app_valid = %u\r\n"
        "boot_valid = %u\r\n"
        "dog_reset_cnt = %u\r\n"
        "keep_boot = %u\r\n",
        config.reboot_count,
        config.ota_status.ota_boot_crc,
        config.ota_status.ota_app_crc,
        config.ota_status.ota_boot_size,
        config.ota_status.ota_app_size,
        config.ota_status.app_valid,
        config.ota_status.boot_valid,
        config.ota_status.dog_reset_cnt,
        config.ota_status.keep_boot
    );
}
