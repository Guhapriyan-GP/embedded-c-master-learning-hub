/**
 * @file    test_c_foundations.c
 * @brief   Stage 01–03 Standalone Embedded C Foundations Unit Test Suite.
 *
 * Verifies mastery of C11 embedded foundations (Concept IDs: C-DATA-001,
 * C-BIT-001, C-PTR-001, C-MEM-001, C-FUNC-001, C-TYPE-001, EMB-MMIO-001,
 * EMB-TIME-001, EMB-FSM-001, EMB-MATH-001):
 *   1. Fixed-width integer limits, two's complement, and unsigned wraparound math
 *   2. Integer promotions (~uint8_t), usual arithmetic signed/unsigned conversions
 *   3. Single-bit and multi-bit register field masking/insertion/extraction
 *   4. Endian-safe byte packing & memcpy strict-aliasing type punning
 *   5. Pointer arithmetic, alignment, struct padding, and offsetof() MMIO layout
 *   6. Discriminated (tagged) unions and bounds-checked function-pointer jump tables
 *   7. Q15 / Q31 fixed-point saturating DSP arithmetic
 *   8. Wraparound-safe 32-bit SysTick / DWT elapsed-time comparison
 *   9. Non-blocking 4-state button debounce FSM (glitch rejection + wraparound)
 *  10. Storage duration (static) and const/volatile qualifier semantics
 *
 * Verification Status: Host-Compiled & Executed (MSYS2 gcc 15.2.0, zero external deps).
 */

#include <limits.h>
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

