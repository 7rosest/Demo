#include "global.h"
#include "spi.h"
#include "scom.h"
#include "../Task/k_task.h"
#include "../Log/log.h"
#include "string.h"
#include <stdint.h>

static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/**
 * @brief 计算CRC16校验值
 * @param data 输入数据指针
 * @param len 数据长度
 * @return uint16_t CRC16校验值
 * */
uint16_t scom_crc16(const uint8_t *data, uint32_t len) {
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc = crc16_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

/**
 * @brief 发送SPI响应
 * @param response 响应字节
 * */
void scom_send(uint8_t response) {
    HAL_SPI_Transmit(&hspi1, &response, 1, HAL_MAX_DELAY);
}
/////////////////////////////////////////////////////////////////////////////
/* 通道配置 */
scom_channel_tbl_t scom_channels[SCOM_CHANNEL_MAX] = {
    {.name = "camera"},
    {.name = "ota"}, 
    {.name = "status"}
};

/* 主缓冲区 */
scom_buffer_t scom_buffer;
SemaphoreHandle_t scom_parse_semaphore;
uint8_t spi_rx_buffer[NUM_BUFFERS][DMA_BUFFER_SIZE];

/* 初始化缓冲区 */
static void scom_buffer_init(scom_buffer_t *buff) {
    buff->read_point = buff->data;
    buff->write_point = buff->data;
    buff->end_point = buff->data + SCOM_MAX_DATA_LEN;
    buff->mutex = xSemaphoreCreateMutex();
}

/* 写入缓冲区 */
static void scom_buffer_write(scom_buffer_t *buff, uint8_t *input, size_t count) {
    xSemaphoreTake(buff->mutex, portMAX_DELAY);
    
    for (size_t i = 0; i < count; i++) {
        *buff->write_point++ = input[i];
        if (buff->write_point >= buff->end_point) {
            buff->write_point = buff->data;
        }
        
        if (buff->write_point == buff->read_point) {
            buff->read_point++;
            if (buff->read_point >= buff->end_point) {
                buff->read_point = buff->data;
            }
        }
    }
    
    xSemaphoreGive(buff->mutex);
}

/* 从缓冲区读取字节 */
static int scom_buffer_read_byte(scom_buffer_t *buff, uint8_t *byte) {
    xSemaphoreTake(buff->mutex, portMAX_DELAY);
    
    if (buff->read_point == buff->write_point) {
        xSemaphoreGive(buff->mutex);
        return -1;
    }
    
    *byte = *buff->read_point++;
    if (buff->read_point >= buff->end_point) {
        buff->read_point = buff->data;
    }
    
    xSemaphoreGive(buff->mutex);
    return 0;
}

/* 扫描帧头 A5 */
static int scom_scan_frame_start(scom_buffer_t *buff) {
    uint8_t byte;
    
    while (1) {
        if (scom_buffer_read_byte(buff, &byte) != 0) {
            return -1;  // 缓冲区空
        }
        
        if (byte == SCOM_FRAME_START) {
            return 0;  // 找到帧头
        }
        
        // 不是帧头，继续扫描
        LOG_I(MODULE_SCOM, "[SCOM] 丢弃无效字节: 0x%02X\n", byte);
    }
}

/* 扫描帧尾 5A */
static int scom_scan_frame_end(scom_buffer_t *buff) {
    uint8_t byte;
    
    while (1) {
        if (scom_buffer_read_byte(buff, &byte) != 0) {
            return -1;  // 缓冲区空
        }
        
        if (byte == SCOM_FRAME_END) {
            return 0;  // 找到帧尾
        }
        
        // 不是帧尾，继续扫描
        LOG_I(MODULE_SCOM,"[SCOM] 帧尾错误，丢弃字节: 0x%02X\n", byte);
    }
}

/* 解析单帧（带帧头帧尾和 CRC16） */
static int scom_parse_frame(scom_buffer_t *buff, scom_frame_t *frame) {
    uint8_t byte;
    uint16_t data_len;
    uint16_t received_crc;
    
    /* ========== 步骤1: 扫描帧头 A5 ========== */
    if (scom_scan_frame_start(buff) != 0) {
        return -1;
    }
    
    frame->start = SCOM_FRAME_START;
    
    /* ========== 步骤2: 解析帧内容 ========== */
    /* 读取通道号 */
    if (scom_buffer_read_byte(buff, &byte) != 0) return -1;
    frame->channel = byte;
    
    /* 读取控制字段 */
    if (scom_buffer_read_byte(buff, &byte) != 0) return -1;
    frame->control = byte;
    
    /* 读取数据长度（大端序） */
    if (scom_buffer_read_byte(buff, &byte) != 0) return -1;
    data_len = byte << 8;
    if (scom_buffer_read_byte(buff, &byte) != 0) return -1;
    data_len |= byte;
    
    frame->data_length = data_len;
    
    /* 读取 CRC16（大端序） */
    if (scom_buffer_read_byte(buff, &byte) != 0) return -1;
    received_crc = byte << 8;
    if (scom_buffer_read_byte(buff, &byte) != 0) return -1;
    received_crc |= byte;
    
    frame->crc16 = received_crc;
    
    /* 读取数据 */
    uint8_t *tmp_data = (uint8_t*)pvPortMalloc(data_len);
    if (!tmp_data) return -1;
    
    for (int i = 0; i < data_len; i++) {
        if (scom_buffer_read_byte(buff, &byte) != 0) {
            vPortFree(tmp_data);
            return -1;
        }
        tmp_data[i] = byte;
    }
    
    frame->data = tmp_data;
    
    /* ========== 步骤3: CRC16 校验 ========== */
    uint16_t calculated_crc = scom_crc16(frame->data, data_len);
    if (calculated_crc != received_crc) {
        vPortFree(frame->data);
        LOG_I(MODULE_SCOM,"[SCOM] CRC 校验失败! 期望: 0x%04X, 实际: 0x%04X\n", calculated_crc, received_crc);
        return -1;
    }
    
    /* ========== 步骤4: 扫描帧尾 5A ========== */
    if (scom_scan_frame_end(buff) != 0) {
        vPortFree(frame->data);
        return -1;
    }
    
    frame->end = SCOM_FRAME_END;
    
    return 0;
}

/* 压入通道 */
static void scom_frame_push(scom_channel_t channel, scom_frame_t *frame) {
    if (channel >= SCOM_CHANNEL_MAX || !scom_channels[channel].enabled) {
        vPortFree(frame->data);
        return;
    }
    
    scom_buffer_write(&scom_channels[channel].buffer, frame->data, frame->data_length);
    vPortFree(frame->data);
    
    xSemaphoreGive(scom_channels[channel].semaphore);
}

/* 处理主缓冲区 */
static void scom_process_buffer(void) {
    scom_frame_t frame;
    
    while (1) {
        /* 解析帧（带帧头帧尾） */
        int ret = scom_parse_frame(&scom_buffer, &frame);
        
        if (ret == 0) {
            /* 解析成功，压入通道 */
            scom_frame_push(frame.channel, &frame);
            LOG_I(MODULE_SCOM,"[SCOM] 帧解析成功: 帧头=0x%02X, 通道=%d, 长度=%d, CRC=0x%04X, 帧尾=0x%02X\n", 
                   frame.start, frame.channel, frame.data_length, frame.crc16, frame.end);
        } else {
            /* 解析失败或数据不足 */
            break;
        }
    }
}

/* 打开通道 */
int scom_open(scom_channel_t channel) {
    if (channel >= SCOM_CHANNEL_MAX) return -1;
    
    if (!scom_channels[channel].semaphore) {
        scom_buffer_init(&scom_channels[channel].buffer);
        scom_channels[channel].semaphore = xSemaphoreCreateBinary();
    }
    
    scom_channels[channel].enabled = 1;
    LOG_I(MODULE_SCOM,"[SCOM] 打开通道: %s\n", scom_channels[channel].name);
    return 0;
}

/* 从通道读取数据 */
int scom_read(scom_channel_t channel, uint8_t *data, size_t max_len) {
    if (channel >= SCOM_CHANNEL_MAX || !scom_channels[channel].enabled) {
        return -1;
    }
    
    xSemaphoreTake(scom_channels[channel].semaphore, portMAX_DELAY);
    
    size_t count = 0;
    while (count < max_len) {
        uint8_t byte;
        if (scom_buffer_read_byte(&scom_channels[channel].buffer, &byte) != 0) {
            break;
        }
        data[count++] = byte;
    }
    
    return count;
}

void recv_scom_from_isr(uint8_t flag, uint8_t len) {
    scom_buffer_write(&scom_buffer, spi_rx_buffer[flag], len);
}

/**
 * @brief 初始化SCOM模块
 * 
 * @return int 返回值
 */
static int scom_init(void)
{
    scom_buffer_init(&scom_buffer);
    scom_parse_semaphore = xSemaphoreCreateBinary();

    HAL_SPI_Receive_DMA(&hspi1, spi_rx_buffer[0], DMA_BUFFER_SIZE * NUM_BUFFERS);

    return 0;
}
MODULE_INIT(scom_init, "SCOM");

/**
 * @brief SCOM任务函数
 * 
 * @param pvParameters 参数
 */
static void scom_task(void *pvParameters) {
    LOG_I(MODULE_SYS, "SCOM Task Start.\r\n");
    
    while (1) {
        xSemaphoreTake(scom_parse_semaphore, portMAX_DELAY);
        scom_process_buffer();

        vTaskDelay(5); 
    }
}
MODULE_TASK(scom_task, "SCOM_Task", configMINIMAL_STACK_SIZE * 2, 1);
