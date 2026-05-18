#include "ring_buffer.h"
#include "string.h"
#include "../Log/log.h"

/**
 * @brief 创建环形缓冲区
 * 
 * @param ringBuf 环形缓冲区指针
 * @param buf 缓冲区数据指针
 * @param buf_len 缓冲区长度
 */
void create_ringBuffer(ringbuffer_t *ringBuf, uint8_t *buf, uint32_t buf_len) {
	ringBuf->br         = 0;
	ringBuf->bw         = 0;
	ringBuf->btoRead    = 0;
	ringBuf->source     = buf;
	ringBuf->length     = buf_len;
}

/**
 * @brief 清空环形缓冲区
 * 
 * @param ringBuf 环形缓冲区指针
 */
void clear_ringBuffer(ringbuffer_t *ringBuf) {
	ringBuf->br         = 0;
	ringBuf->bw         = 0;
	ringBuf->btoRead    = 0;
}

/**
 * @brief 向环形缓冲区写入数据
 * 
 * @param buffer 数据缓冲区指针
 * @param size 写入数据长度
 * @param ringBuf 环形缓冲区指针
 * @return uint32_t 实际写入的数据长度
 */
uint32_t write_ringBuffer(uint8_t *buffer, uint32_t size, ringbuffer_t *ringBuf) {
	uint32_t len            = 0;
	uint32_t ringBuf_bw     = ringBuf->bw;
	uint32_t ringBuf_len    = ringBuf->length;
	uint8_t *ringBuf_source = ringBuf->source;

	if ((ringBuf_bw + size) <= ringBuf_len) {
		memcpy(ringBuf_source + ringBuf_bw, buffer, size);
	} else {
		len = ringBuf_len - ringBuf_bw;
		memcpy(ringBuf_source + ringBuf_bw, buffer, len);
		memcpy(ringBuf_source, buffer + len, size - len);
	}

	ringBuf->bw = (ringBuf->bw + size) % ringBuf_len;
	ringBuf->btoRead += size;

	return size;
}

/**
 * @brief 从环形缓冲区读取数据
 * 
 * @param buffer 数据缓冲区指针
 * @param size 读取数据长度
 * @param ringBuf 环形缓冲区指针
 * @return uint32_t 实际读取的数据长度
 */
uint32_t read_ringBuffer(uint8_t *buffer, uint32_t size, ringbuffer_t *ringBuf) {
	uint32_t len            = 0;
	uint32_t ringBuf_br     = ringBuf->br;
	uint32_t ringBuf_len    = ringBuf->length;
    uint8_t *ringBuf_source = ringBuf->source;

    if (ringBuf->btoRead == 0) return 0;
    if (size > ringBuf->btoRead) size = ringBuf->btoRead;

	if ((ringBuf_br + size ) <= ringBuf_len) {
		memcpy(buffer, ringBuf_source + ringBuf_br, size);
	} else {
		len = ringBuf_len - ringBuf_br;
		memcpy(buffer, ringBuf_source + ringBuf_br, len);
		memcpy(buffer + len, ringBuf_source, size - len);
	}

	ringBuf->br = (ringBuf->br + size) % ringBuf_len;
	ringBuf->btoRead -= size;

	return size;
}

/**
 * @brief 获取环形缓冲区中待读取的数据长度
 * 
 * @param ringBuf 环形缓冲区指针
 * @return uint32_t 待读取的数据长度
 */
uint32_t get_ringBuffer_btoRead(ringbuffer_t *ringBuf) {
	return ringBuf->btoRead;
}

/**
 * @brief 获取环形缓冲区的长度
 * 
 * @param ringBuf 环形缓冲区指针
 * @return uint32_t 缓冲区长度
 */	
uint32_t get_ringBuffer_length(ringbuffer_t *ringBuf) {
	return ringBuf->length;
}

