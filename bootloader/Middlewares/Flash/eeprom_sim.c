#include "eeprom_sim.h"
#include "flash.h"
#include "../Log/log.h"
#include <stdint.h>
#include "global.h"

eeprom_dev_t eeprom = {0};
param_config_t config = {0};

static eeprom_status_t find_valid_block(eeprom_dev_t *dev);
static eeprom_status_t mark_block_invalid(eeprom_dev_t *dev, uint32_t block_idx);
static eeprom_status_t write_block(eeprom_dev_t *dev, uint32_t block_idx, const uint8_t *data, uint32_t data_len);
static eeprom_status_t read_block(eeprom_dev_t *dev, uint32_t block_idx, eeprom_block_t *block);
static uint32_t crc32(const uint8_t *data, uint32_t len);

/**
 * @brief 初始化EEPROM模拟设备
 * @param dev: EEPROM设备指针
 * @param sector_addr: Flash扇区起始地址
 * @return 状态码
 */
eeprom_status_t eeprom_init(eeprom_dev_t *dev, uint32_t sector_addr) {
    if (dev == NULL) {
        return EEPROM_ERR_NULL;
    }

    dev->sector_addr = sector_addr;
    dev->initialized = false;
    dev->current_block = 0;
    dev->valid_block = 0xFFFFFFFF;

    /* 查找有效块 */
    eeprom_status_t status = find_valid_block(dev);
    if (status != EEPROM_OK) {
        /* 如果没有有效块或查找失败，擦除扇区并初始化 */
        status = flash_erase(sector_addr);
        if (status != EEPROM_OK) {
            return EEPROM_ERR_INIT;
        }
        dev->current_block = 0;
        dev->valid_block = 0xFFFFFFFF;
        /*
        自定义参数清零（可选，根据实际需求决定是否需要）
        */
    } else {
        /* 有有效块，设置current_block为下一个块 */
        dev->current_block = (dev->valid_block + 1) % EEPROM_NUM_BLOCKS;
    }

    dev->initialized = true;
    return EEPROM_OK;
}

/**
 * @brief 查找有效的数据块
 */
static eeprom_status_t find_valid_block(eeprom_dev_t *dev) {
    eeprom_block_t block;
    
    /* 从最后一个块向前查找有效块 */
    for (int32_t i = EEPROM_NUM_BLOCKS - 1; i >= 0; i--) {
        eeprom_status_t status = read_block(dev, i, &block);
        if (status != EEPROM_OK) {
            continue;
        }
        
        /* 验证魔术数字 */
        if (block.magic_header != BLOCK_MAGIC_HEADER || block.magic_footer != BLOCK_MAGIC_FOOTER) {
            continue;
        }
        
        /* 验证块状态 */
        if (block.status != BLOCK_STATUS_VALID) {
            continue;
        }
        
        /* 验证数据长度 */
        if (block.data_len > EEPROM_MAX_DATA_SIZE) {
            continue;
        }
        
        /* 验证CRC */
        uint32_t crc = crc32(block.data, block.data_len);
        if (crc == block.crc32) {
            dev->valid_block = i;
            return EEPROM_OK;
        }
    }
    
    dev->valid_block = 0xFFFFFFFF;
    return EEPROM_ERR_INIT;
}

/**
 * @brief 标记块为无效
 */
static eeprom_status_t mark_block_invalid(eeprom_dev_t *dev, uint32_t block_idx) {
    if (block_idx >= EEPROM_NUM_BLOCKS) {
        return EEPROM_ERR_WRITE;
    }
    
    uint32_t addr = dev->sector_addr + block_idx * EEPROM_BLOCK_SIZE + offsetof(eeprom_block_t, status);
    
    /*
     * 由于Flash是只读写入，所以不能直接写入0xFFFFFFFF（无效块标识）
     * 所以，只能通过写入全0来标记块为无效（全0）
    */
    uint8_t invalid_status[4] = {0x00, 0x00, 0x00, 0x00};
    return flash_write(addr, invalid_status, sizeof(invalid_status));
}

/**
 * @brief 写入完整块
 */
static eeprom_status_t write_block(eeprom_dev_t *dev, uint32_t block_idx, const uint8_t *data, uint32_t data_len) {
    if (block_idx >= EEPROM_NUM_BLOCKS || data == NULL) {
        return EEPROM_ERR_WRITE;
    }
    
    uint32_t addr = dev->sector_addr + block_idx * EEPROM_BLOCK_SIZE;
    eeprom_block_t block;
    
    /* 初始化块内容为0xFF（Flash擦除后的值）*/
    for (uint32_t i = 0; i < sizeof(block); i++) {
        ((uint8_t *)&block)[i] = 0xFF;
    }
    
    /* 填充块头部 */
    block.magic_header = BLOCK_MAGIC_HEADER;
    block.status = BLOCK_STATUS_VALID;
    block.data_len = data_len;
    
    /* 填充数据区（复制传入的数据）*/
    for (uint32_t i = 0; i < data_len && i < sizeof(block.data); i++) {
        block.data[i] = data[i];
    }
    
    /* 计算CRC（仅针对实际数据）*/
    block.crc32 = crc32(data, data_len);
    
    /* 填充块尾部 */
    block.magic_footer = BLOCK_MAGIC_FOOTER;
    
    /* 写入整个块到Flash */
    return flash_write(addr, (uint8_t *)&block, sizeof(block));
}

/**
 * @brief 读取块数据
 */
static eeprom_status_t read_block(eeprom_dev_t *dev, uint32_t block_idx, eeprom_block_t *block) {
    if (block_idx >= EEPROM_NUM_BLOCKS || block == NULL) {
        return EEPROM_ERR_READ;
    }
    
    uint32_t addr = dev->sector_addr + block_idx * EEPROM_BLOCK_SIZE;
    
    /* 读取整个块 */
    return flash_read(addr, (uint8_t *)block, sizeof(*block));
}

