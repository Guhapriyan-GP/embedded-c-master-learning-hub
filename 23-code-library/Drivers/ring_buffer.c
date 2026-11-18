/**
 * @file    ring_buffer.c
 * @brief   Lock-free SPSC byte ring buffer implementation using C11 <stdatomic.h>
 *          acquire/release semantics (with __STDC_NO_ATOMICS__ fallback).
 */

#include "ring_buffer.h"

#if RB_USE_C11_ATOMICS
static inline uint32_t rb_load_relaxed(const rb_index_t *idx) {
    /* Cast away const only for pre-C17 strict C11 <stdatomic.h> signature compatibility */
    return atomic_load_explicit((rb_index_t *)(uintptr_t)idx, memory_order_relaxed);
}

static inline uint32_t rb_load_acquire(const rb_index_t *idx) {
    return atomic_load_explicit((rb_index_t *)(uintptr_t)idx, memory_order_acquire);
}

static inline void rb_store_relaxed(rb_index_t *idx, uint32_t val) {
    atomic_store_explicit(idx, val, memory_order_relaxed);
}

static inline void rb_store_release(rb_index_t *idx, uint32_t val) {
    atomic_store_explicit(idx, val, memory_order_release);
}
#else
#if defined(__GNUC__)
#define RB_BARRIER() __sync_synchronize()
#else
#define RB_BARRIER() ((void)0)
#endif

static inline uint32_t rb_load_relaxed(const rb_index_t *idx) {
    return *idx;
}

static inline uint32_t rb_load_acquire(const rb_index_t *idx) {
    uint32_t val = *idx;
    RB_BARRIER();
    return val;
}

static inline void rb_store_relaxed(rb_index_t *idx, uint32_t val) {
    *idx = val;
}

static inline void rb_store_release(rb_index_t *idx, uint32_t val) {
    RB_BARRIER();
    *idx = val;
}
#endif

static bool is_power_of_two(uint32_t n) {
    return (n >= 2U) && ((n & (n - 1U)) == 0U);
}

bool rb_init(ring_buffer_t *rb, uint8_t *storage, uint32_t capacity) {
    if ((rb == NULL) || (storage == NULL) || !is_power_of_two(capacity)) {
        return false;
    }

    rb->storage  = storage;
    rb->capacity = capacity;
    rb->mask     = capacity - 1U;
    rb_store_relaxed(&rb->head, 0U);
    rb_store_release(&rb->tail, 0U);
    return true;
}

void rb_reset(ring_buffer_t *rb) {
    if (rb == NULL) {
        return;
    }
    rb_store_relaxed(&rb->head, 0U);
    rb_store_release(&rb->tail, 0U);
}

uint32_t rb_count(const ring_buffer_t *rb) {
    if (rb == NULL) {
        return 0U;
    }
    uint32_t head  = rb_load_acquire(&rb->head);
    uint32_t tail  = rb_load_acquire(&rb->tail);
    uint32_t count = (uint32_t)(head - tail);
    /* Clamp to capacity to prevent unsigned underflow in rb_free() */
    return (count > rb->capacity) ? rb->capacity : count;
}

uint32_t rb_free(const ring_buffer_t *rb) {
    if (rb == NULL) {
        return 0U;
    }
    uint32_t used = rb_count(rb);
    return (used >= rb->capacity) ? 0U : (rb->capacity - used);
}

bool rb_is_empty(const ring_buffer_t *rb) {
    return (rb == NULL) || (rb_count(rb) == 0U);
}

bool rb_is_full(const ring_buffer_t *rb) {
    return (rb == NULL) || (rb_count(rb) >= rb->capacity);
}

bool rb_push(ring_buffer_t *rb, uint8_t byte) {
    if ((rb == NULL) || (rb->storage == NULL)) {
        return false;
    }

    uint32_t head = rb_load_relaxed(&rb->head);
    uint32_t tail = rb_load_acquire(&rb->tail);

    if ((uint32_t)(head - tail) >= rb->capacity) {
        return false;
    }

    rb->storage[head & rb->mask] = byte;
    /* Release store guarantees payload write happens-before head advances */
    rb_store_release(&rb->head, head + 1U);
    return true;
}

bool rb_pop(ring_buffer_t *rb, uint8_t *out_byte) {
    if ((rb == NULL) || (rb->storage == NULL) || (out_byte == NULL)) {
        return false;
    }

    uint32_t tail = rb_load_relaxed(&rb->tail);
    uint32_t head = rb_load_acquire(&rb->head);

    if (head == tail) {
        return false;
    }

    *out_byte = rb->storage[tail & rb->mask];
    /* Release store guarantees payload read completes before slot is freed */
    rb_store_release(&rb->tail, tail + 1U);
    return true;
}

bool rb_peek(const ring_buffer_t *rb, uint8_t *out_byte) {
    if ((rb == NULL) || (rb->storage == NULL) || (out_byte == NULL)) {
        return false;
    }

    uint32_t tail = rb_load_relaxed(&rb->tail);
    uint32_t head = rb_load_acquire(&rb->head);

    if (head == tail) {
        return false;
    }

    *out_byte = rb->storage[tail & rb->mask];
    return true;
}

uint32_t rb_write(ring_buffer_t *rb, const uint8_t *data, uint32_t len) {
    if ((rb == NULL) || (rb->storage == NULL) || (data == NULL) || (len == 0U)) {
        return 0U;
    }

    uint32_t head      = rb_load_relaxed(&rb->head);
    uint32_t tail      = rb_load_acquire(&rb->tail);
    uint32_t available = rb->capacity - (uint32_t)(head - tail);
    uint32_t to_write  = (len < available) ? len : available;

    for (uint32_t i = 0U; i < to_write; ++i) {
        rb->storage[(head + i) & rb->mask] = data[i];
    }

    rb_store_release(&rb->head, head + to_write);
    return to_write;
}

uint32_t rb_read(ring_buffer_t *rb, uint8_t *out_data, uint32_t len) {
    if ((rb == NULL) || (rb->storage == NULL) || (out_data == NULL) || (len == 0U)) {
        return 0U;
    }

    uint32_t tail    = rb_load_relaxed(&rb->tail);
    uint32_t head    = rb_load_acquire(&rb->head);
    uint32_t count   = (uint32_t)(head - tail);
    uint32_t to_read = (len < count) ? len : count;

    for (uint32_t i = 0U; i < to_read; ++i) {
        out_data[i] = rb->storage[(tail + i) & rb->mask];
    }

    rb_store_release(&rb->tail, tail + to_read);
    return to_read;
}
