# Lab 17: STM32G474RE Hardware Math Coprocessors — CORDIC & FMAC Benchmarked via `DWT->CYCCNT`

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Harness the two signature **STM32G474RE hardware math accelerators** on AHB1 (`170 MHz`):
1. **`CORDIC`** (`0x40020C00`): Compute `cos(θ)` + `sin(θ)` simultaneously in `q1.31` fixed-point with **zero-wait-state AHB stall polling** (~6 cycles/pair) and benchmark cycle count against Cortex-M4F FPU `cosf()`/`sinf()` using **`DWT->CYCCNT`**.
2. **`FMAC`** (`0x40021400`): Execute an 8-tap `q1.15` **FIR low-pass filter** in hardware (`1 MAC/cycle` = `170 MMAC/s`) using its 256×16-bit internal SRAM (`X1`, `X2`, `Y` circular buffers) and verify bit-exact accuracy against a software reference loop!

- **Target Concept IDs:** `PERIPH-CORDIC-001` (`CORDIC` `q1.31` Trig Acceleration), `PERIPH-FMAC-001` (`FMAC` `q1.15` FIR/IIR Hardware DSP), `ARM-DWT-001` (`DWT->CYCCNT` Cycle Benchmarking), `C-FIXPT-001` (`q1.31` / `q1.15` Fixed-Point Scaling)
- **Scope Layer:** `[STM32G474RE Exact]` / `[Cortex-M4 Specific]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C) — 100% on-chip experiment (no external wiring needed!)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+) with `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -O2 -lm`
- `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

No external pins required. Optional: stream benchmark metrics over `USART2` (`PA2`/`PA3`, Lab 08).

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 17** (*CORDIC*), **RM0440 Chapter 18** (*FMAC*), **AN5325** (*How to use math accelerators on STM32G4*), **ARMv7-M Architecture Reference Manual** (`DWT->CYCCNT`).

### 5.1 CORDIC `q1.31` Angle Mapping

CORDIC expects angles normalized by $\pi$: $\hat{\theta} = \frac{\theta}{\pi} \in [-1, +1)$ represented as a signed 32-bit `q1.31` integer:

$$\text{angle\_q31} = \left\lfloor \frac{\theta}{\pi} \times 2^{31} \right\rfloor \quad \text{(e.g., } \theta = \frac{\pi}{3} \implies \text{angle\_q31} = \frac{2^{31}}{3} = 715\,827\,882\text{)}$$

- **`CORDIC->CSR`**: `FUNC = 0` (Cosine primary + Sine secondary), `PRECISION = 6` (`24` iterations = `6` clock cycles, $2^{-19}$ accuracy!), `NRES = 1` (read 2 results: `cos` then `sin`), `NARGS = 0` (1 input argument), `ARGSIZE = 0` (`32-bit`), `RESSIZE = 0` (`32-bit`).
- **Zero-Wait-State Polling:** Writing `CORDIC->WDATA` starts computation immediately; reading `CORDIC->RDATA` automatically inserts AHB wait states until `RRDY == 1`, eliminating polling-loop overhead!

### 5.2 FMAC 256×16-Bit SRAM Partitioning (8-Tap FIR Filter)

For an $N = 8$-tap FIR filter ($y[n] = \sum_{k=0}^{7} b_k \cdot x[n-k]$) in `q1.15` (`int16_t` scaled by $2^{15} = 32768$):
- **`X2` (Coefficients `b[0..7]`):** `X2_BASE = 0`, `X2_BUF_SIZE = 8` (words `0..7`).
- **`X1` (Input Delay Line):** `X1_BASE = 8`, `X1_BUF_SIZE = 16` (must be $\ge N$; words `8..23`).
- **`Y` (Output Buffer):** `Y_BASE = 24`, `Y_BUF_SIZE = 16` (words `24..39`).
- **FMAC `PARAM` Function Codes (RM0440 §18.4.4):** `FUNC = 2` (`LOAD_X2` coefficients), `FUNC = 8` (`CONVO_FIR`, with `P = 8` taps, `Q = 0`, `R = 0` post-shift, `START = 1`).

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <math.h>
#include <stdint.h>

extern uint32_t SystemCoreClock;

