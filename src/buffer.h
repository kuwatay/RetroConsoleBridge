#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t size;
    volatile size_t head;
    volatile size_t tail;
} ring_buffer_t;

void rb_init(ring_buffer_t *rb, uint8_t *storage, size_t size);
bool rb_put(ring_buffer_t *rb, uint8_t ch);
bool rb_get(ring_buffer_t *rb, uint8_t *ch);
bool rb_empty(const ring_buffer_t *rb);
bool rb_full(const ring_buffer_t *rb);
size_t rb_count(const ring_buffer_t *rb);
