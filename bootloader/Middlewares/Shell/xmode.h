#ifndef XMODEM_H
#define XMODEM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief XMODEM 协议控制字符定义
 */
#define XMODEM_SOH         0x01    /* 开始头（128字节数据包）*/
// #define XMODEM_STX         0x02    /* 开始头（1024字节数据包）*/
#define XMODEM_EOT         0x04    /* 传输结束 */
#define XMODEM_ACK         0x06    /* 确认 */
#define XMODEM_NAK         0x15    /* 否定确认 */
#define XMODEM_CAN         0x18    /* 取消确认（CAN） */
#define XMODEM_C           0x43    /* ASCII 'C'，请求使用CRC模式 */

/**
 * @brief XMODEM 配置参数
 */
#define XMODEM_PACKET_SIZE    128     /* 数据包大小 */
#define XMODEM_PACKET_SIZE_1K 1024    /* 1K 数据包大小 */
#define XMODEM_TIMEOUT_MS     1000    /* 超时时间（毫秒）*/
#define XMODEM_BYTE_TIMEOUT   100     /* 字节超时时间（毫秒）*/
#define XMODEM_MAX_RETRY      30      /* 最大重传次数 */
#define XMODEM_MAX_HANDSHAKE  60      /* 最大握手次数 */

/**
 * @brief XMODEM 传输状态枚举
 */
typedef enum {
    XMODEM_STATE_IDLE = 0,       /* 空闲状态 */
    XMODEM_STATE_HANDSHAKE,      /* 握手阶段 */
    XMODEM_STATE_RECEIVING,      /* 接收数据阶段 */
    XMODEM_STATE_COMPLETE,       /* 传输完成 */
    XMODEM_STATE_ERROR           /* 传输错误 */
} xmodem_state_t;

/**
 * @brief XMODEM 错误码枚举
 */
typedef enum {
    XMODEM_OK = 0,               /* 成功 */
    XMODEM_ERR_TIMEOUT,          /* 超时错误 */
    XMODEM_ERR_CRC,              /* CRC校验错误 */
    XMODEM_ERR_SEQ,              /* 序列号错误 */
    XMODEM_ERR_FLASH_ERASE,      /* Flash擦除错误 */
    XMODEM_ERR_FLASH_WRITE,      /* Flash写入错误 */
    XMODEM_ERR_HANDSHAKE,        /* 握手失败 */
    XMODEM_ERR_RETRY_EXCEED      /* 重传次数超限 */
} xmodem_error_t;

/**
 * @brief XMODEM 传输上下文结构体
 */ 
typedef struct {
#ifdef XMODEM_STX
    uint8_t         buffer[1029];     /* 数据包缓冲区 */   //1029
#else
    uint8_t         buffer[133];     /* 数据包缓冲区 */   //133  
#endif
    uint8_t         expected_seq;    /* 期望的数据包序列号 */
    uint32_t        timeout_start;   /* 超时计时起始时间 */
    xmodem_state_t  state;           /* 当前传输状态 */
    xmodem_error_t  error;           /* 错误码 */
    uint32_t        flash_addr;      /* Flash写入地址 */
    uint32_t        bytes_received;  /* 已接收字节数 */
    uint8_t         retry_count;     /* 重传计数器 */
    uint8_t         handshake_count; /* 握手计数器 */
    bool            crc_enabled;     /* CRC模式标志 */
} xmodem_context_t;

void xmodem_init(xmodem_context_t *ctx, uint32_t flash_addr);
xmodem_error_t xmodem_run(xmodem_context_t *ctx);
xmodem_state_t xmodem_get_state(const xmodem_context_t *ctx);
uint32_t xmodem_get_bytes_received(const xmodem_context_t *ctx);

#endif 