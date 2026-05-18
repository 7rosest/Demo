#include "shell_list.h"
#include "../global.h"
#include "../Log/log.h"
#include "../../Core/inc/usart.h"
#include "ring_buffer.h"
#include "string.h"
#include "xmode.h"
#include "../Flash/flash.h"

extern uint32_t xbin_uart_recv(uint8_t *buffer, uint32_t size);

/**
 * @brief 计算 CRC16 校验值
 * 
 * @param data 数据指针
 * @param length 数据长度
 * @return uint16_t CRC16 值
 */
static uint16_t xmodem_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0x0000;

    while (length--) {
        crc ^= ((uint16_t)*data++) << 8;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0x0000);
        }
    }
    return crc;
}

/**
 * @brief 获取系统运行时间
 * @return uint32_t 系统运行时间（毫秒）
 */
uint32_t driverGetUptime()
{
	return xTaskGetTickCount();
}

/**
 * @brief 发送单个字节
 * 
 * @param byte 要发送的字节
 */
static void xmodem_send_byte(uint8_t byte)
{
    uint8_t buf[1] = {byte};
    dbg_uart_send(buf, 1);
}

/**
 * @brief 接收单个字节（带超时）
 * 
 * @param byte 接收字节的指针
 * @param timeout_ms 超时时间（毫秒）
 * @return bool 成功返回true，超时返回false
 */