#define BENCH_N_ANGLES   (256U)
#define FIR_TAPS         (8U)
#define FIR_SAMPLES      (32U)

/* Benchmark & Verification outputs inspectable directly in GDB */
static volatile uint32_t g_cordic_cycles     = 0U;
static volatile uint32_t g_fpu_cycles        = 0U;
static volatile float    g_cordic_max_err    = 0.0f;
static volatile uint32_t g_fmac_cycles       = 0U;
static volatile uint32_t g_sw_fir_cycles     = 0U;
static volatile int32_t  g_fmac_max_lsb_diff = 0;

static void Clock_Init_170MHz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) {}
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS             |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) {}

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {}

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI              |
                   ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos) |
                   (85U       << RCC_PLLCFGR_PLLN_Pos) |
                   (0U        << RCC_PLLCFGR_PLLR_Pos) |
                   RCC_PLLCFGR_PLLREN;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {}

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    for (volatile uint32_t i = 0U; i < 120U; i++) { __NOP(); }
    RCC->CFGR &= ~RCC_CFGR_HPRE;
    SystemCoreClock = 170000000UL;
}

static void DWT_CycleCounter_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT       = 0U;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;
}

/* ==================== PART 1: CORDIC vs Cortex-M4F FPU ==================== */
static void Benchmark_CORDIC_vs_FPU(void) {
    static int32_t angles_q31[BENCH_N_ANGLES];
    static float   angles_rad[BENCH_N_ANGLES];
    static int32_t cordic_cos[BENCH_N_ANGLES];
    static int32_t cordic_sin[BENCH_N_ANGLES];
    static float   fpu_cos[BENCH_N_ANGLES];
    static float   fpu_sin[BENCH_N_ANGLES];

    const float pi = 3.14159265358979f;
    for (uint32_t i = 0U; i < BENCH_N_ANGLES; i++) {
        float norm = -0.95f + (1.90f * (float)i) / (float)BENCH_N_ANGLES;
        angles_rad[i] = norm * pi;
        angles_q31[i] = (int32_t)(norm * 2147483648.0f);
    }

    /* Enable CORDIC on AHB1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;
    volatile uint32_t tmpreg = RCC->AHB1ENR;
    (void)tmpreg;

    /* FUNC=0 (Cos+Sin), PRECISION=6 (24 iter = 6 cycles), NRES=1 (2 outputs), 32-bit q1.31 */
    CORDIC->CSR = (0U << CORDIC_CSR_FUNC_Pos)      |
                  (6U << CORDIC_CSR_PRECISION_Pos) |
                  CORDIC_CSR_NRES;

    uint32_t t0 = DWT->CYCCNT;
    for (uint32_t i = 0U; i < BENCH_N_ANGLES; i++) {
        CORDIC->WDATA = (uint32_t)angles_q31[i];
        /* Zero-wait-state AHB stall read: hardware stalls bus until result ready */
        cordic_cos[i] = (int32_t)CORDIC->RDATA;
        cordic_sin[i] = (int32_t)CORDIC->RDATA;
    }
    g_cordic_cycles = DWT->CYCCNT - t0;

    /* Benchmark software libm FPU cosf() + sinf() */
    t0 = DWT->CYCCNT;
    for (uint32_t i = 0U; i < BENCH_N_ANGLES; i++) {
        fpu_cos[i] = cosf(angles_rad[i]);
        fpu_sin[i] = sinf(angles_rad[i]);
    }
    g_fpu_cycles = DWT->CYCCNT - t0;

    /* Verify numerical accuracy against FPU reference */
    float max_err = 0.0f;
    const float inv_q31 = 1.0f / 2147483648.0f;
    for (uint32_t i = 0U; i < BENCH_N_ANGLES; i++) {
        float ec = fabsf(((float)cordic_cos[i] * inv_q31) - fpu_cos[i]);
        float es = fabsf(((float)cordic_sin[i] * inv_q31) - fpu_sin[i]);
        if (ec > max_err) { max_err = ec; }
        if (es > max_err) { max_err = es; }
    }
    g_cordic_max_err = max_err;
}

