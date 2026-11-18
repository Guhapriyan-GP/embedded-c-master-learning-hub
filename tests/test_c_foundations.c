/**
 * @file    test_c_foundations.c
 * @brief   Stage 01 Standalone C Foundations Unit Test Suite.
 *
 * Verifies mastery of C11 embedded foundations (Concept IDs: C-DATA-001,
 * C-BIT-001, C-PTR-001, C-MEM-001, EMB-MMIO-001):
 *   1. Fixed-width integer limits, two's complement, and unsigned wraparound math
 *   2. Single-bit and multi-bit register field masking/insertion/extraction
 *   3. Endian-safe byte packing & unpacking without strict-aliasing UB
 *   4. Pointer arithmetic, struct padding, and offsetof() MMIO register layout
 *   5. Storage duration (static) and const/volatile qualifier semantics
 *
 * Self-contained standard C11 test runner (zero external dependencies).
 */

#include <stdbool.h>
#include <stddef.h>
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
            fprintf(stderr, "  [FAIL] %s:%d: expected 0x%08X, got 0x%08X (%s)\n",       \
                    __FILE__, __LINE__, _e, _a, #actual);                               \
        }                                                                               \
    } while (0)

/* --- Helper Bitfield Functions --- */
static uint32_t bit_set(uint32_t reg, uint8_t bit) {
    return reg | (1UL << bit);
}

static uint32_t bit_clear(uint32_t reg, uint8_t bit) {
    return reg & ~(1UL << bit);
}

static uint32_t bit_toggle(uint32_t reg, uint8_t bit) {
    return reg ^ (1UL << bit);
}

static uint32_t field_write(uint32_t reg, uint8_t pos, uint8_t width, uint32_t value) {
    uint32_t mask = ((1UL << width) - 1UL) << pos;
    return (reg & ~mask) | ((value << pos) & mask);
}

static uint32_t field_read(uint32_t reg, uint8_t pos, uint8_t width) {
    uint32_t mask = (1UL << width) - 1UL;
    return (reg >> pos) & mask;
}

static uint32_t pack_be32(const uint8_t b[4]) {
    return ((uint32_t)b[0] << 24) |
           ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8)  |
           ((uint32_t)b[3]);
}

static uint32_t pack_le32(const uint8_t b[4]) {
    return ((uint32_t)b[3] << 24) |
           ((uint32_t)b[2] << 16) |
           ((uint32_t)b[1] << 8)  |
           ((uint32_t)b[0]);
}

/* Simulated STM32G4 GPIO_TypeDef layout (verified against RM0440 Ch. 9) */
typedef struct {
    volatile uint32_t MODER;   /* 0x00 */
    volatile uint32_t OTYPER;  /* 0x04 */
    volatile uint32_t OSPEEDR; /* 0x08 */
    volatile uint32_t PUPDR;   /* 0x0C */
    volatile uint32_t IDR;     /* 0x10 */
    volatile uint32_t ODR;     /* 0x14 */
    volatile uint32_t BSRR;    /* 0x18 */
    volatile uint32_t LCKR;    /* 0x1C */
    volatile uint32_t AFR[2];  /* 0x20, 0x24 */
    volatile uint32_t BRR;     /* 0x28 */
} fake_gpio_t;

typedef struct {
    uint8_t  tag;    /* offset 0, followed by 3 bytes padding */
    uint32_t value;  /* offset 4 */
    uint16_t crc;    /* offset 8, followed by 2 bytes tail padding */
} padded_packet_t;

static uint32_t static_counter_next(void) {
    static uint32_t counter = 0U;
    counter += 1U;
    return counter;
}

/* --- Test Cases --- */
static void test_fixed_width_and_wraparound(void) {
    ASSERT_EQ_U32(1U, sizeof(uint8_t));
    ASSERT_EQ_U32(2U, sizeof(uint16_t));
    ASSERT_EQ_U32(4U, sizeof(uint32_t));
    ASSERT_EQ_U32(8U, sizeof(uint64_t));

    /* Two's complement representation check */
    int16_t neg_one = -1;
    uint16_t raw = 0U;
    memcpy(&raw, &neg_one, sizeof(raw));
    ASSERT_EQ_U32(0xFFFFU, raw);

    /* Unsigned modular elapsed time across 32-bit DWT/SysTick rollover */
    uint32_t t_start = 0xFFFFFFF0UL;
    uint32_t t_now   = 0x00000010UL;
    uint32_t elapsed = (uint32_t)(t_now - t_start);
    ASSERT_EQ_U32(0x20U, elapsed);
}