static bool xmodem_recv_byte(uint8_t *byte, uint32_t timeout_ms)
{
    uint32_t start = driverGetUptime();
    while (driverGetUptime() < start + timeout_ms) {
        if (xbin_uart_recv(byte, 1)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 发送握手请求（'C'字符）
 */
static void xmodem_send_handshake(void)
{
    xmodem_send_byte(XMODEM_C);
}

/**
 * @brief 发送确认（ACK）
 */
static void xmodem_send_ack(void)
{
    xmodem_send_byte(XMODEM_ACK);
}

/**
 * @brief 发送否定确认（NAK）
 */
static void xmodem_send_nak(void)
{
    xmodem_send_byte(XMODEM_NAK);
}

/**
 * @brief 发送取消确认（CAN）
 */
static void xmodem_send_can(void)
{
    xmodem_send_byte(XMODEM_CAN);
}

/**
 * @brief 初始化XMODEM上下文
 * @param ctx XMODEM上下文指针
 * @param flash_addr Flash地址
 */
void xmodem_init(xmodem_context_t *ctx, uint32_t flash_addr)
{
    memset(ctx, 0, sizeof(xmodem_context_t)); 

    ctx->expected_seq = 1;
    ctx->state = XMODEM_STATE_HANDSHAKE;
    ctx->error = XMODEM_OK;
    ctx->flash_addr = flash_addr;
    ctx->crc_enabled = true;
}

/**
 * @brief 运行XMODEM协议
 * @param ctx XMODEM上下文指针
 * @return XMODEM错误码
 */
xmodem_error_t xmodem_run(xmodem_context_t *ctx)
{
    uint8_t byte;
    uint16_t packet_size;

    while(1) {
        switch (ctx->state) {
            case XMODEM_STATE_HANDSHAKE:
                xmodem_send_handshake();
                ctx->timeout_start = driverGetUptime();
                ctx->handshake_count = 0;
                
                while (ctx->state == XMODEM_STATE_HANDSHAKE) {
                    if (xmodem_recv_byte(&byte, XMODEM_BYTE_TIMEOUT)) {
                        if ((byte == XMODEM_SOH) || (byte == XMODEM_STX)) {
                            ctx->buffer[0] = byte;
                            packet_size = (byte == XMODEM_SOH) ? XMODEM_PACKET_SIZE : XMODEM_PACKET_SIZE_1K;
                            ctx->state = XMODEM_STATE_RECEIVING;
                            ctx->timeout_start = driverGetUptime();
                            break;
                        }
                    }
                    
                    if (driverGetUptime() > ctx->timeout_start + XMODEM_TIMEOUT_MS) {
                        ctx->handshake_count++;
                        if (ctx->handshake_count >= XMODEM_MAX_HANDSHAKE) {
                            ctx->state = XMODEM_STATE_ERROR;
                            ctx->error = XMODEM_ERR_HANDSHAKE;
                            return ctx->error;
                        }
                        xmodem_send_handshake();
                        ctx->timeout_start = driverGetUptime();
                    }
                }
                continue; 

            case XMODEM_STATE_RECEIVING: {
                uint16_t offset = 1;
                packet_size = (ctx->buffer[0] == XMODEM_SOH) ? XMODEM_PACKET_SIZE : XMODEM_PACKET_SIZE_1K;
                uint16_t total_len = 5 + packet_size; 

                while (offset < total_len) { // 133 = 1(SOH) + 1(SEQ) + 1(SEQ_CMP) + 128(DATA) + 2(CRC)
                    if (xmodem_recv_byte(ctx->buffer + offset, XMODEM_BYTE_TIMEOUT)) {
                        offset++;
                    } else {
                        ctx->error = XMODEM_ERR_TIMEOUT;
                        break;
                    }
                }

                // if (ctx->error != XMODEM_OK) {
                //     ctx->state = XMODEM_STATE_ERROR;
                //     // xmodem_send_can();
                //     // return ctx->error;
                // }

                if (ctx->buffer[1] != ctx->expected_seq || 
                    ctx->buffer[2] != (uint8_t)(0xFF - ctx->buffer[1])) {
                    ctx->error = XMODEM_ERR_SEQ;
                    // ctx->state = XMODEM_STATE_ERROR;
                    // xmodem_send_can();
                    // return ctx->error;
                }

                if (xmodem_crc16(&ctx->buffer[3], packet_size) != 
                    ((ctx->buffer[total_len - 2] << 8) | ctx->buffer[total_len - 1])) {
                    ctx->error = XMODEM_ERR_CRC;
                    // ctx->state = XMODEM_STATE_ERROR;
                    // xmodem_send_can();
                    // return ctx->error;
                }

                // 如果出现错误，尝试重试
                // if (ctx->error != XMODEM_OK) { 
                //     ctx->retry_count++; 
                //     if (ctx->retry_count < XMODEM_MAX_RETRY) { 
                //         xmodem_send_nak();  // 发送 NAK 请求重传
                //         ctx->timeout_start = driverGetUptime(); 
                //         break;
                //     } 

                //     ctx->error = XMODEM_ERR_RETRY_EXCEED;
                //     ctx->state = XMODEM_STATE_ERROR; 
                //     xmodem_send_can(); 
                //     return ctx->error; 
                // } 

                ctx->retry_count = 0;
                
                if (flash_erase_xbin(ctx->flash_addr) != FLASH_OK) {
                        ctx->error = XMODEM_ERR_FLASH_ERASE;
                        ctx->state = XMODEM_STATE_ERROR;
                        xmodem_send_can();
                        return ctx->error;
                }
                
                if (flash_write(ctx->flash_addr, ctx->buffer + 3, packet_size) != FLASH_OK) {
                    ctx->error = XMODEM_ERR_FLASH_WRITE;
                    ctx->state = XMODEM_STATE_ERROR;
                    xmodem_send_can();
                    return ctx->error;
                }
                
                ctx->flash_addr += packet_size;
                ctx->bytes_received += packet_size;
                ctx->expected_seq++;
                xmodem_send_ack();
                
                /* 继续接收下一包或检查结束 */
                if (xmodem_recv_byte(&byte, XMODEM_TIMEOUT_MS)) {
                    if (byte == XMODEM_EOT) {
                        xmodem_send_ack();
                        ctx->state = XMODEM_STATE_COMPLETE;
                    } else if (byte == XMODEM_SOH || byte == XMODEM_STX) {
                        ctx->buffer[0] = byte;
                    } else {
                        ctx->error = XMODEM_ERR_SEQ;
                        ctx->state = XMODEM_STATE_ERROR;
                    }
                } else {
                    ctx->error = XMODEM_ERR_TIMEOUT;
                    ctx->state = XMODEM_STATE_ERROR;
                }
                break;
            }

            case XMODEM_STATE_COMPLETE:
                return XMODEM_OK;

            case XMODEM_STATE_ERROR:
                xmodem_send_can();
                return ctx->error;

            default:
                xmodem_send_can();
                return ctx->error;
                break;
        }
    }
    
    return XMODEM_OK;
}

/**
 * @brief 获取XMODEM状态
 * @param ctx XMODEM上下文指针
 * @return XMODEM状态
 */
xmodem_state_t xmodem_get_state(const xmodem_context_t *ctx)
{
    return ctx->state;
}

/**
 * @brief 获取XMODEM接收的字节数
 * @param ctx XMODEM上下文指针
 * @return 接收的字节数
 */
uint32_t xmodem_get_bytes_received(const xmodem_context_t *ctx)
{
    return ctx->bytes_received;
}