/* ==================== PART 2: FMAC 8-Tap FIR vs Software C ==================== */
static void Benchmark_FMAC_vs_SW_FIR(void) {
    /* 8-tap boxcar moving-average low-pass filter: b_k = 1/8 = 0.125 -> 4096 in q1.15 */
    static const int16_t coeffs_q15[FIR_TAPS] = {
        4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096
    };
    static int16_t input_q15[FIR_SAMPLES];
    static int16_t fmac_out[FIR_SAMPLES];
    static int16_t sw_out[FIR_SAMPLES];

    /* Step + alternating high-frequency ripple signal */
    for (uint32_t i = 0U; i < FIR_SAMPLES; i++) {
        input_q15[i] = (i < 8U) ? 0 : ((i & 1U) ? 20000 : 12000);
    }

    /* Enable and reset FMAC on AHB1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_FMACEN;
    volatile uint32_t tmpreg = RCC->AHB1ENR;
    (void)tmpreg;

    FMAC->CR |= FMAC_CR_RESET;
    while ((FMAC->CR & FMAC_CR_RESET) != 0U) {}

    /* Partition 256-word internal SRAM: X2=[0..7], X1=[8..23], Y=[24..39] */
    FMAC->X2BUFCFG = (0U  << FMAC_X2BUFCFG_X2_BASE_Pos) | (FIR_TAPS << FMAC_X2BUFCFG_X2_BUF_SIZE_Pos);
    FMAC->X1BUFCFG = (8U  << FMAC_X1BUFCFG_X1_BASE_Pos) | (16U      << FMAC_X1BUFCFG_X1_BUF_SIZE_Pos);
    FMAC->YBUFCFG  = (24U << FMAC_YBUFCFG_Y_BASE_Pos)   | (16U      << FMAC_YBUFCFG_Y_BUF_SIZE_Pos);

    /* 1. Preload X2 coefficients (FUNC = 2 = LOAD_X2, P = FIR_TAPS) */
    FMAC->PARAM = (FIR_TAPS << FMAC_PARAM_P_Pos) | (2U << FMAC_PARAM_FUNC_Pos) | FMAC_PARAM_START;
    for (uint32_t k = 0U; k < FIR_TAPS; k++) {
        FMAC->WDATA = (uint32_t)(uint16_t)coeffs_q15[k];
    }
    while ((FMAC->PARAM & FMAC_PARAM_START) != 0U) {}

    /* 2. Zero-preload X1 delay line (FUNC = 1 = LOAD_X1, P = FIR_TAPS - 1) */
    FMAC->PARAM = ((FIR_TAPS - 1U) << FMAC_PARAM_P_Pos) | (1U << FMAC_PARAM_FUNC_Pos) | FMAC_PARAM_START;
    for (uint32_t k = 0U; k < (FIR_TAPS - 1U); k++) {
        FMAC->WDATA = 0U;
    }
    while ((FMAC->PARAM & FMAC_PARAM_START) != 0U) {}

    /* 3. Enable saturation clipping and launch continuous FIR convolution (FUNC = 8) */
    FMAC->CR    = FMAC_CR_CLIPEN;
    FMAC->PARAM = (FIR_TAPS << FMAC_PARAM_P_Pos) |
                  (0U       << FMAC_PARAM_Q_Pos) |
                  (0U       << FMAC_PARAM_R_Pos) |
                  (8U       << FMAC_PARAM_FUNC_Pos) |
                  FMAC_PARAM_START;

    /* Stream samples through FMAC hardware */
    uint32_t t0 = DWT->CYCCNT;
    for (uint32_t n = 0U; n < FIR_SAMPLES; n++) {
        while ((FMAC->SR & FMAC_SR_X1FULL) != 0U) {}
        FMAC->WDATA = (uint32_t)(uint16_t)input_q15[n];
        while ((FMAC->SR & FMAC_SR_YEMPTY) != 0U) {}
        fmac_out[n] = (int16_t)(FMAC->RDATA & 0xFFFFU);
    }
    g_fmac_cycles = DWT->CYCCNT - t0;

    /* Reference Software FIR */
    t0 = DWT->CYCCNT;
    for (uint32_t n = 0U; n < FIR_SAMPLES; n++) {
        int32_t acc = 0;
        for (uint32_t k = 0U; k < FIR_TAPS; k++) {
            int16_t x = (n >= k) ? input_q15[n - k] : 0;
            acc += (int32_t)coeffs_q15[k] * (int32_t)x;
        }
        sw_out[n] = (int16_t)(acc >> 15);
    }
    g_sw_fir_cycles = DWT->CYCCNT - t0;

    /* Compare hardware vs software outputs */
    int32_t max_diff = 0;
    for (uint32_t n = 0U; n < FIR_SAMPLES; n++) {
        int32_t d = (int32_t)fmac_out[n] - (int32_t)sw_out[n];
        if (d < 0) { d = -d; }
        if (d > max_diff) { max_diff = d; }
    }
    g_fmac_max_lsb_diff = max_diff;
}

