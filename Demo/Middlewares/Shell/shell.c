#include "shell.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
#include "../global.h"
#include "../Log/log.h"
#include "usart.h"
#include "ring_buffer.h"
#include "shell_list.h"

static int mask;
ringbuffer_t dbg_rx_ring;
uint8_t recvBuff[2048];

/* shell list */
ShellStruct shellList[1];
shell_buff_t shellBuff;

void show_help(uint8_t cmdSource, uint32_t instance);

static const commandStruct commandList[] = {
    {"help",       (void *)show_help,         "help."},
    {"dbg",        (void *)dbg,               "dbg set."},
    {"xbin_boot",   (void *)xbin_boot,          "xbin boot."},
    {"reset",   (void *)software_reset,          "software reset."},
    {"cpu_info",   (void *)cpu_info,          "cpu info."},
};

extern int isr_cnt;
void show_help(uint8_t cmdSource, uint32_t instance) {
    unsigned int i,j;
    shellPrintf("Shell command List:num = %d\r\n",shellList[0].cmdNum);
    char buf[200];

    
    for (i = 0; i < sizeof(shellList)/sizeof(shellList[0]); i++) {
        if (shellList[i].used) {
            for(j = 0 ; j < shellList[0].cmdNum; j ++) {
                sprintf(buf,"%.30s - %s\r\n", commandList[j].cmdName, commandList[j].cmdHelp);//.11
                shellPrintf(buf);
            }
        }
    }
}

/**
 * @brief 打印调试信息
 * 
 * @param fmt 格式化字符串
 * @param ... 可变参数列表
 * 
 * @return void
 */
void shellPrintf(const char *fmt, ...) {
    if(g_xModemCommEnable) {
    	return;
    }

    va_list args;
    uint8_t *uart_out_data = NULL;
    uart_out_data = pvPortMalloc(sizeof(uint8_t) * 256);
    if(uart_out_data == NULL) {
        shellPrintf("malloc faild\r\n");
        return;
    }

    va_start(args, fmt);
    vsnprintf((char *)uart_out_data, 256, fmt, args); 
    HAL_UART_Transmit(&huart1, uart_out_data, strlen(uart_out_data), HAL_MAX_DELAY);
    vPortFree(uart_out_data);
    va_end(args);    
}

/**
 * @brief 添加一个shell命令列表到系统中
 * 
 * @param pShell 指向要添加的shell命令列表的指针
 * @return 0表示添加成功，1表示添加失败
 */
static unsigned int addShellList(ShellStruct *pShell) {
    unsigned int i;

    for (i = 0; i < sizeof(shellList)/sizeof(shellList[0]); i++) {
        if (shellList[i].used == 0) {
            shellList[i].used    = 1;
            shellList[i].cmdList = pShell->cmdList;
            shellList[i].cmdNum  = pShell->cmdNum;

            memcpy(shellList[i].name, pShell->name, sizeof( pShell->name ) - 1);
            return 0;
        }
    }

    return 1;
}

/**
 * @brief 运行shell命令
 * 
 * @param cmdSource 命令源
 * @param instance 实例 
 * @param strLine 命令字符串
 * @return 0表示命令执行成功，1表示命令执行失败
 */
static unsigned int runCmd( uint8_t cmdSource, uint32_t instance, unsigned char *strLine ) {
    int argc = 0, i = 0, j = 0;
    char argv[SHELL_PARAM_MAX][64];
    char *argvv[SHELL_PARAM_MAX];
    commandStruct *pCmdList;

    argv[0][0] = 0;
    for (i = 0; i < SHELL_PARAM_MAX; i++) {
        int res = sscanf((char const *)strLine, "%63s", &argv[i][0]);
        if (res == 1) { 
            argc++;
            strLine += strlen(argv[i]) + 1;
        } else {
            break; 
        }
    }

    for (i = 0; i < SHELL_PARAM_MAX - 1; i++) {
        /**
         * argv参数是从argv[1]开始的，因为argv[0]保留给命令名，所以需要将参数向前移动一位
         * 并且argvv[argc]指向argv[0]，以便命令处理
         */
        argvv[i] = argv[i + 1];
    }

    argvv[argc] = argv[0]; //command name

    for (i = 0; i < sizeof(shellList)/sizeof(shellList[0]); i++) {
        if (shellList[i].used) {
            pCmdList = shellList[i].cmdList;

            for (j = 0 ; j < shellList[i].cmdNum; j ++ ) {
                if (strcmp((char const *)argv[0], pCmdList[j].cmdName) == 0) {
                    ((void(*)(uint8_t,uint32_t,unsigned int, unsigned char**))(unsigned int)pCmdList[j].cmdHandler)
                        ( cmdSource,instance,argc - 1, (unsigned char**)argvv );
                    return 0;
                }
            }
        }
    }

    return 1;
}

