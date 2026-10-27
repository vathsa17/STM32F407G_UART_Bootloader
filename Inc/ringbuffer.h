/*
 * ringbuffer.h
 *
 *  Created on: 7 Oct 2026
 *      Author: shriv
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include "stm32f407xx.h"

#define BUFFER_SIZE 64

typedef struct
{
    uint8_t buffer[BUFFER_SIZE];

    volatile uint8_t head;
    volatile uint8_t tail;

} RingBuffer_t;

void RingBuffer_Init(RingBuffer_t *rb);
bool RingBuffer_IsEmpty(RingBuffer_t *rb);
bool RingBuffer_IsFull(RingBuffer_t *rb);
bool RingBuffer_Put(RingBuffer_t *rb, uint8_t data);
bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *data);

#endif /* RINGBUFFER_H_ */