#define ASSERT_EQ_I32(expected, actual)                                                 \
    do {                                                                                \
        int32_t _e = (int32_t)(expected);                                               \
        int32_t _a = (int32_t)(actual);                                                 \
        g_tests_run++;                                                                  \
        if (_e != _a) {                                                                 \
            g_tests_failed++;                                                           \
            fprintf(stderr, "  [FAIL] %s:%d: expected %d, got %d (%s)\n",               \
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

/* Simulated STM32G4 GPIO_TypeDef layout (verified against RM0440 Ch. 9 & stm32g474xx.h) */
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

typedef struct {
    uint32_t value;  /* offset 0 */
    uint16_t crc;    /* offset 4 */
    uint8_t  tag;    /* offset 6, followed by 1 byte tail padding -> 8 bytes total */
} compact_packet_t;

/* --- Tagged Union Telemetry Packet --- */
typedef enum {
    MSG_NONE        = 0U,
    MSG_TEMP_MDEGC  = 1U,
    MSG_VOLTAGE_MV  = 2U,
    MSG_FAULT_INFO  = 3U
} telemetry_tag_t;

typedef struct {
    telemetry_tag_t tag;
    union {
        int32_t  temp_mdegc;
        uint32_t voltage_mv;
        struct {
            uint16_t code;
            uint16_t subsystem;
        } fault;
    } payload;
} telemetry_msg_t;

static int32_t telemetry_extract_scaled(const telemetry_msg_t *msg) {
    if (msg == NULL) {
        return -1;
    }
    switch (msg->tag) {
        case MSG_TEMP_MDEGC: return msg->payload.temp_mdegc / 1000;
        case MSG_VOLTAGE_MV: return (int32_t)msg->payload.voltage_mv;
        case MSG_FAULT_INFO: return (int32_t)(((uint32_t)msg->payload.fault.subsystem << 16U) |
                                              (uint32_t)msg->payload.fault.code);
        case MSG_NONE:
        default:             return 0;
    }
}

/* --- Function-Pointer Jump Table --- */
typedef int32_t (*alu_op_fn)(int32_t a, int32_t b);

static int32_t alu_add(int32_t a, int32_t b) { return a + b; }
static int32_t alu_sub(int32_t a, int32_t b) { return a - b; }
static int32_t alu_mul(int32_t a, int32_t b) { return a * b; }
static int32_t alu_max(int32_t a, int32_t b) { return (a > b) ? a : b; }

static bool alu_dispatch(uint8_t opcode, int32_t a, int32_t b, int32_t *out) {
    static const alu_op_fn k_jump_table[4] = {
        alu_add, alu_sub, alu_mul, alu_max
    };
    if ((opcode >= 4U) || (out == NULL)) {
        return false;
    }
    *out = k_jump_table[opcode](a, b);
    return true;
}

/* --- Q15 & Q31 Saturating Fixed-Point Arithmetic --- */
static int16_t sat_i16(int32_t x) {
    if (x > INT16_MAX) {
        return INT16_MAX;
    }
    if (x < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)x;
}

static int16_t q15_add_sat(int16_t a, int16_t b) {
    return sat_i16((int32_t)a + (int32_t)b);
}

static int16_t q15_sub_sat(int16_t a, int16_t b) {
    return sat_i16((int32_t)a - (int32_t)b);
}

static int16_t q15_mul_sat(int16_t a, int16_t b) {
    if ((a == INT16_MIN) && (b == INT16_MIN)) {
        return INT16_MAX; /* (-1.0) * (-1.0) saturates to +0.999969 (0x7FFF) */
    }
    int32_t prod = ((int32_t)a * (int32_t)b) + (1 << 14); /* Round-half-up */
    return sat_i16(prod >> 15);
}

static int32_t sat_i32(int64_t x) {
    if (x > INT32_MAX) {
        return INT32_MAX;
    }
    if (x < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)x;
}

static int32_t q31_add_sat(int32_t a, int32_t b) {
    return sat_i32((int64_t)a + (int64_t)b);
}

static int32_t q31_mul_sat(int32_t a, int32_t b) {
    if ((a == INT32_MIN) && (b == INT32_MIN)) {
        return INT32_MAX;
    }
    int64_t prod = ((int64_t)a * (int64_t)b) + (1LL << 30);
    return sat_i32(prod >> 31);
}

/* --- Wraparound-Safe 32-Bit Tick Helper --- */
static bool tick_elapsed(uint32_t now_ms, uint32_t start_ms, uint32_t period_ms) {
    return (uint32_t)(now_ms - start_ms) >= period_ms;
}

/* --- Non-Blocking 4-State Button Debounce FSM --- */
typedef enum {
    BTN_STATE_IDLE             = 0U,
    BTN_STATE_DEBOUNCE_PRESS   = 1U,
    BTN_STATE_PRESSED          = 2U,
    BTN_STATE_DEBOUNCE_RELEASE = 3U
} btn_state_t;

typedef enum {
    BTN_EVT_NONE     = 0U,
    BTN_EVT_PRESSED  = 1U,
    BTN_EVT_RELEASED = 2U
} btn_event_t;

typedef struct {
    btn_state_t state;
    uint32_t    stamp_ms;
    uint32_t    debounce_ms;
} btn_debounce_t;

static void btn_init(btn_debounce_t *btn, uint32_t debounce_ms) {
    btn->state       = BTN_STATE_IDLE;
    btn->stamp_ms    = 0U;
    btn->debounce_ms = debounce_ms;
}

static btn_event_t btn_update(btn_debounce_t *btn, bool raw_high, uint32_t now_ms) {
    switch (btn->state) {
        case BTN_STATE_IDLE:
            if (raw_high) {
                btn->stamp_ms = now_ms;
                btn->state    = BTN_STATE_DEBOUNCE_PRESS;
            }
            return BTN_EVT_NONE;

        case BTN_STATE_DEBOUNCE_PRESS:
            if (!raw_high) {
                btn->state = BTN_STATE_IDLE;
            } else if (tick_elapsed(now_ms, btn->stamp_ms, btn->debounce_ms)) {
                btn->state = BTN_STATE_PRESSED;
                return BTN_EVT_PRESSED;
            }
            return BTN_EVT_NONE;

        case BTN_STATE_PRESSED:
            if (!raw_high) {
                btn->stamp_ms = now_ms;
                btn->state    = BTN_STATE_DEBOUNCE_RELEASE;
            }
            return BTN_EVT_NONE;

        case BTN_STATE_DEBOUNCE_RELEASE:
            if (raw_high) {
                btn->state = BTN_STATE_PRESSED;
            } else if (tick_elapsed(now_ms, btn->stamp_ms, btn->debounce_ms)) {
                btn->state = BTN_STATE_IDLE;
                return BTN_EVT_RELEASED;
            }
            return BTN_EVT_NONE;

        default:
            btn->state = BTN_STATE_IDLE;
            return BTN_EVT_NONE;
    }
}

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

    int16_t min16 = INT16_MIN;
    memcpy(&raw, &min16, sizeof(raw));
    ASSERT_EQ_U32(0x8000U, raw);

    /* Unsigned modular elapsed time across 32-bit DWT/SysTick rollover */
    uint32_t t_start = 0xFFFFFFF0UL;
    uint32_t t_now   = 0x00000010UL;
    uint32_t elapsed = (uint32_t)(t_now - t_start);
    ASSERT_EQ_U32(0x20U, elapsed);

    /* Unsigned 32-bit wraparound arithmetic is well-defined modulo 2^32 */
    uint32_t max_u32 = UINT32_MAX;
    ASSERT_EQ_U32(0U, max_u32 + 1U);
    ASSERT_EQ_U32(UINT32_MAX, (uint32_t)(0U - 1U));
}

static void test_integer_promotions_and_signed_unsigned(void) {
    /* Classic embedded C trap: ~uint8_t promotes to signed int (0xFFFFFFF0 = -16)! */
    uint8_t port_byte = 0x0FU;
    int     promoted_inv = ~port_byte;
    uint8_t masked_inv   = (uint8_t)(~port_byte);
    ASSERT_TRUE(promoted_inv < 0);
    ASSERT_EQ_I32(-16, promoted_inv);
    ASSERT_EQ_U32(0xF0U, masked_inv);
    ASSERT_TRUE(promoted_inv != 0xF0);
    ASSERT_TRUE(masked_inv == 0xF0U);

    /* Usual arithmetic conversions: signed -1 converts to 0xFFFFFFFFU when compared to uint32_t */
    int32_t  s_neg = -1;
    uint32_t u_pos = 1U;
    ASSERT_TRUE((uint32_t)s_neg > u_pos);
    ASSERT_TRUE(!(s_neg < (int32_t)0 ? false : ((uint32_t)s_neg < u_pos)));

    /* Safe signed-vs-unsigned comparison helper idiom */
    bool safe_lt = (s_neg < 0) || ((uint32_t)s_neg < u_pos);
    ASSERT_TRUE(safe_lt);

    /* Explicit cast to uint32_t before shifting >= 24 bits avoids signed int UB */
    uint8_t msb = 0x80U;
    uint32_t shifted = ((uint32_t)msb) << 24U;
    ASSERT_EQ_U32(0x80000000UL, shifted);
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

    /* Verify field_write masks oversized values without corrupting neighbor bits */
    uint32_t clamped = field_write(0x00000000UL, 4U, 3U, 0xFFU);
    ASSERT_EQ_U32(0x00000070UL, clamped);

    /* Configure PC13 = Input (0b00) from STM32G4 reset 0xFFFFFFFF */
    uint32_t gpioc_moder = 0xFFFFFFFFUL;
    gpioc_moder = field_write(gpioc_moder, 26U, 2U, 0x0U);
    ASSERT_EQ_U32(0x0U, field_read(gpioc_moder, 26U, 2U));
    ASSERT_EQ_U32(0xF3FFFFFFUL, gpioc_moder);

    /* Clear single bit */
    ASSERT_EQ_U32(0xF3FFFFDFUL, bit_clear(gpioc_moder, 5U));

    /* Power-of-two test idiom */
    ASSERT_TRUE((64U != 0U) && ((64U & (64U - 1U)) == 0U));
    ASSERT_TRUE(!((48U != 0U) && ((48U & (48U - 1U)) == 0U)));
}

static void test_byte_packing_and_memcpy_punning(void) {
    const uint8_t bytes[4] = { 0x12U, 0x34U, 0x56U, 0x78U };
    ASSERT_EQ_U32(0x12345678UL, pack_be32(bytes));
    ASSERT_EQ_U32(0x78563412UL, pack_le32(bytes));

    /* Verify STM32G4 initial MSP vector word 0x20018000 in little-endian flash binary */
    const uint8_t estack_bytes[4] = { 0x00U, 0x80U, 0x01U, 0x20U };
    ASSERT_EQ_U32(0x20018000UL, pack_le32(estack_bytes));

    /* Strict-aliasing safe IEEE-754 float <-> uint32_t type punning via memcpy */
    float f_one = 1.0f;
    uint32_t bits = 0U;
    memcpy(&bits, &f_one, sizeof(bits));
    ASSERT_EQ_U32(0x3F800000UL, bits);

    uint32_t neg_two_half_bits = 0xC0200000UL;
    float f_val = 0.0f;
    memcpy(&f_val, &neg_two_half_bits, sizeof(f_val));
    ASSERT_TRUE((f_val > -2.5001f) && (f_val < -2.4999f));

    uint32_t zero_bits = 0xFFFFFFFFUL;
    float f_zero = 0.0f;
    memcpy(&zero_bits, &f_zero, sizeof(zero_bits));
    ASSERT_EQ_U32(0x00000000UL, zero_bits);

    /* Unaligned stream read via memcpy (never cast misaligned uint8_t* to uint32_t*!) */
    const uint8_t wire[6] = { 0xAAU, 0x78U, 0x56U, 0x34U, 0x12U, 0x55U };
    uint32_t extracted = 0U;
    memcpy(&extracted, &wire[1], sizeof(extracted));
    ASSERT_EQ_U32(0x12345678UL, extracted);
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
    ASSERT_EQ_I32(3, (int32_t)(p3 - p0));

    /* Struct alignment and padding vs descending-alignment packing */
    ASSERT_EQ_U32(0U, offsetof(padded_packet_t, tag));
    ASSERT_EQ_U32(4U, offsetof(padded_packet_t, value));
    ASSERT_EQ_U32(8U, offsetof(padded_packet_t, crc));
    ASSERT_EQ_U32(12U, sizeof(padded_packet_t));

    ASSERT_EQ_U32(0U, offsetof(compact_packet_t, value));
    ASSERT_EQ_U32(4U, offsetof(compact_packet_t, crc));
    ASSERT_EQ_U32(6U, offsetof(compact_packet_t, tag));
    ASSERT_EQ_U32(8U, sizeof(compact_packet_t));
    ASSERT_TRUE(sizeof(compact_packet_t) < sizeof(padded_packet_t));
}

static void test_tagged_union_and_jump_table(void) {
    telemetry_msg_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.tag                = MSG_TEMP_MDEGC;
    msg.payload.temp_mdegc = 42750;
    ASSERT_EQ_I32(42, telemetry_extract_scaled(&msg));

    msg.tag                = MSG_VOLTAGE_MV;
    msg.payload.voltage_mv = 3305U;
    ASSERT_EQ_I32(3305, telemetry_extract_scaled(&msg));

    msg.tag                     = MSG_FAULT_INFO;
    msg.payload.fault.subsystem = 0x0002U;
    msg.payload.fault.code      = 0x00A5U;
    ASSERT_EQ_U32(0x000200A5UL, (uint32_t)telemetry_extract_scaled(&msg));

    msg.tag = MSG_NONE;
    ASSERT_EQ_I32(0, telemetry_extract_scaled(&msg));
    ASSERT_EQ_I32(-1, telemetry_extract_scaled(NULL));

    /* Bounds-checked function-pointer jump table */
    int32_t out = 0;
    ASSERT_TRUE(alu_dispatch(0U, 12, 8, &out));  ASSERT_EQ_I32(20, out);
    ASSERT_TRUE(alu_dispatch(1U, 12, 8, &out));  ASSERT_EQ_I32(4, out);
    ASSERT_TRUE(alu_dispatch(2U, -6, 7, &out));  ASSERT_EQ_I32(-42, out);
    ASSERT_TRUE(alu_dispatch(3U, -5, 19, &out)); ASSERT_EQ_I32(19, out);
    ASSERT_TRUE(!alu_dispatch(4U, 1, 2, &out));
    ASSERT_TRUE(!alu_dispatch(0U, 1, 2, NULL));
}

static void test_fixed_point_q15_q31_math(void) {
    /* Q15: 0.5 = 16384 (0x4000), -1.0 = -32768 (0x8000), +0.99997 = 32767 (0x7FFF) */
    ASSERT_EQ_I32(24576, q15_add_sat(16384, 8192));        /* 0.5 + 0.25 = 0.75 */
    ASSERT_EQ_I32(INT16_MAX, q15_add_sat(25000, 20000));   /* Positive saturation */
    ASSERT_EQ_I32(INT16_MIN, q15_add_sat(-25000, -20000)); /* Negative saturation */
    ASSERT_EQ_I32(INT16_MAX, q15_sub_sat(20000, -20000));  /* Sub positive saturation */
    ASSERT_EQ_I32(INT16_MIN, q15_sub_sat(-20000, 20000));  /* Sub negative saturation */
    ASSERT_EQ_I32(8192, q15_mul_sat(16384, 16384));        /* 0.5 * 0.5 = 0.25 */
    ASSERT_EQ_I32(-8192, q15_mul_sat(-16384, 16384));      /* -0.5 * 0.5 = -0.25 */
    ASSERT_EQ_I32(INT16_MAX, q15_mul_sat(INT16_MIN, INT16_MIN)); /* -1 * -1 saturates */

    /* Q31: 0.5 = 0x40000000 (1073741824) */
    const int32_t q31_half = 0x40000000;
    const int32_t q31_qtr  = 0x20000000;
    ASSERT_EQ_I32(0x60000000, q31_add_sat(q31_half, q31_qtr));
    ASSERT_EQ_I32(INT32_MAX, q31_add_sat(INT32_MAX, 100));
    ASSERT_EQ_I32(INT32_MIN, q31_add_sat(INT32_MIN, -100));
    ASSERT_EQ_I32(q31_qtr, q31_mul_sat(q31_half, q31_half));
    ASSERT_EQ_I32(INT32_MAX, q31_mul_sat(INT32_MIN, INT32_MIN));
}

static void test_wraparound_safe_tick_timer(void) {
    /* Normal progression */
    ASSERT_TRUE(!tick_elapsed(109U, 100U, 10U));
    ASSERT_TRUE(tick_elapsed(110U, 100U, 10U));
    ASSERT_TRUE(tick_elapsed(150U, 100U, 10U));

    /* Exact 32-bit wraparound crossing 0xFFFFFFFF -> 0x00000000 */
    ASSERT_TRUE(!tick_elapsed(0xFFFFFFFEUL, 0xFFFFFFF0UL, 20U)); /* 14 ms < 20 ms */
    ASSERT_TRUE(!tick_elapsed(0x00000003UL, 0xFFFFFFF0UL, 20U)); /* 19 ms < 20 ms */
    ASSERT_TRUE(tick_elapsed(0x00000004UL, 0xFFFFFFF0UL, 20U));  /* 20 ms == 20 ms */
    ASSERT_TRUE(tick_elapsed(0x00000022UL, 0xFFFFFFF0UL, 50U));  /* 50 ms == 50 ms */
    ASSERT_TRUE(tick_elapsed(0x00000000UL, 0xFFFFFFFFUL, 1U));   /* 1 ms across 0 */
}

static void test_button_debounce_fsm(void) {
    btn_debounce_t btn;
    btn_init(&btn, 20U);

    /* Idle with button released */
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, false, 100U));
    ASSERT_EQ_U32(BTN_STATE_IDLE, btn.state);

    /* Short 10 ms noise spike must be rejected without emitting press */
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, true, 110U));
    ASSERT_EQ_U32(BTN_STATE_DEBOUNCE_PRESS, btn.state);
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, true, 120U));
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, false, 125U));
    ASSERT_EQ_U32(BTN_STATE_IDLE, btn.state);

    /* Sustained press (>= 20 ms) triggers exactly one BTN_EVT_PRESSED */
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, true, 200U));
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, true, 219U));
    ASSERT_EQ_U32(BTN_EVT_PRESSED, btn_update(&btn, true, 220U));
    ASSERT_EQ_U32(BTN_STATE_PRESSED, btn.state);
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, true, 250U)); /* Held: no re-trigger */

    /* Brief contact bounce LOW (< 20 ms) while pressed returns to PRESSED */
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, false, 300U));
    ASSERT_EQ_U32(BTN_STATE_DEBOUNCE_RELEASE, btn.state);
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, true, 310U));
    ASSERT_EQ_U32(BTN_STATE_PRESSED, btn.state);

    /* Sustained release crossing 32-bit SysTick rollover emits BTN_EVT_RELEASED */
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, false, 0xFFFFFFF5UL));
    ASSERT_EQ_U32(BTN_EVT_NONE, btn_update(&btn, false, 0x00000008UL)); /* 19 ms */
    ASSERT_EQ_U32(BTN_EVT_RELEASED, btn_update(&btn, false, 0x00000009UL)); /* 20 ms */
    ASSERT_EQ_U32(BTN_STATE_IDLE, btn.state);
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
    printf("Running Stage 01-03 Embedded C Foundations Unit Tests...\n");
    test_fixed_width_and_wraparound();
    test_integer_promotions_and_signed_unsigned();
    test_bit_manipulation_and_fields();
    test_byte_packing_and_memcpy_punning();
    test_pointer_offsets_and_struct_layout();
    test_tagged_union_and_jump_table();
    test_fixed_point_q15_q31_math();
    test_wraparound_safe_tick_timer();
    test_button_debounce_fsm();
    test_static_and_qualifiers();

    printf("Results: %d assertions executed, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}
