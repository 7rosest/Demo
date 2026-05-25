#ifndef SCOM_H
#define SCOM_H

#include <stdint.h>
#include "semphr.h"

#define SCOM_HEADER           "SCOM"
#define SCOM_HEADER_LEN       4
#define SCOM_MAX_DATA_LEN     256
#define DMA_BLOCK_SIZE        64 

#define SCOM_FRAME_START 0xA5
#define SCOM_FRAME_END   0x5A

#define DMA_BUFFER_SIZE 64
#define NUM_BUFFERS     2

/* 通道号 */
typedef enum {
    SCOM_CHANNEL_CAMERA = 0,
    SCOM_CHANNEL_OTA = 1,
    SCOM_CHANNEL_STATUS = 2,
    SCOM_CHANNEL_MAX
} scom_channel_t;

/**
 * @brief SPI命令码定义
 */
typedef enum {
    CMD_PING            = 0x01,
    CMD_CAM_FRAME       = 0x10,
    CMD_CAM_CONFIG      = 0x11,
    CMD_ACK             = 0x80,
    CMD_NACK            = 0x81,
} scom_cmd_t;

/**
 * @brief SCOM帧头部结构
 */
typedef struct {
    uint8_t start;         // 帧头 A5
    uint8_t channel;       // 通道号
    uint8_t control;       // 帧类型
    uint16_t data_length;       // 数据长度
    uint16_t crc16;        // CRC16 校验值
    uint8_t *data;         // 数据指针
    uint8_t end;           // 帧尾 5A        
} scom_frame_t;

/* 环形缓冲区结构 */
typedef struct {
    uint8_t data[SCOM_MAX_DATA_LEN];
    uint8_t *read_point;
    uint8_t *write_point;
    uint8_t *end_point;
    SemaphoreHandle_t mutex;
} scom_buffer_t;

/* 通道结构 */
typedef struct {
    scom_buffer_t buffer;
    SemaphoreHandle_t semaphore;
    uint8_t enabled;
    const char *name;
} scom_channel_tbl_t;

extern uint8_t spi_rx_buffer[NUM_BUFFERS][DMA_BUFFER_SIZE];
extern SemaphoreHandle_t scom_parse_semaphore;
extern scom_buffer_t scom_buffer;
extern void recv_scom_from_isr(uint8_t flag, uint8_t len);
extern scom_channel_tbl_t scom_channels[SCOM_CHANNEL_MAX];

/* 函数声明 */
int scom_open(scom_channel_t channel);
int scom_read(scom_channel_t channel, uint8_t *data, size_t max_len);

#endif 