#ifndef FLASH_H
#define FLASH_H

#include "main.h"

typedef enum
{
    FLASH_OK = 0,         /* 操作成功 */
    FLASH_ERROR,          /* 通用错误 */
    FLASH_TIMEOUT,        /* 操作超时 */
    FLASH_WRITE_ERROR,    /* 写入错误 */
    FLASH_ERASE_ERROR,    /* 擦除错误 */
    FLASH_WRITE_MISMATCH, /* 写入不匹配 */
} flash_status_t;

#define INT_FLASH_BASE      0x08000000   // FLASH的起始地址
#define INT_FLASH_END       0x08080000   // FLASH的结束地址

#define FLASH_WAITETIME     500        //FLASH等待超时时间                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) 	//扇区0起始地址, 16 Kbytes   //BOOT
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) 	//扇区1起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) 	//扇区2起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) 	//扇区3起始地址, 16 Kbytes
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) 	//扇区4起始地址, 64 Kbytes   //参数区
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) 	//扇区5起始地址, 128 Kbytes  //APP
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) 	//扇区6起始地址, 128 Kbytes
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) 	//扇区7起始地址, 128 Kbytes

flash_status_t  flash_write(uint32_t writeAddress, uint8_t *data, uint32_t length);
flash_status_t  flash_erase(uint32_t addr);
flash_status_t flash_erase_xbin(uint32_t addr);
flash_status_t flash_read(uint32_t addr, uint8_t *data, uint32_t length);
uint8_t  flash_block_start_addr(uint32_t sectorAddr);

#endif 