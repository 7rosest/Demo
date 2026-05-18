#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "stdint.h"

typedef struct {
    uint8_t *source;  //缓冲区指针
    uint32_t br;      //读指针
    uint32_t bw;      //写指针
    uint32_t btoRead; //可用字节数
    uint32_t length;  //缓冲区大小
}ringbuffer_t;

void create_ringBuffer(ringbuffer_t *ringBuf, uint8_t *buf, uint32_t buf_len);
uint32_t write_ringBuffer(uint8_t *buffer, uint32_t size, ringbuffer_t *ringBuf);
uint32_t read_ringBuffer(uint8_t *buffer, uint32_t size, ringbuffer_t *ringBuf);
void clear_ringBuffer(ringbuffer_t *ringBuf);
uint32_t get_ringBuffer_btoRead(ringbuffer_t *ringBuf);
uint32_t get_ringBuffer_length(ringbuffer_t *ringBuf);

#endif 