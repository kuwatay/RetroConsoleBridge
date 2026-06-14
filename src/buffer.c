#include "buffer.h"

void rb_init(ring_buffer_t *rb, uint8_t *storage, size_t size) {
    rb->data = storage;
    rb->size = size;
    rb->head = rb->tail = 0;
}

bool rb_empty(const ring_buffer_t *rb) { return rb->head == rb->tail; }
bool rb_full(const ring_buffer_t *rb) { return ((rb->head + 1) % rb->size) == rb->tail; }

bool rb_put(ring_buffer_t *rb, uint8_t ch) {
    size_t next = (rb->head + 1) % rb->size;
    if (next == rb->tail) return false;
    rb->data[rb->head] = ch;
    rb->head = next;
    return true;
}

bool rb_get(ring_buffer_t *rb, uint8_t *ch) {
    if (rb_empty(rb)) return false;
    *ch = rb->data[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    return true;
}

size_t rb_count(const ring_buffer_t *rb) {
    return rb->head >= rb->tail ? rb->head - rb->tail : rb->size - rb->tail + rb->head;
}