static void test_bit_manipulation_and_fields(void) {
    uint32_t reg = 0x00000000UL;
    reg = bit_set(reg, 5U);
    ASSERT_EQ_U32(0x00000020UL, reg);

    reg = bit_toggle(reg, 5U);
    ASSERT_EQ_U32(0x00000000UL, reg);

    /* STM32G4 GPIOA->MODER reset value is 0xABFFFFFF; configure PA5 = Output (0b01) */
    uint32_t moder = 0xABFFFFFFUL;
    moder = field_write(moder, 10U, 2U, 0x1U);
    ASSERT_EQ_U32(0x1U, field_read(moder, 10U, 2U));
    ASSERT_EQ_U32(0xABFFF7FFUL, moder);

    /* Configure PC13 = Input (0b00) from STM32G4 reset 0xFFFFFFFF */
    uint32_t gpioc_moder = 0xFFFFFFFFUL;
    gpioc_moder = field_write(gpioc_moder, 26U, 2U, 0x0U);
    ASSERT_EQ_U32(0x0U, field_read(gpioc_moder, 26U, 2U));
    ASSERT_EQ_U32(0xF3FFFFFFUL, gpioc_moder);

    /* Clear single bit */
    ASSERT_EQ_U32(0xF3FFFFDFUL, bit_clear(gpioc_moder, 5U));
}

static void test_byte_packing_endianness(void) {
    const uint8_t bytes[4] = { 0x12U, 0x34U, 0x56U, 0x78U };
    ASSERT_EQ_U32(0x12345678UL, pack_be32(bytes));
    ASSERT_EQ_U32(0x78563412UL, pack_le32(bytes));

    /* Verify STM32G4 initial MSP vector word 0x20018000 in little-endian flash binary */
    const uint8_t estack_bytes[4] = { 0x00U, 0x80U, 0x01U, 0x20U };
    ASSERT_EQ_U32(0x20018000UL, pack_le32(estack_bytes));
}

static void test_pointer_offsets_and_struct_layout(void) {
    /* Verify simulated STM32G4 GPIO register offsets */
    ASSERT_EQ_U32(0x00U, offsetof(fake_gpio_t, MODER));
    ASSERT_EQ_U32(0x04U, offsetof(fake_gpio_t, OTYPER));
    ASSERT_EQ_U32(0x08U, offsetof(fake_gpio_t, OSPEEDR));
    ASSERT_EQ_U32(0x0CU, offsetof(fake_gpio_t, PUPDR));
    ASSERT_EQ_U32(0x10U, offsetof(fake_gpio_t, IDR));
    ASSERT_EQ_U32(0x14U, offsetof(fake_gpio_t, ODR));
    ASSERT_EQ_U32(0x18U, offsetof(fake_gpio_t, BSRR));
    ASSERT_EQ_U32(0x20U, offsetof(fake_gpio_t, AFR));
    ASSERT_EQ_U32(0x28U, offsetof(fake_gpio_t, BRR));

    /* Pointer arithmetic stride equals sizeof(element) */
    uint32_t words[4] = { 0x10U, 0x20U, 0x30U, 0x40U };
    const uint32_t *p0 = &words[0];
    const uint32_t *p3 = p0 + 3;
    ASSERT_EQ_U32(0x40U, *p3);
    ASSERT_EQ_U32(12U, (uint32_t)((const uint8_t *)p3 - (const uint8_t *)p0));

    /* Struct alignment and padding */
    ASSERT_EQ_U32(0U, offsetof(padded_packet_t, tag));
    ASSERT_EQ_U32(4U, offsetof(padded_packet_t, value));
    ASSERT_EQ_U32(8U, offsetof(padded_packet_t, crc));
    ASSERT_EQ_U32(12U, sizeof(padded_packet_t));
}

static void test_static_and_qualifiers(void) {
    ASSERT_EQ_U32(1U, static_counter_next());
    ASSERT_EQ_U32(2U, static_counter_next());
    ASSERT_EQ_U32(3U, static_counter_next());

    fake_gpio_t fake_port;
    memset(&fake_port, 0, sizeof(fake_port));
    fake_port.IDR = (1UL << 13U); /* Active-HIGH B1 pressed */
    const volatile uint32_t *idr_ro = &fake_port.IDR;
    ASSERT_TRUE((*idr_ro & (1UL << 13U)) != 0UL);
}

int main(void) {
    printf("Running Stage 01 C Foundations Unit Tests...\n");
    test_fixed_width_and_wraparound();
    test_bit_manipulation_and_fields();
    test_byte_packing_endianness();
    test_pointer_offsets_and_struct_layout();
    test_static_and_qualifiers();

    printf("Results: %d assertions executed, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
