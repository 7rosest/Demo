#include "shell_list.h"
#include "../global.h"
#include "../Log/log.h"
#include "ring_buffer.h"
#include "string.h"
#include "xmode.h"
#include "main.h"

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
	const uint32_t FLASH_APP_ADDR = 0x8000000;
    
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

void software_reset(void) {
    __disable_irq();          
    __NVIC_SystemReset();     
}