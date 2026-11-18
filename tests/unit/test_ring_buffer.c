/**
 * @file    test_ring_buffer.c
 * @brief   Host Unit Test Suite for 23-code-library/drivers/ring_buffer.c.
 *
 * Tests initialization validation, FIFO ordering, full-capacity utilization,
 * wraparound across multiple revolutions, bulk read/write, and 32-bit unsigned
 * index rollover behavior.
 *
 * Concept IDs: TOOL-QA-001, EMB-CONC-001
 */

#include "ring_buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int g_tests_run    = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(expr)                                                               \
    do {                                                                                \
        g_tests_run++;                                                                  \
        if (!(expr)) {                                                                  \
            g_tests_failed++;                                                           \
            fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #expr);         \
        }                                                                               \
    } while (0)

#define ASSERT_EQ_U32(expected, actual)                                                 \
    do {                                                                                \
        uint32_t _e = (uint32_t)(expected);                                             \
        uint32_t _a = (uint32_t)(actual);                                               \
        g_tests_run++;                                                                  \
        if (_e != _a) {                                                                 \
            g_tests_failed++;                                                           \
            fprintf(stderr, "  [FAIL] %s:%d: expected %u, got %u (%s)\n",               \
                    __FILE__, __LINE__, _e, _a, #actual);                               \
        }                                                                               \
    } while (0)

static void test_init_validation(void) {
    ring_buffer_t rb;
    uint8_t buf[16];

    ASSERT_TRUE(!rb_init(NULL, buf, 16U));
    ASSERT_TRUE(!rb_init(&rb, NULL, 16U));
    ASSERT_TRUE(!rb_init(&rb, buf, 0U));
    ASSERT_TRUE(!rb_init(&rb, buf, 1U));
    ASSERT_TRUE(!rb_init(&rb, buf, 3U));
    ASSERT_TRUE(!rb_init(&rb, buf, 12U));

    ASSERT_TRUE(rb_init(&rb, buf, 16U));
    ASSERT_TRUE(rb_is_empty(&rb));
    ASSERT_TRUE(!rb_is_full(&rb));
    ASSERT_EQ_U32(0U, rb_count(&rb));
    ASSERT_EQ_U32(16U, rb_free(&rb));
}

static void test_push_pop_peek_and_full_capacity(void) {
    ring_buffer_t rb;
    uint8_t buf[8];
    uint8_t out = 0U;

    ASSERT_TRUE(rb_init(&rb, buf, 8U));
    ASSERT_TRUE(!rb_pop(&rb, &out));
    ASSERT_TRUE(!rb_peek(&rb, &out));

    /* Fill all 8 bytes (power-of-2 free-running indices use 100% capacity) */
    for (uint8_t i = 0U; i < 8U; ++i) {
        ASSERT_TRUE(rb_push(&rb, (uint8_t)(0xA0U + i)));
    }
    ASSERT_TRUE(rb_is_full(&rb));
    ASSERT_EQ_U32(8U, rb_count(&rb));
    ASSERT_EQ_U32(0U, rb_free(&rb));

    /* 9th push must fail without corrupting stored bytes */
    ASSERT_TRUE(!rb_push(&rb, 0xFFU));

    /* Peek must read oldest byte without consuming it */
    ASSERT_TRUE(rb_peek(&rb, &out));
    ASSERT_EQ_U32(0xA0U, out);
    ASSERT_EQ_U32(8U, rb_count(&rb));

    /* Drain all 8 bytes in exact FIFO order */
    for (uint8_t i = 0U; i < 8U; ++i) {
        ASSERT_TRUE(rb_pop(&rb, &out));
        ASSERT_EQ_U32((uint32_t)(0xA0U + i), out);
    }
    ASSERT_TRUE(rb_is_empty(&rb));
    ASSERT_TRUE(!rb_pop(&rb, &out));
}

static void test_bulk_read_write_and_wraparound(void) {
    ring_buffer_t rb;
    uint8_t buf[8];
    ASSERT_TRUE(rb_init(&rb, buf, 8U));

    /* Push 6 bytes, pop 4 bytes -> tail=4, head=6 */
    const uint8_t seq1[6] = { 1U, 2U, 3U, 4U, 5U, 6U };
    uint8_t rx[8];
    ASSERT_EQ_U32(6U, rb_write(&rb, seq1, 6U));
    ASSERT_EQ_U32(4U, rb_read(&rb, rx, 4U));
    ASSERT_TRUE(memcmp(rx, seq1, 4U) == 0);
    ASSERT_EQ_U32(2U, rb_count(&rb));

    /* Attempt to write 8 bytes when only 6 slots are free (wraps past index 7 -> 0..3) */
    const uint8_t seq2[8] = { 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U };
    ASSERT_EQ_U32(6U, rb_write(&rb, seq2, 8U));
    ASSERT_TRUE(rb_is_full(&rb));

    /* Drain all 8 bytes: expect {5, 6, 10, 11, 12, 13, 14, 15} */
    const uint8_t expected[8] = { 5U, 6U, 10U, 11U, 12U, 13U, 14U, 15U };
    ASSERT_EQ_U32(8U, rb_read(&rb, rx, 8U));
    ASSERT_TRUE(memcmp(rx, expected, 8U) == 0);
    ASSERT_TRUE(rb_is_empty(&rb));
}

static void test_uint32_index_rollover(void) {
    ring_buffer_t rb;
    uint8_t buf[4];
    ASSERT_TRUE(rb_init(&rb, buf, 4U));

    /* Simulate indices right before 32-bit unsigned overflow */
    rb.head = 0xFFFFFFFEUL;
    rb.tail = 0xFFFFFFFEUL;

    ASSERT_TRUE(rb_is_empty(&rb));
    ASSERT_TRUE(rb_push(&rb, 0x11U)); /* head -> 0xFFFFFFFF */
    ASSERT_TRUE(rb_push(&rb, 0x22U)); /* head -> 0x00000000 (wrapped!) */
    ASSERT_TRUE(rb_push(&rb, 0x33U)); /* head -> 0x00000001 */
    ASSERT_TRUE(rb_push(&rb, 0x44U)); /* head -> 0x00000002 */
    ASSERT_TRUE(rb_is_full(&rb));
    ASSERT_EQ_U32(4U, rb_count(&rb));
    ASSERT_TRUE(!rb_push(&rb, 0x55U));

    uint8_t b = 0U;
    ASSERT_TRUE(rb_pop(&rb, &b)); ASSERT_EQ_U32(0x11U, b);
    ASSERT_TRUE(rb_pop(&rb, &b)); ASSERT_EQ_U32(0x22U, b);
    ASSERT_TRUE(rb_pop(&rb, &b)); ASSERT_EQ_U32(0x33U, b);
    ASSERT_TRUE(rb_pop(&rb, &b)); ASSERT_EQ_U32(0x44U, b);
    ASSERT_TRUE(rb_is_empty(&rb));

    /* Verify rb_reset restores clean state */
    ASSERT_TRUE(rb_push(&rb, 0x99U));
    rb_reset(&rb);
    ASSERT_TRUE(rb_is_empty(&rb));
    ASSERT_EQ_U32(0U, rb_count(&rb));
}

static void test_null_and_defensive_guards(void) {
    ring_buffer_t bad_rb;
    memset(&bad_rb, 0, sizeof(bad_rb));
    bad_rb.storage  = NULL;
    bad_rb.capacity = 8U;
    bad_rb.mask     = 7U;

    uint8_t byte   = 0x5AU;
    uint8_t buf[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };
    uint8_t out[8] = { 0U };

    /* NULL control block checks */
    rb_reset(NULL);
    ASSERT_EQ_U32(0U, rb_count(NULL));
    ASSERT_EQ_U32(0U, rb_free(NULL));
    ASSERT_TRUE(rb_is_empty(NULL));
    ASSERT_TRUE(rb_is_full(NULL));
    ASSERT_TRUE(!rb_push(NULL, 0x11U));
    ASSERT_TRUE(!rb_pop(NULL, &byte));
    ASSERT_TRUE(!rb_peek(NULL, &byte));
    ASSERT_EQ_U32(0U, rb_write(NULL, buf, 4U));
    ASSERT_EQ_U32(0U, rb_read(NULL, out, 4U));

    /* NULL backing storage checks */
    ASSERT_TRUE(!rb_push(&bad_rb, 0x22U));
    ASSERT_TRUE(!rb_pop(&bad_rb, &byte));
    ASSERT_TRUE(!rb_peek(&bad_rb, &byte));
    ASSERT_EQ_U32(0U, rb_write(&bad_rb, buf, 4U));
    ASSERT_EQ_U32(0U, rb_read(&bad_rb, out, 4U));

    /* NULL data pointer / zero-length checks on valid ring buffer */
    ring_buffer_t rb;
    uint8_t storage[8];
    ASSERT_TRUE(rb_init(&rb, storage, 8U));
    ASSERT_TRUE(!rb_pop(&rb, NULL));
    ASSERT_TRUE(!rb_peek(&rb, NULL));
    ASSERT_EQ_U32(0U, rb_write(&rb, NULL, 4U));
    ASSERT_EQ_U32(0U, rb_write(&rb, buf, 0U));
    ASSERT_EQ_U32(0U, rb_read(&rb, NULL, 4U));
    ASSERT_EQ_U32(0U, rb_read(&rb, out, 0U));
    ASSERT_EQ_U32(0U, rb_read(&rb, out, 4U)); /* empty bulk read returns 0 */

    /* Clamping guard if head - tail ever exceeded capacity during concurrent snapshot */
    rb.head = 20U;
    rb.tail = 0U;
    ASSERT_EQ_U32(8U, rb_count(&rb));
    ASSERT_EQ_U32(0U, rb_free(&rb));
    ASSERT_TRUE(rb_is_full(&rb));
    rb_reset(&rb);
    ASSERT_EQ_U32(0U, rb_count(&rb));
}

int main(void) {
    printf("Running Ring Buffer Host Unit Tests...\n");
    test_init_validation();
    test_push_pop_peek_and_full_capacity();
    test_bulk_read_write_and_wraparound();
    test_uint32_index_rollover();
    test_null_and_defensive_guards();

    printf("Results: %d assertions executed, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
