#include "flash.h"
#include <stdint.h>

extern void feed_dog(void);

 /**
 * @brief 判断是否为扇区首地址
 * 
 * @param SectorAddr 扇区地址
 * @note 扇区地址必须是16K的倍数，使用0,1,2,3扇区
 *
 * @return uint8_t 是否 0:不是 1:是
 */
uint8_t flash_block_start_addr(uint32_t SectorAddr) {
	if ((SectorAddr) % (16 * 1024)) {
		return 0;
	} else {
		return 1;
	}
}

// 辅助函数：根据地址获取扇区编号
uint8_t flash_get_sector(uint32_t addr) {
    if(addr < ADDR_FLASH_SECTOR_1) return FLASH_SECTOR_0;
    else if(addr < ADDR_FLASH_SECTOR_2) return FLASH_SECTOR_1;
    else if(addr < ADDR_FLASH_SECTOR_3) return FLASH_SECTOR_2;
    else if(addr < ADDR_FLASH_SECTOR_4) return FLASH_SECTOR_3;
    else if(addr < ADDR_FLASH_SECTOR_5) return FLASH_SECTOR_4;
    else if(addr < ADDR_FLASH_SECTOR_6) return FLASH_SECTOR_5;
    else if(addr < ADDR_FLASH_SECTOR_7) return FLASH_SECTOR_6;
    return FLASH_SECTOR_7;
}

/**
 * @brief 擦除指定地址的flash-适用于128k
 * 
 * @param addr 擦除地址
 * @return uint8_t 是否成功 
 */
flash_status_t flash_erase_xbin(uint32_t addr) {
    feed_dog();
	if (flash_block_start_addr(addr)) {
        HAL_FLASH_Unlock();

	    FLASH_EraseInitTypeDef flashEraseInit = {0};
	    uint32_t sectorError = 0;

        FLASH->SR |= FLASH_SR_PGSERR|FLASH_SR_PGPERR;             // 这两位是写入错误和擦除错误标志位，写1清除 

		flashEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除
		flashEraseInit.Sector = flash_get_sector(addr);           //要擦除的扇区
		flashEraseInit.NbSectors = 1;                             //一次只擦除一个扇区
		flashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
		if (HAL_FLASHEx_Erase(&flashEraseInit, &sectorError) != HAL_OK) {
            HAL_FLASH_Lock();
			return FLASH_ERASE_ERROR;
		}

        HAL_FLASH_Lock();
	}
    feed_dog();

	return FLASH_OK;
}

/**
 * @brief 擦除指定地址的flash
 * 
 * @param addr 擦除地址
 * @return uint8_t 是否成功 
 */
flash_status_t flash_erase(uint32_t addr) {
    feed_dog();
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef flashEraseInit = {0};
    uint32_t sectorError = 0;

    FLASH->SR |= FLASH_SR_PGSERR|FLASH_SR_PGPERR;             // 这两位是写入错误和擦除错误标志位，写1清除 

    flashEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除
    flashEraseInit.Sector = flash_get_sector(addr);           //要擦除的扇区
    flashEraseInit.NbSectors = 1;                             //一次只擦除一个扇区
    flashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
    if (HAL_FLASHEx_Erase(&flashEraseInit, &sectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        return FLASH_ERASE_ERROR;
    }

    HAL_FLASH_Lock();
    feed_dog();
	return FLASH_OK;
}

/**
 * @brief 写入数据到flash(外部按字节写入，内部按字写入)
 * 
 * @param writeAddress 写入地址
 * @param data 数据指针
 * @param length 数据长度
 * @return uint8_t 是否成功 
 */
flash_status_t flash_write(uint32_t writeAddress, uint8_t *data, uint32_t length) {
    HAL_StatusTypeDef flashStatus = HAL_OK;

    uint32_t endAddr = 0;
    uint32_t wordData = 0;  // 用于组装4字节数据

    if (writeAddress < INT_FLASH_BASE || writeAddress % 4) return 0;

    HAL_FLASH_Unlock();

    endAddr = writeAddress + length;  

    FLASH->SR |= FLASH_SR_PGSERR|FLASH_SR_PGPERR;/* 这两位是写入错误和擦除错误标志位，写1清除 */
    flashStatus = FLASH_WaitForLastOperation(FLASH_WAITETIME); 


    if (flashStatus == HAL_OK) {
        while (writeAddress < endAddr) {
            // 组装4字节数据（小端模式）
            wordData = 0;
            uint32_t bytesToWrite = 4;
             
             // 计算剩余字节数
            if ((writeAddress + 4) > endAddr) {
                bytesToWrite = endAddr - writeAddress;
            }
            
            for (uint32_t i = 0; i < bytesToWrite && (writeAddress + i) < endAddr; i++) {
                wordData |= ((uint32_t)data[i]) << (8 * i);
            }
            
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, writeAddress, wordData) != HAL_OK) {
                // break; 
                return FLASH_WRITE_ERROR;
            }

            uint32_t readData = *(uint32_t *)writeAddress; 
            if (readData != wordData) { 
                HAL_FLASH_Lock(); 
                return FLASH_WRITE_MISMATCH; 
            } 
            
            writeAddress += 4;
            data += 4;
        }
    } else {
        HAL_FLASH_Lock(); 
        return FLASH_TIMEOUT;
    }

    HAL_FLASH_Lock(); 
    return FLASH_OK;
}

/**
 * @brief 读取参数到Flash（内部按字节读取，对外按字节读取）
 * 
 * @return uint8_t 是否成功
 */
flash_status_t flash_read(uint32_t addr, uint8_t *data, uint32_t length) {
    if ((addr + length) > (INT_FLASH_END)) {
        return FLASH_ERROR;
    }

    // 逐字节读取（兼容性最好）
    for (uint32_t i = 0; i < length; i++) {
        data[i] = *(uint8_t *)(addr + i);
    }
    
    return FLASH_OK;
}