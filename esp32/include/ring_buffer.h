#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RING_BUFFER_SIZE 512

typedef struct {
    uint8_t buf[RING_BUFFER_SIZE];
    volatile size_t head;
    volatile size_t tail;
} RingBuffer;

void rb_init(RingBuffer *rb);
size_t rb_available(const RingBuffer *rb);
bool rb_push(RingBuffer *rb, uint8_t byte);
bool rb_pop(RingBuffer *rb, uint8_t *out);