int main(void) {
    /* Ensure Cortex-M4F FPU (CP10/CP11) is enabled before calling cosf/sinf */
    SCB->CPACR |= (0xFUL << 20U);
    __DSB();
    __ISB();

    Clock_Init_170MHz();
    DWT_CycleCounter_Init();

    Benchmark_CORDIC_vs_FPU();
    Benchmark_FMAC_vs_SW_FIR();

    while (1) {
        __WFI();
    }
}
```

---

## 7. Expected Output

Inspecting benchmark globals in GDB after completion:
- **`g_cordic_cycles`** vs **`g_fpu_cycles`**: CORDIC computes 256 `(cos, sin)` pairs ~**5×–10× faster** than `libm` `cosf()`/`sinf()` (~`10–15` cycles/pair vs `> 90` cycles/pair!).
- **`g_cordic_max_err`**: `< 1e-5` (`~2e-6`, 20-bit accuracy).
- **`g_fmac_max_lsb_diff`**: **`0` or `1` LSB** (rounding difference only), with `(FMAC->SR & (FMAC_SR_OVFL | FMAC_SR_UNFL | FMAC_SR_SAT)) == 0`.

---

## 8. Debugging Procedure

1. Place a breakpoint on `while (1)` in `main()`.
2. Run in GDB:
   ```gdb
   (gdb) p g_cordic_cycles
   (gdb) p g_fpu_cycles
   (gdb) p g_cordic_max_err
   (gdb) p g_fmac_cycles
   (gdb) p g_sw_fir_cycles
   (gdb) p g_fmac_max_lsb_diff
   (gdb) p/x FMAC->SR
   ```

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| UsageFault (`NOCP`) on first `float` instruction | Forgot to enable FPU coprocessors CP10/CP11 in `SCB->CPACR` | Set `SCB->CPACR \|= (0xFUL << 20U); __DSB(); __ISB();` |
| `DWT->CYCCNT` always reads `0` | Forgot `CoreDebug->DEMCR \|= CoreDebug_DEMCR_TRCENA_Msk;` before `DWT->CTRL` | Enable `TRCENA` first |
| `FMAC` returns garbage or stalls on `YEMPTY` | Overlapping `X1`/`X2`/`Y` SRAM ranges or skipped `LOAD_X1` initial delay-line preload | Ensure disjoint ranges inside `0..255` and preload `P - 1` zeros into `X1` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wall -Wextra -Werror`.
- [ ] `g_cordic_cycles < g_fpu_cycles` and `g_cordic_max_err < 1e-5f`.
- [ ] `g_fmac_max_lsb_diff <= 1` and `FMAC->SR` has `OVFL=0, UNFL=0, SAT=0`.

---

## 11. Extension Exercises

1. **Zero-CPU DMA Streaming into FMAC:** Wire `DMAMUX1` ID `111` (`FMAC_WRITE`) and ID `110` (`FMAC_READ`) to run a 64-tap FIR filter at `500 kSPS` with near-0% CPU utilization!
2. **3p3z IIR Digital Power Compensator:** Switch `FMAC->PARAM` to `FUNC = 9` (`IIR_DIRECT_FORM_1`, `P=4, Q=3`) to implement a hardware closed-loop buck converter compensator (ST AN5305).
3. **CORDIC `atan2` & `hypot` (`FUNC = 2` Phase / `FUNC = 3` Modulus):** Compute polar coordinates `(r, θ)` for Field-Oriented Control (FOC) Clarke/Park transforms.
