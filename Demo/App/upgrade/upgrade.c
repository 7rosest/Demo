#include "json_public.h"
#include "../Middlewares/global.h"
#include "semphr.h"
#include <string.h>

#define mutex_lock(mutex)     xSemaphoreTake((mutex), portMAX_DELAY)
#define mutex_unlock(mutex)   xSemaphoreGive((mutex))
#define mutex_create()        xSemaphoreCreateMutex()

#define OTA_MAX_SIZE        (1024 * 384)    // 最大OTA包大小 384KB
#define OTA_CHUNK_SIZE      1024             // 每包数据大小

typedef enum {
    OTA_IDLE,           // 空闲状态
    OTA_RECEIVING,      // 接收中
    OTA_COMPLETE,       // 接收完成
    OTA_ERROR           // 错误状态
} OTA_State_t;

typedef struct {
    uint8_t     chunkType;       // 包类型
    char        version[16];     // 版本号
    uint32_t    bagSize;         // 总包大小
    uint8_t     bagType;         // 包类型
    uint32_t    sequence;        // 当前包序号
    uint32_t    receivedSize;    // 已接收大小
    uint32_t    flashOffset;     // Flash写入偏移
    OTA_State_t state;           // 当前状态
} OTA_Info_t;

static const uint8_t base64_decode_table[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF,
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static OTA_Info_t ota_info = {0};
static SemaphoreHandle_t ota_mutex = NULL;

static size_t base64_decode(const char* encoded, uint8_t* decoded, size_t max_len);
static bool write_to_flash(const uint8_t* data, size_t len, uint32_t offset);
static bool erase_flash_for_ota(void);

/**
 * @brief 初始化 OTA 模块
 * 
 * @param 无
 * @return 无   
 */
void ota_init(void) {
    /* 初始化 OTA 信息 */
    ota_info.state = OTA_IDLE;
    ota_info.receivedSize = 0;
    ota_info.flashOffset = 0;
    ota_info.bagSize = 0;
    ota_info.sequence = 0;
    ota_info.chunkType = 0;
    ota_info.bagType = 0;
    memset(ota_info.version, 0, sizeof(ota_info.version));
    
    /* 创建 mutex */
    ota_mutex = mutex_create();
    if (ota_mutex == NULL) {
        LOG_E(MODULE_OTA, "Failed to create OTA mutex!\r\n");
        return;
    }
}

/**
 * @brief Base64 解码函数
 * 
 * @param encoded Base64 编码字符串指针
 * @param decoded 解码后数据缓冲区指针
 * @return size_t 解码后数据长度
 */
static size_t base64_decode(const char* encoded, uint8_t* decoded, size_t max_len) {
    size_t len = 0;
    uint32_t temp = 0;
    int bits = 0;
    
    if (encoded == NULL || decoded == NULL || max_len == 0) {
        return 0;
    }
    
    while (*encoded != '\0' && len < max_len) {
        uint8_t c = (uint8_t)*encoded++;
        
        if (base64_decode_table[c] == 0xFF) {
            continue;  // 跳过非法字符
        }
        
        temp = (temp << 6) | base64_decode_table[c];
        bits += 6;
        
        if (bits >= 8) {
            bits -= 8;
            decoded[len++] = (uint8_t)(temp >> bits);
        }
    }
    
    return len;
}

/**
 * @brief 擦除 OTA Flash 区域
 * 
 * @param 无
 * @return bool true - 成功, false - 失败
 */
static bool erase_flash_for_ota(void) {
    
    // TODO: 调用实际的 Flash 擦除函数
    // 示例: return FLASH_EraseSector(OTA_FLASH_SECTOR);
    // 由于使用的是内部flash，只能整块扇区擦除，故而在OTA启动时就擦除整个扇区
    
    return true;
}

/**
 * @brief 写入数据到 Flash
 * 
 * @param data 数据指针
 * @param len 数据长度
 * @param offset Flash 偏移地址
 * @return bool true - 成功, false - 失败
 */
static bool write_to_flash(const uint8_t* data, size_t len, uint32_t offset) {
    LOG_D(MODULE_OTA, "Writing %d bytes to flash at offset 0x%08X\r\n", len, offset);
    
    // TODO: 调用实际的 Flash 写入函数
    // 示例: return FLASH_Program(OTA_FLASH_ADDR + offset, data, len);
    
    return true;
}

/**
 * @brief 处理接收到的 JSON 数据块
 * 
 * @param json_string JSON 字符串指针
 * @return bool true - 成功, false - 失败
 */
bool ota_process_chunk(const char* json_string) {
    cJSON* root = NULL;
    cJSON* item = NULL;
    uint32_t current_seq = 0;
    
    if (json_string == NULL) {
        LOG_E(MODULE_OTA, "Invalid JSON string (NULL)\r\n");
        return false;
    }
    
    if (mutex_lock(ota_mutex) != pdTRUE) {
        LOG_E(MODULE_OTA, "Failed to lock OTA mutex\r\n");
        return false;
    }
    
    root = cJSON_Parse(json_string);
    if (root == NULL) {
        LOG_E(MODULE_OTA, "JSON parse error\r\n");
        goto error_exit;
    }
    
    item = cJSON_GetObjectItem(root, "sequence");
    if (item == NULL || !cJSON_IsNumber(item)) {
        LOG_E(MODULE_OTA, "Missing or invalid 'sequence' field\r\n");
        goto error_exit;
    }
    current_seq = (uint32_t)item->valueint;
    
    if (current_seq == 1) {
        item = cJSON_GetObjectItem(root, "version");
        if (item != NULL && cJSON_IsString(item) && item->valuestring != NULL) {
            snprintf(ota_info.version, sizeof(ota_info.version), "%s", item->valuestring);
        } else {
            LOG_E(MODULE_OTA, "Missing 'version' field in first chunk\r\n");
            goto error_exit;
        }
        
        item = cJSON_GetObjectItem(root, "bagSize");
        if (item == NULL || !cJSON_IsNumber(item)) {
            LOG_E(MODULE_OTA, "Missing or invalid 'bagSize' field\r\n");
            goto error_exit;
        }
        ota_info.bagSize = (uint32_t)item->valueint;
        
        if (ota_info.bagSize > OTA_MAX_SIZE) {
            LOG_E(MODULE_OTA, "Package size %d exceeds maximum %d\r\n", 
                  ota_info.bagSize, OTA_MAX_SIZE);
            goto error_exit;
        }
        
        item = cJSON_GetObjectItem(root, "bagType");
        if (item != NULL && cJSON_IsNumber(item)) {
            ota_info.bagType = (uint8_t)item->valueint;
        }
        
        item = cJSON_GetObjectItem(root, "chunkType");
        if (item != NULL && cJSON_IsNumber(item)) {
            ota_info.chunkType = (uint8_t)item->valueint;
        }
        
        if (!erase_flash_for_ota()) {
            LOG_E(MODULE_OTA, "Flash erase failed\r\n");
            goto error_exit;
        }
        
        ota_info.state = OTA_RECEIVING;
        ota_info.receivedSize = 0;
        ota_info.flashOffset = 0;
        ota_info.sequence = 0;
        
        LOG_I(MODULE_OTA, "Starting OTA upgrade: version=%s, size=%d bytes, type=%d\r\n", 
              ota_info.version, ota_info.bagSize, ota_info.bagType);
    }
    
    if (current_seq != ota_info.sequence + 1) {
        LOG_E(MODULE_OTA, "Sequence mismatch! Expected: %d, Received: %d\r\n",
              ota_info.sequence + 1, current_seq);
        goto error_exit;
    }
    ota_info.sequence = current_seq;
    
    item = cJSON_GetObjectItem(root, "data");
    if (item == NULL || !cJSON_IsString(item) || item->valuestring == NULL) {
        LOG_E(MODULE_OTA, "Missing or invalid 'data' field\r\n");
        goto error_exit;
    }
    
    uint8_t decoded_data[OTA_CHUNK_SIZE] = {0};
    size_t decoded_len = base64_decode(item->valuestring, decoded_data, sizeof(decoded_data));
    
    if (decoded_len == 0) {
        LOG_E(MODULE_OTA, "Base64 decode failed\r\n");
        goto error_exit;
    }
    
    if (!write_to_flash(decoded_data, decoded_len, ota_info.flashOffset)) {
        LOG_E(MODULE_OTA, "Flash write failed at offset %d\r\n", ota_info.flashOffset);
        ota_info.state = OTA_ERROR;
        goto error_exit;
    }
    
    ota_info.flashOffset += decoded_len;
    ota_info.receivedSize += decoded_len;
    
    LOG_I(MODULE_OTA, "Chunk %d processed: decoded=%d bytes, offset=%d, total=%d/%d\r\n",
          ota_info.sequence, decoded_len, ota_info.flashOffset, 
          ota_info.receivedSize, ota_info.bagSize);
    
    if (ota_info.receivedSize >= ota_info.bagSize) {
        if (ota_info.receivedSize == ota_info.bagSize) {
            ota_info.state = OTA_COMPLETE;
            LOG_I(MODULE_OTA, "OTA upgrade complete! Version: %s, Total: %d bytes\r\n", 
                  ota_info.version, ota_info.bagSize);
        } else {
            LOG_E(MODULE_OTA, "Size mismatch! Expected: %d, Received: %d\r\n", 
                  ota_info.bagSize, ota_info.receivedSize);
            ota_info.state = OTA_ERROR;
        }
    }
    
    cJSON_Delete(root);
    mutex_unlock(ota_mutex);
    return true;
    
error_exit:
    if (root != NULL) {
        cJSON_Delete(root);
    }
    mutex_unlock(ota_mutex);
    return false;
}

/**
 * @brief 获取 OTA 当前状态
 * 
 * @return OTA_State_t OTA 状态
 */
OTA_State_t ota_get_state(void) {
    OTA_State_t state;
    
    mutex_lock(ota_mutex);
    state = ota_info.state;
    mutex_unlock(ota_mutex);
    
    return state;
}

/**
 * @brief 获取 OTA 信息
 * 
 * @return OTA_Info_t* OTA 信息指针
 */
OTA_Info_t* ota_get_info(void) {
    return &ota_info;
}

/**
 * @brief 获取升级传包进度百分比
 * 
 * @return uint32_t 进度百分比 (0-100)
 */
uint32_t ota_get_progress(void) {
    uint32_t progress = 0;
    
    mutex_lock(ota_mutex);
    if (ota_info.bagSize > 0) {
        progress = (ota_info.receivedSize * 100) / ota_info.bagSize;
    }
    mutex_unlock(ota_mutex);
    
    return progress;
}

/**
 * @brief 重置 OTA 状态
 * 
 * @return void
 */
void ota_reset(void) {
    mutex_lock(ota_mutex);
    
    ota_info.state = OTA_IDLE;
    ota_info.receivedSize = 0;
    ota_info.flashOffset = 0;
    ota_info.sequence = 0;
    ota_info.bagSize = 0;
    ota_info.chunkType = 0;
    ota_info.bagType = 0;
    memset(ota_info.version, 0, sizeof(ota_info.version));
    
    mutex_unlock(ota_mutex);
    
    LOG_I(MODULE_OTA, "OTA state reset\r\n");
}

/**
 * @brief 初始化Upgrade模块
 * 
 * @return int 返回值
 */
static int upgrade_init(void) {
    json_init_with_os();
    ota_init();

    return 0;
}
MODULE_INIT(upgrade_init, "Upgrade");