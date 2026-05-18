#include "shell_list.h"
#include "../global.h"
#include "../Log/log.h"
#include "ring_buffer.h"
#include "stm32f407xx.h"
#include "string.h"
#include "xmode.h"
#include "main.h"

#define FLASH_APP_ADDR             (0x8020000)

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

int xbin_app(uint8_t cmdSource, uint32_t instance, unsigned int  argc, unsigned char **argv) {
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
            return -1;
        
        case XMODEM_ERR_CRC:
            shellPrintf("Error: CRC Check Failed\r\n");
            return -1;
        
        case XMODEM_ERR_SEQ:
            shellPrintf("Error: Sequence Number Mismatch\r\n");
            return -1;
        
        case XMODEM_ERR_FLASH_ERASE:
            shellPrintf("Error: Flash Erase Failed\r\n");
            return -1;
        
        case XMODEM_ERR_FLASH_WRITE:
            shellPrintf("Error: Flash Write Failed\r\n");
            return -1;
        
        case XMODEM_ERR_HANDSHAKE:
            shellPrintf("Error: Handshake Failed\r\n");
            return -1;
        
        case XMODEM_ERR_RETRY_EXCEED:
            shellPrintf("Error: Maximum Retries Exceeded\r\n");
            return -1;
        
        default:
            shellPrintf("Error: Unknown Error\r\n");
            return -1;
    }
}

/* 定义类型 */
typedef void (*pFunction)(void);

/* APP flash address */
#define APP_FLASH_ADDR             (0x8020000)

void jump_to_app(void) {
    uint32_t JumpAddress;
    pFunction Jump_To_Application;

    /* 检查栈顶地址是否合法 */
    if(((*(__IO uint32_t *)APP_FLASH_ADDR) & 0x2FFE0000) == 0x20020000) {

  LOG_I(MODULE_SHELL,"APP_FLASH_ADDR: 0x%08X\n", APP_FLASH_ADDR);
  LOG_I(MODULE_SHELL,"Stack Top: 0x%08X\n", *(__IO uint32_t *)APP_FLASH_ADDR);
  LOG_I(MODULE_SHELL,"Reset Vector: 0x%08X\n", *(__IO uint32_t *)(APP_FLASH_ADDR + 4));
        
        /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
        __disable_irq();

        SysTick->CTRL=0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;

        HAL_RCC_DeInit();    

        SCB->VTOR = APP_FLASH_ADDR;

        /* 用户代码区第二个 字 为程序开始地址(复位地址) */
        JumpAddress = *(__IO uint32_t *) (APP_FLASH_ADDR + 4);

        /* Initialize user application's Stack Pointer */
        /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
        __set_MSP(*(__IO uint32_t *) APP_FLASH_ADDR);

        /* 类型转换 */
        Jump_To_Application = (pFunction) JumpAddress;

        __set_CONTROL(0);

        /* 跳转到 APP */
        Jump_To_Application();
    }
}

void system_info() {
    /* 使用 GCC 预定义宏获取编译时间 */
    const char *compile_date = __DATE__;   // 格式: "May 18 2026"
    const char *compile_time = __TIME__;   // 格式: "10:30:45"
    
    LOG_I(MODULE_SHELL, "\r\n");
    LOG_I(MODULE_SHELL, "╔════════════════════════════════════════════════════════════╗\r\n");
    LOG_I(MODULE_SHELL, "║                      Bootloader v%s                     ║\r\n", BOOT_VERSION);
    LOG_I(MODULE_SHELL, "║                STM32F407VE Development Board               ║\r\n");
    LOG_I(MODULE_SHELL, "╠════════════════════════════════════════════════════════════╣\r\n");
    LOG_I(MODULE_SHELL, "║  Compile Date: %-44s║\r\n", compile_date);
    LOG_I(MODULE_SHELL, "║  Compile Time: %-44s║\r\n", compile_time);
    LOG_I(MODULE_SHELL, "╚════════════════════════════════════════════════════════════╝\r\n");
    LOG_I(MODULE_SHELL, "  [INFO] Flash Base: 0x%08X, App Start: 0x%08X\r\n", FLASH_BASE, APP_FLASH_ADDR);
    LOG_I(MODULE_SHELL, "  [INFO] RAM Base:   0x%08X, Size: 192 KB\r\n", SRAM_BASE);
    LOG_I(MODULE_SHELL, "  [INFO] System Clock: 168 MHz\r\n");
    LOG_I(MODULE_SHELL, "  [READY] Waiting for commands...\r\n");
    LOG_I(MODULE_SHELL, "\r\n");
}
