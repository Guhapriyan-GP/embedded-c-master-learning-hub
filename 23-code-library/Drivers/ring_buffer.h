/**
 * @file    ring_buffer.h
 * @brief   Lock-free Single-Producer Single-Consumer (SPSC) byte ring buffer
 *          with power-of-2 capacity masking and C11 memory barrier discipline.
 *
 * Designed for ISR <-> main-loop communication (e.g. USART RX/TX) and 100%
 * host unit-testable without hardware dependencies.
 *
 * Concepts: EMB-CONC-001, C-MEM-001, FW-ARCH-001
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__cplusplus) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define RB_USE_C11_ATOMICS 1
typedef _Atomic uint32_t rb_index_t;
#else
#define RB_USE_C11_ATOMICS 0
typedef volatile uint32_t rb_index_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPSC byte ring buffer control block.
 *
 * Capacity MUST be a non-zero power of two (e.g. 16, 64, 256, 1024).
 * Uses free-running unsigned 32-bit C11 _Atomic head/tail indices so the full
 * capacity (all N bytes, not N-1) is usable while distinguishing full vs empty
 * via `(head - tail) == capacity` and `head == tail` with zero C11 data-race UB.
 */
typedef struct {
    uint8_t   *storage;  /**< Caller-owned backing byte array (no malloc) */
    uint32_t   capacity; /**< Power-of-2 buffer size in bytes             */
    uint32_t   mask;     /**< capacity - 1 for branchless wraparound      */
    rb_index_t head;     /**< Producer write index (monotonically grows)  */
    rb_index_t tail;     /**< Consumer read index  (monotonically grows)  */
} ring_buffer_t;

/**
 * @brief Initialize a ring buffer over caller-provided static storage.
 * @param rb        Pointer to control block.
 * @param storage   Pointer to byte array of length @p capacity.
 * @param capacity  Buffer capacity in bytes; MUST be a power of 2 (>= 2).
 * @return true on success, false if arguments are invalid (NULL or non-power-of-2).
 */
bool rb_init(ring_buffer_t *rb, uint8_t *storage, uint32_t capacity);

/**
 * @brief Reset ring buffer to empty state (call only when producer & consumer are quiescent).
 */
void rb_reset(ring_buffer_t *rb);

/**
 * @brief Return instantaneous snapshot of bytes currently available to read.
 * @note  Valid for SPSC producer or consumer context (not an arbitrary third
 *        observer racing with concurrent rb_reset()). Result is clamped to capacity.
 */
uint32_t rb_count(const ring_buffer_t *rb);

/**
 * @brief Return instantaneous snapshot of free byte slots available to write.
 * @note  Valid for SPSC producer or consumer context (never underflows).
 */
uint32_t rb_free(const ring_buffer_t *rb);

/**
 * @brief Return true if buffer contains zero bytes.
 */
bool rb_is_empty(const ring_buffer_t *rb);

/**
 * @brief Return true if buffer has no free slots remaining.
 */
bool rb_is_full(const ring_buffer_t *rb);

/**
 * @brief Push a single byte (producer context only).
 * @return true if stored, false if buffer is full or invalid.
 */
bool rb_push(ring_buffer_t *rb, uint8_t byte);

/**
 * @brief Pop a single byte (consumer context only).
 * @return true if byte retrieved, false if buffer is empty or invalid.
 */
bool rb_pop(ring_buffer_t *rb, uint8_t *out_byte);

/**
 * @brief Peek at the oldest byte without advancing tail (consumer context only).
 * @return true if byte retrieved, false if buffer is empty or invalid.
 */
bool rb_peek(const ring_buffer_t *rb, uint8_t *out_byte);

/**
 * @brief Push up to @p len bytes (producer context only).
 * @return Actual number of bytes written (0..len).
 */
uint32_t rb_write(ring_buffer_t *rb, const uint8_t *data, uint32_t len);

/**
 * @brief Pop up to @p len bytes (consumer context only).
 * @return Actual number of bytes read (0..len).
 */
uint32_t rb_read(ring_buffer_t *rb, uint8_t *out_data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H */
