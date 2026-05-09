#include "ring_buffer.h"

void rb_init(RingBuffer *rb) {
    rb->head = 0;
    rb->tail = 0;
}

size_t rb_available(const RingBuffer *rb) {
    return (rb->head - rb->tail) & (RING_BUFFER_SIZE - 1);
}

bool rb_push(RingBuffer *rb, uint8_t byte) {
    size_t next_head = (rb->head + 1) & (RING_BUFFER_SIZE - 1);
    if (next_head == rb->tail) {
        return false;
    }
    rb->buf[rb->head] = byte;
    rb->head = next_head;
    return true;
}

bool rb_pop(RingBuffer *rb, uint8_t *out) {
    if (rb->head == rb->tail) {
        return false;
    }
    *out = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) & (RING_BUFFER_SIZE - 1);
    return true;
}