/**
 * @brief 写入自定义结构体到EEPROM（简化接口）
 * @param dev: EEPROM设备指针
 * @param data: 要写入的数据指针（任意结构体）
 * @param data_size: 数据大小（结构体大小）
 * @return 状态码
 */ 
eeprom_status_t eeprom_write_data(eeprom_dev_t *dev, const void *data, uint32_t data_size) {
    /* 参数校验 */
    if (dev == NULL || data == NULL) {
        return EEPROM_ERR_NULL;
    }
    
    if (!dev->initialized) {
        return EEPROM_ERR_INIT;
    }
    
    /* 检查数据大小是否越界 */
    if (data_size == 0 || data_size > EEPROM_MAX_DATA_SIZE) {
        return EEPROM_ERR_SIZE;
    }
    
    eeprom_status_t status;
    
    /* 检查是否块已满（当前块索引等于有效块索引，表示所有块都已写过） */
    if (dev->current_block == 0 && dev->valid_block != 0xFFFFFFFF) {
        LOG_I(MODULE_SHELL, "block full!!!\r\n");
        /* 块已满，直接擦除整个扇区 */
        status = flash_erase(dev->sector_addr);
        if (status != EEPROM_OK) {
            return EEPROM_ERR_ERASE;
        }
        
        /* 重置块索引 */
        dev->current_block = 0;
        dev->valid_block = 0xFFFFFFFF;
    } 
    
    /* 如果有上一个有效块，标记为无效 */
    if (dev->valid_block != 0xFFFFFFFF) {
        status = mark_block_invalid(dev, dev->valid_block);
        if (status != EEPROM_OK) {
            return status;
        }
    }
    
    /* 写入新块（将结构体转换为字节数组写入）*/
    status = write_block(dev, dev->current_block, (const uint8_t *)data, data_size);
    if (status != EEPROM_OK) {
        return status;
    }
    
    /* 更新有效块和当前块索引 */
    dev->valid_block = dev->current_block;
    dev->current_block = (dev->current_block + 1) % EEPROM_NUM_BLOCKS;
    
    return EEPROM_OK;
}

/**
 * @brief 读取自定义结构体从EEPROM（简化接口）
 * @param dev: EEPROM设备指针
 * @param data: 数据接收缓冲区（任意结构体）
 * @param max_size: 缓冲区最大大小（结构体大小）
 * @return 状态码
 */
eeprom_status_t eeprom_read_data(eeprom_dev_t *dev, void *data, uint32_t max_size) {
    /* 参数校验 */
    if (dev == NULL || data == NULL) {
        return EEPROM_ERR_NULL;
    }
    
    if (!dev->initialized) {
        return EEPROM_ERR_INIT;
    }
    
    /* 检查缓冲区大小 */
    if (max_size == 0) {
        return EEPROM_ERR_SIZE;
    }
    
    /* 如果没有有效块 */
    if (dev->valid_block == 0xFFFFFFFF) {
        return EEPROM_ERR_READ;
    }
    
    eeprom_block_t block;
    
    /* 读取有效块 */
    eeprom_status_t status = read_block(dev, dev->valid_block, &block);
    if (status != EEPROM_OK) {
        return status;
    }
    
    /* 验证魔术数字 */
    if (block.magic_header != BLOCK_MAGIC_HEADER || block.magic_footer != BLOCK_MAGIC_FOOTER) {
        return EEPROM_ERR_MAGIC;
    }
    
    /* 验证数据长度 */
    if (block.data_len > EEPROM_MAX_DATA_SIZE) {
        return EEPROM_ERR_SIZE;
    }
    
    /* 验证CRC */
    uint32_t crc = crc32(block.data, block.data_len);
    if (crc != block.crc32) {
        return EEPROM_ERR_CRC;
    }
    
    /* 检查缓冲区是否足够大 */
    if (block.data_len > max_size) {
        return EEPROM_ERR_SIZE;
    }
    
    /* 将字节数据复制到用户结构体（内部自动处理类型转换）*/
    memcpy(data, block.data, block.data_len);
    
    return EEPROM_OK;
}

/**
 * @brief 擦除整个EEPROM扇区
 * @param dev: EEPROM设备指针
 * @return 状态码
 */
eeprom_status_t eeprom_erase(eeprom_dev_t *dev) {
    if (dev == NULL || !dev->initialized) {
        return (dev == NULL) ? EEPROM_ERR_NULL : EEPROM_ERR_INIT;
    }
    
    eeprom_status_t status = flash_erase(dev->sector_addr);
    if (status == EEPROM_OK) {
        dev->current_block = 0;
        dev->valid_block = 0xFFFFFFFF;
    }
    
    return status;
}

/**
 * @brief 计算CRC32校验值（位计算方式）
 * 使用标准CRC32多项式: 0xEDB88320
 */
static uint32_t crc32(const uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;
    
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

int eeprom_test(void) {
    param_config_t read_config = {0};

    eeprom_init(&eeprom, EEPROM_SECTOR_ADDR);

    eeprom_read_data(&eeprom, &read_config, sizeof(read_config));

    config.reboot_count = read_config.reboot_count + 1;
    
    if (eeprom_write_data(&eeprom, &config, sizeof(config)) != EEPROM_OK) {
        LOG_E(MODULE_SHELL, "EEPROM Write Failed\r\n");
        return -1;
    }

    LOG_I(MODULE_SHELL, "EEPROM Write Success: reboot_count=%u\r\n", config.reboot_count);

    return 0;
}