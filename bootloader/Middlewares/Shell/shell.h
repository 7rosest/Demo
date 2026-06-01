#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

/* command list */
typedef struct {
    const char cmdName[100];
    const void * cmdHandler;
    char cmdHelp[20];
} commandStruct;

/* shell cmd */
typedef struct{
    unsigned char used;
    unsigned char name[16];
    unsigned int  cmdNum;
    const commandStruct *cmdList;
} ShellStruct;

typedef struct {
    uint16_t recvPtr;
    uint8_t  recvPipe[256];  
} shell_buff_t;

/**
 * @brief SHELL事件定义
 * 
 * @note 这里定义了一个SHELL事件，表示有新的命令行输入需要处理
 */
#define TASK_SYS_EVENT_READ   (1<<17)

/**
 * @brief 最大参数数量定义
 * 
 * @note 参数数量最大为SHELL_PARAM_MAX - 1（因为argv[0]保留给命令名）
 */
#define SHELL_PARAM_MAX       (4)    

void shellPrintf(const char *fmt, ...);
void shell_uart_rx_callback(uint8_t rx_data);

#endif 