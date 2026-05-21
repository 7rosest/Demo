#ifndef EEPROM_SIM_H
#define EEPROM_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define EEPROM_SECTOR_ADDR  0x08010000

/* 配置参数 */
#define EEPROM_SECTOR_SIZE      65536UL    /* 扇区大小 64KB */
#define EEPROM_BLOCK_SIZE       64UL      /* 块大小 64字节 */
#define EEPROM_NUM_BLOCKS       (EEPROM_SECTOR_SIZE / EEPROM_BLOCK_SIZE)  /* 1024个块 */

/* 魔术数字定义 */
#define BLOCK_MAGIC_HEADER      0x5AA55AA5  /* 头部魔术数字 */
#define BLOCK_MAGIC_FOOTER      0xA55AA55A  /* 尾部魔术数字 */

/* 块状态定义 */
#define BLOCK_STATUS_INVALID    0xFFFFFFFF  /* 无效块标识 */   /* 无效（全0） */
#define BLOCK_STATUS_VALID      0xAAAAAAAA  /* 有效块标识 */

/* 完整的块结构体（512字节）*/
typedef struct {
    /* 块头部 */
    uint32_t magic_header;    /* 头部魔术数字: BLOCK_MAGIC_HEADER */
    uint32_t status;         /* 块状态: BLOCK_STATUS_VALID 或 BLOCK_STATUS_INVALID */
    uint32_t crc32;          /* 数据CRC32校验值 */
    uint32_t data_len;       /* 实际数据长度 */
    
    /* 数据区 */
    uint8_t  data[EEPROM_BLOCK_SIZE - sizeof(uint32_t) * 5];  /* 数据区域 = 512 - 20 = 492字节 */
    
    /* 块尾部 */
    uint32_t magic_footer;    /* 尾部魔术数字: BLOCK_MAGIC_FOOTER */
} eeprom_block_t;

/* 计算最大数据长度 */
#define EEPROM_MAX_DATA_SIZE    sizeof(((eeprom_block_t *)0)->data)

/* EEPROM状态枚举 */
typedef enum {
    EEPROM_OK = 0,
    EEPROM_ERR_INIT,
    EEPROM_ERR_WRITE,
    EEPROM_ERR_READ,
    EEPROM_ERR_ERASE,
    EEPROM_ERR_CRC,
    EEPROM_ERR_MAGIC,
    EEPROM_ERR_SIZE,          /* 数据大小超过限制 */
    EEPROM_ERR_NULL           /* 空指针错误 */
} eeprom_status_t;

/* EEPROM设备结构 */
typedef struct {
    uint32_t sector_addr;    /* 扇区起始地址 */
    uint32_t current_block;  /* 当前写入块索引 */
    uint32_t valid_block;    /* 有效块索引 (0xFFFFFFFF表示无有效块) */
    bool     initialized;    /* 是否已初始化 */
} eeprom_dev_t;

/* 函数声明 */

/**
 * @brief 初始化EEPROM模拟设备
 * @param dev: EEPROM设备指针
 * @param sector_addr: Flash扇区起始地址
 * @return 状态码
 */
eeprom_status_t eeprom_init(eeprom_dev_t *dev, uint32_t sector_addr);

/**
 * @brief 写入自定义结构体到EEPROM（简化接口）
 * @param dev: EEPROM设备指针
 * @param data: 要写入的数据指针（任意结构体）
 * @param data_size: 数据大小（结构体大小）
 * @return 状态码
 */
eeprom_status_t eeprom_write_data(eeprom_dev_t *dev, const void *data, uint32_t data_size);

/**
 * @brief 读取自定义结构体从EEPROM（简化接口）
 * @param dev: EEPROM设备指针
 * @param data: 数据接收缓冲区（任意结构体）
 * @param max_size: 缓冲区最大大小（结构体大小）
 * @return 状态码
 */
eeprom_status_t eeprom_read_data(eeprom_dev_t *dev, void *data, uint32_t max_size);

/**
 * @brief 擦除整个EEPROM扇区
 * @param dev: EEPROM设备指针
 * @return 状态码
 */
eeprom_status_t eeprom_erase(eeprom_dev_t *dev);

/**
 * @brief 获取EEPROM最大可存储数据大小
 * @return 最大数据大小（字节）
 */
static inline uint32_t eeprom_get_max_size(void) {
    return EEPROM_MAX_DATA_SIZE;
}

#endif /* EEPROM_SIM_H */