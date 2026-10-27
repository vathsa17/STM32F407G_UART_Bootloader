#include "stm32f407xx.h"

bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *byte)
{
    if (rb->head == rb->tail)
    {
        return false;   // Buffer empty
    }

    *byte = rb->buffer[rb->tail];

    rb->tail++;

    if (rb->tail == BUFFER_SIZE)
    {
        rb->tail = 0;
    }

    return true;
}

bool RingBuffer_Put(RingBuffer_t *rb, uint8_t byte)
{
    uint8_t next_head = rb->head + 1;

    if (next_head == BUFFER_SIZE)
    {
        next_head = 0;
    }

    if (next_head == rb->tail)
    {
        return false;   // Buffer full
    }

    rb->buffer[rb->head] = byte;

    rb->head = next_head;

    return true;
}

bool RingBuffer_IsFull(RingBuffer_t *rb)
{
    uint8_t next_head = rb->head + 1;

    if (next_head == BUFFER_SIZE)
    {
        next_head = 0;
    }

    return next_head == rb->tail;
}

bool RingBuffer_IsEmpty(RingBuffer_t *rb)
{
    return rb->head == rb->tail;
}


void RingBuffer_Init(RingBuffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}
