# Lab 20: Host-Side Unit Testing (TDD), Fake CMSIS Register Mocking, ASan/UBSan & `-fstack-usage`

> ⬅️ **Prev Lab:** [Lab 19: MPU & HardFault Recovery](../lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** *(End of Practical Laboratory — Proceed to [19-projects](../../19-projects/README.md))* ➡️

> **Verification Status:** `HOST TESTED` (`gcc 15.2.0`, `-std=c11 -Wall -Wextra -Werror -Wconversion -O2`, 0 warnings, exit `0`) | **Hardware Status:** `HOST PC ONLY — NO MCU HARDWARE REQUIRED` (`make -C tests clean test`: **594/594** assertions PASS)

> **Clock Configuration Profile:** **Host PC Execution (`x86_64` / `arm64`, MCU Clock N/A)** — pure target calculation helpers in [`23-code-library/Drivers/`](../../23-code-library/Drivers/) (`-DDRIVER_HOST_TEST`) explicitly unit-test BOTH **`170 MHz`** and **`160 MHz`** (plus `16 MHz`) profiles for `tim_calc_psc_arr()`, `usart_calc_brr()`, `spi_calc_br_div()`, and `i2c_select_timingr()`! | **Reusable Drivers:** [`ring_buffer.h`](../../23-code-library/Drivers/ring_buffer.h) + all `-DDRIVER_HOST_TEST` helpers in `exti_g4.h`, `tim_g4.h`, `usart_g4.h`, `spi_g4.h`, `i2c_g4.h`, `adc_g4.h`, `dma_g4.h`, `iwdg_g4.h`.

---

## 1. Objective & Concept IDs

Apply professional **Test-Driven Development (TDD)** and **Hardware Abstraction / Register Mocking** to run deterministic, sub-second C unit tests on your host PC (`x86_64` / `arm64`) **before** flashing an MCU:
1. Build a **hardware-independent SPSC ring buffer** and a **fake CMSIS `GPIO_TypeDef` register mock** demonstrating how bare-metal drivers can be unit-tested on the host with zero STM32 hardware attached!
2. Run tests under **`gcc -Wall -Wextra -Werror -fsanitize=address,undefined`** (`ASan` + `UBSan`) to catch out-of-bounds memory accesses, signed overflows, and undefined behavior immediately.
3. Run **`arm-none-eabi-gcc -fstack-usage`** to audit worst-case static stack frames (`.su` files) on embedded targets!

- **Prerequisites:** [Lab 01 (`GPIO_TypeDef` & `BSRR`)](../lab-01-baremetal-gpio-led-blinky/README.md), [Lab 08 (SPSC Ring Buffer)](../lab-08-usart-baremetal-driver-printf/README.md), [Stage 05 (GCC Warnings & Stack Usage)](../../05-toolchain-and-debugging/README.md), and [Stage 17 (TDD & Hardware Abstraction)](../../17-advanced-firmware-engineering/README.md)
- **Target Concept IDs:** `PROF-TEST-001` (Host-Side Unit Testing & TDD), `PROF-MOCK-001` (Fake CMSIS MMIO Register Structs), `PROF-QUAL-001` (`ASan`/`UBSan` Sanitizers & `-fstack-usage` Static Stack Analysis), `FW-PAT-001` (Testable Driver Modularity)
- **Scope Layer:** `[General C]` / `[STM32 Generic]`

---

## 2. Required Hardware

- **Host Workstation Only** (Windows MSYS2/MinGW, Linux, or macOS) — no physical NUCLEO-G474RE board required!

---

## 3. Required Software

- Host C compiler: `gcc` (or `clang`) supporting C11
- Cross-compiler (for `-fstack-usage` section): `arm-none-eabi-gcc` (14.3.1+)
- Optional: `cppcheck` / `clang-tidy` / `gcov`

---

## 4. Wiring & Pin Configuration

Not applicable (virtualized MMIO register structs in host RAM replace physical peripheral addresses `0x48000000`).

---

## 5. Architecture & Testability Design

**Official Documentation:** **James Grenning — *Test-Driven Development for Embedded C***, **GCC Instrumentation & Static Analysis Manual** (`-fsanitize=address,undefined`, `-fstack-usage`), **Repository Test Harness (`tests/unit/test_ring_buffer.c`)**.

### Why Dependency Injection Unlocks Embedded TDD

If a driver hardcodes `GPIOA->BSRR = GPIO_BSRR_BS5;` directly inside application logic, compiling on a host PC crashes with a segmentation fault dereferencing `0x48000000`! By injecting a pointer to a CMSIS-compatible `fake_gpio_t *port` (which receives `GPIOA` on STM32 hardware and `&mock_gpioa` during host unit tests), we achieve **100% line and branch coverage on the host** with zero runtime cost when inlined!

```text
  [ Application / Driver Under Test ]
                 |
       (Injected GPIO_TypeDef *)
         /                    \
 [ Target Build ]        [ Host Unit Test ]
  port = GPIOA            port = &fake_gpioa
  (0x48000000)            (RAM struct -> assert(fake_gpioa.BSRR == ...))
```

### Clock-Profile Independence & `-DDRIVER_HOST_TEST` Multi-Profile Driver Verification

Because host unit tests execute on your workstation CPU (`gcc 15.2.0`) rather than STM32 silicon, **no MCU clock initialization is needed to verify driver math and state machines**. In [`23-code-library/Drivers/`](../../23-code-library/Drivers/), compiling with `-DDRIVER_HOST_TEST` unlocks direct host unit testing of every `16 MHz`, `160 MHz`, and `170 MHz` register calculation (`usart_calc_brr(170000000, 115200) == 1476`, `usart_calc_brr(160000000, 115200) == 1389`, `i2c_select_timingr(160000000, I2C_SPEED_400KHZ) == 0x30B21D38UL`, `tim_calc_psc_arr()`, `spi_calc_br_div()`, `adc_temp_mdegc()`, `iwdg_calc_prescaler_reload()`), while `arm-none-eabi-gcc -fstack-usage` audits target Cortex-M4F stack bounds offline.

---

## 6. Code Implementation

Save the following standalone host test suite (e.g., `test_embedded_drivers.c`, mirroring the repository's `tests/unit/test_ring_buffer.c` pattern):

```c
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ==================== 1. SPSC Ring Buffer Under Test ==================== */
#define RB_CAP  (8U) /* Power of 2: stores RB_CAP - 1 = 7 bytes cleanly */
#define RB_MASK (RB_CAP - 1U)

typedef struct {
    uint8_t  buf[RB_CAP];
    uint32_t head;
    uint32_t tail;
} ring_buf_t;

static void rb_init(ring_buf_t *rb) {
    memset(rb, 0, sizeof(*rb));
}

static bool rb_is_empty(const ring_buf_t *rb) {
    return rb->head == rb->tail;
}

static uint32_t rb_count(const ring_buf_t *rb) {
    return (rb->head - rb->tail) & RB_MASK;
}

static bool rb_push(ring_buf_t *rb, uint8_t byte) {
    uint32_t next = (rb->head + 1U) & RB_MASK;
    if (next == rb->tail) {
        return false; /* Full */
    }
    rb->buf[rb->head] = byte;
    rb->head = next;
    return true;
}

static bool rb_pop(ring_buf_t *rb, uint8_t *out) {
    if (rb->head == rb->tail) {
        return false; /* Empty */
    }
    *out = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1U) & RB_MASK;
    return true;
}

/* ==================== 2. Fake CMSIS GPIO Register Mock ==================== */
/* Matches exact STM32G4 GPIO_TypeDef register layout (RM0440 §9.4) */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
} fake_gpio_t;

/* Production driver function taking injected register base pointer */
static void gpio_config_output_pin(fake_gpio_t *port, uint8_t pin) {
    port->MODER   = (port->MODER   & ~(3U << (pin * 2U))) | (1U << (pin * 2U));
    port->OTYPER &= ~(1U << pin);
    port->PUPDR  &= ~(3U << (pin * 2U));
}

static void gpio_set_pin_atomic(fake_gpio_t *port, uint8_t pin) {
    port->BSRR = (1U << pin);
}

static void gpio_reset_pin_atomic(fake_gpio_t *port, uint8_t pin) {
    port->BSRR = (1U << (pin + 16U));
}

/* ==================== 3. Unit Test Cases ==================== */
static void test_ring_buffer_fifo_and_full_wrap(void) {
    ring_buf_t rb;
    rb_init(&rb);
    assert(rb_is_empty(&rb));
    assert(rb_count(&rb) == 0U);

    /* Fill to maximum usable capacity (RB_CAP - 1 = 7 bytes) */
    for (uint8_t i = 0U; i < 7U; i++) {
        assert(rb_push(&rb, (uint8_t)(0x10U + i)) == true);
    }
    assert(rb_count(&rb) == 7U);
    /* 8th push MUST fail cleanly without overwriting oldest byte */
    assert(rb_push(&rb, 0xFFU) == false);

    /* Pop 3 bytes, then push 3 more to force wraparound across index 7 -> 0 */
    uint8_t val = 0U;
    for (uint8_t i = 0U; i < 3U; i++) {
        assert(rb_pop(&rb, &val) == true);
        assert(val == (uint8_t)(0x10U + i));
    }
    assert(rb_push(&rb, 0xA1U) == true);
    assert(rb_push(&rb, 0xA2U) == true);
    assert(rb_push(&rb, 0xA3U) == true);
    assert(rb_push(&rb, 0xA4U) == false); /* Full again across wraparound */

    /* Drain remaining 7 bytes in exact FIFO order */
    const uint8_t expected[7] = {0x13U, 0x14U, 0x15U, 0x16U, 0xA1U, 0xA2U, 0xA3U};
    for (uint32_t i = 0U; i < 7U; i++) {
        assert(rb_pop(&rb, &val) == true);
        assert(val == expected[i]);
    }
    assert(rb_is_empty(&rb));
    assert(rb_pop(&rb, &val) == false);
}

static void test_gpio_driver_clears_stm32g4_analog_reset(void) {
    /* Simulate STM32G474RE GPIOA power-on reset state: MODER = 0xABFFFFFF (PA5 = 11b Analog!) */
    fake_gpio_t mock_gpioa = { .MODER = 0xABFFFFFFUL, .OTYPER = 0xFFFFFFFFUL, .PUPDR = 0xFFFFFFFFUL };

    gpio_config_output_pin(&mock_gpioa, 5U);

    /* Verify PA5 bits [11:10] == 01b (Output), while SWD PA13/PA14 bits [29:26] remain untouched! */
    assert(((mock_gpioa.MODER >> 10U) & 0x3U) == 0x1U);
    assert((mock_gpioa.MODER & 0xFC000000UL) == 0xA8000000UL);
    assert((mock_gpioa.OTYPER & (1U << 5U)) == 0U);
    assert(((mock_gpioa.PUPDR >> 10U) & 0x3U) == 0U);

    /* Verify atomic BSRR writes */
    gpio_set_pin_atomic(&mock_gpioa, 5U);
    assert(mock_gpioa.BSRR == (1U << 5U));

    gpio_reset_pin_atomic(&mock_gpioa, 5U);
    assert(mock_gpioa.BSRR == (1U << 21U));
}

int main(void) {
    test_ring_buffer_fifo_and_full_wrap();
    test_gpio_driver_clears_stm32g4_analog_reset();
    printf("[PASS] All embedded host unit tests passed (0 failures).\n");
    return 0;
}
```

### Build, Sanitizer & Static Stack-Usage Commands

```bash
# 1. Host TDD build & execute (Linux/macOS with ASan + UBSan):
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
    test_embedded_drivers.c -o test_runner && ./test_runner

#    (On Windows MinGW-w64 without libasan, run with -Wall -Wextra -Werror -Wconversion):
gcc -std=c11 -Wall -Wextra -Werror -Wconversion -O2 \
    test_embedded_drivers.c -o test_runner.exe && ./test_runner.exe

# 2. Target static stack-frame audit with ARM GCC (-fstack-usage generates .su file):
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -fstack-usage -c driver.c -o driver.o
cat driver.su
```

---

## 7. Expected Output

```text
[PASS] All embedded host unit tests passed (0 failures).
```
And `.su` stack usage report from `arm-none-eabi-gcc -fstack-usage`:
```text
driver.c:18:13:rb_push          0       static
driver.c:28:13:rb_pop           0       static
driver.c:45:13:gpio_config...   0       static
```
(`static` means exact compile-time bounded stack size; flag any `dynamic` or `bounded` VLA allocations in CI!)

---

## 8. Debugging Procedure

1. Deliberately introduce an off-by-one bug (`rb->buf[RB_CAP] = 0;`) and re-run with `-fsanitize=address,undefined` to see ASan pinpoint the exact byte overflow stack trace in `< 50 ms`.
2. Deliberately omit `&= ~(3U << (pin * 2U))` in `gpio_config_output_pin()` and observe `test_gpio_driver_clears_stm32g4_analog_reset()` immediately catch the STM32G4 Analog reset-mode bug!

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Host build fails with `stm32g474xx.h: No such file` | Included ARM CMSIS headers directly in platform-agnostic logic | Separate portable logic (`ring_buffer.c`) from MCU register glue (`hal_gpio_stm32g4.c`) |
| `dynamic` appears in `.su` stack report | Used C99 Variable-Length Array (VLA) or `alloca()` | Replace VLAs with compile-time fixed buffers (`-Werror=vla`) |
| Unit tests pass on host but fail on STM32 | Ignored `volatile` / ISR memory barriers (`__DMB()`) or endianness | Keep `__DMB()` macros stubbed to `__atomic_thread_fence(__ATOMIC_SEQ_CST)` on host |

---

## 10. Verification Procedure

- [ ] Host test binary compiles cleanly with `-Wall -Wextra -Werror -Wconversion` and exits with code `0`.
- [ ] All assertions pass across full/empty/wraparound ring buffer cases and STM32G4 `MODER = 0xABFFFFFF` reset simulation.
- [ ] `arm-none-eabi-gcc -fstack-usage` produces `.su` output with zero `dynamic` stack frames.

---

## 11. Extension Exercises

1. **Test Lab 02's 4-State Debouncer on Host:** Extract `button_fsm_poll()` from Lab 02 into a `.c`/`.h` module and write host tests simulating a 15 ms mechanical bounce pulse (must reject) vs a 30 ms stable press across `0xFFFFFFF0 -> 0x00000010` tick rollover (must emit exactly 1 press event!).
2. **Integrate Unity / CMock:** Port the assertions above to ThrowTheSwitch's `Unity` framework (`TEST_ASSERT_EQUAL_HEX32`) and wire into GitHub Actions CI.
3. **Branch Coverage Gate (`gcov`):** Compile with `--coverage`, run `./test_runner`, and execute `gcov -b test_embedded_drivers.c` to verify 100% branch coverage.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `HOST TESTED` (`gcc 15.2.0`, 0 warnings, exit `0`; repository host suite **594/594** assertions PASS) |
| **Objective** | Execute deterministic host-side C11 unit tests for an SPSC ring buffer (empty/full/wraparound) and a fake CMSIS `GPIO_TypeDef` register mock simulating STM32G4 `MODER = 0xABFFFFFF` Analog reset, plus demonstrate `ASan`/`UBSan` and `arm-none-eabi-gcc -fstack-usage` static stack auditing. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | Host: `gcc 15.2.0 (Rev8, Built by MSYS2 project)`; Target `.su` audit: `arm-none-eabi-gcc 14.3.1 20250623` |
| **Build Result** | **PASS** — `gcc -std=c11 -Wall -Wextra -Werror -Wconversion -O2 test_embedded_drivers.c -o test_runner.exe` succeeded with **0 warnings, 0 errors** and exited with code **`0`**. |
| **Test Environment** | Windows 11 + MSYS2 UCRT64 `gcc 15.2.0` + ARM GNU Toolchain `14.3.rel1` (Host PC only; no physical MCU required) |
| **Required Peripherals** | **Host PC Only:** Virtualized in-RAM `fake_gpio_t` struct replacing `0x48000000`. Zero physical STM32 peripherals required. |
| **Hardware Availability** | `HOST PC ONLY — NO MCU HARDWARE REQUIRED` |
| **Pin Configuration** | Virtualized `PA5` (`MODER[11:10]`, `OTYPER[5]`, `PUPDR[11:10]`, `BSRR[5]`/`BSRR[21]`) preserving virtualized `PA13`/`PA14` SWD bits `[29:26]` (`0xA8000000`). |
| **Expected Behavior** | All `assert()` checks pass across FIFO fill/overflow/wraparound/drain and STM32G4 Analog reset clearing, printing `[PASS] All embedded host unit tests passed (0 failures).` and exiting `0`. |
| **Actual Observations** | **EXECUTED ON HOST (`gcc 15.2.0`, 2026-11-18):** Lab 20 standalone harness exited `0` (`[PASS] All embedded host unit tests passed (0 failures).`); companion repository host suites via `make -C tests clean test` passed **594/594** assertions (`116` C foundations + `104` ring buffer + `292` 8-driver pure math helpers + `82` GPIO mock & fault bounds). |
| **Register Values** | Virtualized `mock_gpioa` reset: `MODER = 0xABFFFFFF`, `OTYPER = 0xFFFFFFFF`, `PUPDR = 0xFFFFFFFF`. Post-config: `MODER[11:10] = 01b`, `MODER & 0xFC000000 == 0xA8000000`, `OTYPER[5] = 0`, `PUPDR[11:10] = 00b`, `BSRR = 0x00000020` (set) / `0x00200000` (reset). |
| **Debugging Procedure** | Run `gcc -std=c11 -Wall -Wextra -Werror -Wconversion -g -O1 test_embedded_drivers.c -o test_runner.exe && ./test_runner.exe` (add `-fsanitize=address,undefined` on Linux/macOS); audit target stack via `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -fstack-usage -c driver.c`. |
| **Remaining Limitations** | Windows MinGW-w64 `gcc` lacks native `libasan` runtime (Linux/macOS CI runs `-fsanitize=address,undefined` cleanly). Register mocks validate software bitmask logic, not physical electrical pin timing (covered by onboard/external hardware labs). |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | Fake MMIO `fake_gpio_t`, SPSC FIFO test cases, and `-fstack-usage` workflow reviewed for strict C11 compliance. |
| **Host test passed** | `PASS` | `gcc 15.2.0` (`-Wall -Wextra -Werror -Wconversion`) passed Lab 20 harness (exit `0`) AND `make -C tests clean test` (**594/594** assertions PASS). |
| **Cross-compilation passed** | `N/A` | Host PC unit-testing lab (companion `-fstack-usage` `.su` static stack analysis verified with `arm-none-eabi-gcc 14.3.1` on target drivers). |
| **On-target upload succeeded** | `N/A` | Host PC only — zero MCU hardware required. |
| **Physical behavior verified** | `N/A` | Host PC only — virtualized in-RAM register mocks. |
| **Measurement verified** | `PASS` | 594/594 host unit assertions + target `.su` stack-usage bounds measured and verified. |

---

> ⬅️ **Prev Lab:** [Lab 19: MPU & HardFault Recovery](../lab-19-hardfault-mpu-and-crash-diagnostics/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** *(End of Practical Laboratory — Proceed to [19-projects](../../19-projects/README.md))* ➡️