/**
 * @brief 运行shell命令行
 * 
 * @param cmdSource 命令源
 * @param instance 实例
 * @param strLine 命令行字符串
 * @return 0表示命令行执行成功，1表示命令行执行失败
 */
static unsigned int runShellCMD( uint8_t cmdSource,uint32_t instance,unsigned char *strLine ) {
    unsigned int i = 0, j = 0,len;
    len = strlen( (char const*)strLine );
    for (i = 0; i < len ; i++) {
        if ((strLine[i] == '\r') && (strLine[i+1] == '\n')) {
            strLine[i] = '\0';
            if (runCmd( cmdSource,instance,strLine + j)) {
                shellPrintf("Unknown shell cmd: %s\r\n", (strLine+j));
                show_help(cmdSource,instance);
                return 1;
            }
            j = i + 2;
        }
    }

    if ((i == len) && ((j + 1) <= len)) {
        if (runCmd( cmdSource,instance,strLine + j)) {
            show_help(cmdSource,instance);
            return 1;
        }
    }

    return 0;
}

/**
 * @brief 初始化shell模块
 * 
 * @param 无
 * @return 无   
 */
static void ShellInit(void) {
    ShellStruct uShell = {   
        1,              /* used */             
        "user shell",   /* name */               
        0,              /* cmdNum */              
        0               /* cmdlist */
    };
    uShell.used = 1;
    uShell.cmdNum = sizeof(commandList) / sizeof(commandList[0]);
    uShell.cmdList = commandList;
    addShellList(&uShell);

    return;
}

/* set task event (used in ISR) */
static void kernelSetTaskEvent(unsigned int event) {
    mask |= event;

    return;
}

static void runcmd(void) {
    runShellCMD(1, 0, (unsigned char*)shellBuff.recvPipe);
    memset((void*)shellBuff.recvPipe, 0, 256);
}

static void checkcmd(void) {
    if (mask & TASK_SYS_EVENT_READ) {
        mask &= ( ~ TASK_SYS_EVENT_READ );
        runcmd();
    }
}

/**
 * @brief SHELL UART接收回调函数
 * @param rx_data 接收到的字节数据
 * 
 * @return void
 */
void shell_uart_rx_callback(uint8_t rx_data) {
    uint8_t tempchar = rx_data;
    static uint8_t lastchar;

    if (shellBuff.recvPtr>=256) {
        shellBuff.recvPtr = 0;
    }
    shellBuff.recvPipe[shellBuff.recvPtr++] = tempchar;

    if (lastchar == '\r' && tempchar == '\n') {
        shellBuff.recvPipe[shellBuff.recvPtr++] = '\0';
        shellBuff.recvPtr = 0;
        kernelSetTaskEvent(TASK_SYS_EVENT_READ);
    }
    lastchar = tempchar;
}

void RingbufferInit(void) {
  create_ringBuffer(&dbg_rx_ring, recvBuff, 2048);
}

static int shell_init(void) {
    ShellInit();
    RingbufferInit();

    return 0;
}
MODULE_INIT(shell_init, "SHELL");

extern int eeprom_test(void);
static void Shell_Task(void *pvParameters) {   
    LOG_I(MODULE_SYS, "Shell Task Started\r\n");
    eeprom_test();

    while (1) {
        checkcmd();

        vTaskDelay(10UL);
    }
}
MODULE_TASK(Shell_Task, "SHELL_Task", 256 * 3, 1);
