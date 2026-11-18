# Six End-to-End Worked Examples (STM32G474RE Bare-Metal)

These 6 guided walkthroughs bridge the exact gap where beginners most often stall — connecting C syntax to real STM32G474RE registers and live GDB verification. Every example follows the same **6-step engineering progression**:

> **1. Small Demo → 2. Explanation → 3. Register-Level Implementation → 4. GDB Debugging Walkthrough → 5. Full Lab Link → 6. Independent Exercise**

> **Verification Honesty:** Instructional code excerpts below are **Statically Reviewed (Not Tested on Physical Hardware)** against `stm32g474xx.h`, `core_cm4.h`, `RM0440`, `DS12288`, `PM0214`, `UM2505`, and `ES0430`. The full production equivalents in [`23-code-library/`](../23-code-library/README.md) and [`18-practical-laboratory/`](../18-practical-laboratory/README.md) (`Labs 01–14, 16–19`) are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`)**.

---

## Worked Example 1: Pointers, `volatile`, and Atomic `GPIOA->BSRR` LED Toggle

**Concept IDs:** `C-PTR-001`, `EMB-MMIO-001`, `EMB-VOL-001`, `PERIPH-GPIO-001`

### 1. Small Demo (Goal)
Turn the NUCLEO-G474RE onboard green LED `LD2` (`PA5`) ON and OFF safely using only pointers to hardware memory — without corrupting any other `GPIOA` pin if an interrupt fires mid-update.

### 2. Explanation (Why `volatile` and `BSRR` Matter)
- A peripheral register is just a fixed 32-bit address in memory (`GPIOA_BASE = 0x48000000UL`; `GPIOA->BSRR` is at offset `0x18` → `0x48000018UL`).
- Without **`volatile`** (CMSIS `__IO`), GCC at `-O2` assumes no external hardware observes writes to that address and can **delete** repeated writes (`*bsrr = ...; *bsrr = ...;`) or hoist reads out of polling loops!
- Using **`GPIOA->ODR ^= (1U << 5U)`** compiles to **3 instructions** (`LDR` → `EOR` → `STR`). If an ISR modifies `PA6` between `LDR` and `STR`, main-loop `STR` **overwrites and loses** the ISR's change (Read-Modify-Write race).
- **`GPIOA->BSRR`** (`0x48000018`, Write-Only) performs an **atomic single-cycle** set (bits `[15:0]`) or reset (bits `[31:16]`) in hardware with zero RMW race.

### 3. Register-Level Implementation *(Statically Reviewed — Not Tested on Physical Hardware)*
```c
#include <stdint.h>
#include "stm32g474xx.h"

/* Raw pointer view (what CMSIS GPIOA->BSRR expands to under the hood): */
#define RAW_GPIOA_BSRR  (*(volatile uint32_t *)(0x48000000UL + 0x18UL))

void led_pa5_init(void) {
    /* 1. Enable GPIOA clock on AHB2 + dummy readback for clock sync */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* 2. Configure Output Type (Push-Pull), Speed (Low), Pull (None) BEFORE MODER */
    GPIOA->OTYPER  &= ~(1U << 5U);
    GPIOA->OSPEEDR &= ~(3U << (5U * 2U));
    GPIOA->PUPDR   &= ~(3U << (5U * 2U));

    /* 3. Clear STM32G4 reset Analog mode (11b) and set Output mode (01b) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (5U * 2U))) | (1U << (5U * 2U));
}

void led_pa5_toggle_atomic(void) {
    /* Check current ODR latch; drive opposite state via single-cycle BSRR write */
    if ((GPIOA->ODR & (1U << 5U)) != 0U) {
        GPIOA->BSRR = GPIO_BSRR_BR5;   /* Bit 21: atomic reset PA5 LOW  */
    } else {
        GPIOA->BSRR = GPIO_BSRR_BS5;   /* Bit  5: atomic set   PA5 HIGH */
    }
}
```

### 4. GDB Debugging Walkthrough
```gdb
(gdb) break led_pa5_toggle_atomic
(gdb) continue
(gdb) x/1wx 0x48000000                 # Inspect GPIOA->MODER: bits [11:10] == 01b (0xABFFF7FF -> Output)
(gdb) x/1wx 0x48000014                 # Inspect GPIOA->ODR before BSRR write (bit 5 == 0)
(gdb) step                             # Execute GPIOA->BSRR = GPIO_BSRR_BS5 (0x00000020)
(gdb) x/1wx 0x48000014                 # Confirm bit 5 of GPIOA->ODR flipped to 1 (0x00000020)!
(gdb) disassemble /m led_pa5_toggle_atomic  # Verify single STR instruction to BSRR (no LDR/ORR/STR)
```

### 5. Full Lab Link
- Complete 3-way (Bare-Metal Register / ST LL / ST HAL) buildable experiment: **[`Lab 01: Bare-Metal GPIO LED Blinky`](../18-practical-laboratory/lab-01-baremetal-gpio-led-blinky/README.md)** and reusable driver [`23-code-library/Drivers/gpio_g4.c`](../23-code-library/Drivers/gpio_g4.c).

### 6. Independent Exercise
Modify `led_pa5_toggle_atomic()` to use `GPIOA->BRR = GPIO_BRR_BR5;` (offset `0x28`) for clearing `PA5` instead of `BSRR` bit 21. Compile with `make BUILD_TYPE=release` and inspect `build/firmware.lss` (`make disasm`) to see whether GCC saves a Thumb-2 constant-load instruction (hint: `0x00000020` fits in an 8-bit immediate, just like `0x00200000`!).

---

## Worked Example 2: Reading Active-HIGH `PC13` (Clearing STM32G4 Analog Reset `MODER=11b`)

**Concept IDs:** `PERIPH-GPIO-001`, `PERIPH-GPIO-004`, `HWSW-PCB-001`

### 1. Small Demo (Goal)
Read the blue User Button `B1` (`PC13`) on NUCLEO-G474RE so `LD2` (`PA5`) lights up **only while `B1` is pressed**.

### 2. Explanation (The Two Classic STM32G474RE Traps!)
1. **STM32G4 Reset Trap (`MODER = 11b`):** Unlike older STM32F4 chips that reset GPIOs to Input (`00b`), **STM32G4 resets `GPIOC->MODER` to `0xFFFFFFFF` (`11b` Analog Mode)** to prevent floating-input power leakage. In Analog Mode, the digital Schmitt trigger is **disconnected**, so `GPIOC->IDR` bit 13 **always reads `0`**, even when `3.3 V` is on the pin! You **must** explicitly clear bits `[27:26]` to `00b`.
2. **NUCLEO-G474RE Polarity Trap (`UM2505 §6.6`):** On MB1367 Rev C, `B1` (`PC13`) has an **external pull-down resistor** and connects to `VDD` (`3.3 V`) when pressed (**Active-HIGH**). Leave `PUPDR` at `00b` (no internal pull-up, which would fight the external pull-down!).

### 3. Register-Level Implementation *(Statically Reviewed — Not Tested on Physical Hardware)*
```c
#include <stdbool.h>
#include "stm32g474xx.h"

void button_pc13_init(void) {
    /* 1. Enable GPIOC clock on AHB2 + dummy readback */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;

    /* 2. CRITICAL: Clear STM32G4 reset Analog mode (11b) -> Input mode (00b) on PC13 */
    GPIOC->MODER &= ~(3U << (13U * 2U));

    /* 3. No internal pull-up/pull-down (00b): MB1367 already provides external pull-down */
    GPIOC->PUPDR &= ~(3U << (13U * 2U));
}

bool button_pc13_is_pressed(void) {
    /* Active-HIGH per UM2505: pressed == 1 (3.3 V), released == 0 (0 V) */
    return ((GPIOC->IDR & (1U << 13U)) != 0U);
}
```

### 4. GDB Debugging Walkthrough
```gdb
(gdb) break button_pc13_init
(gdb) continue
(gdb) x/1wx 0x48000800                 # BEFORE clearing: GPIOC->MODER == 0xFFFFFFFF (Analog!)
(gdb) finish
(gdb) x/1wx 0x48000800                 # AFTER clearing:  GPIOC->MODER == 0xF3FFFFFF (bits [27:26] == 00b)
(gdb) x/1wx 0x48000810                 # Read GPIOC->IDR while B1 released -> bit 13 == 0
# Hold down blue B1 button on NUCLEO-G474RE, then run:
(gdb) x/1wx 0x48000810                 # Read GPIOC->IDR while B1 pressed  -> bit 13 == 1 (0x00002000)
```

### 5. Full Lab Link
- Complete 4-state non-blocking SysTick FSM debouncer: **[`Lab 02: Non-Blocking GPIO Pushbutton Debouncing`](../18-practical-laboratory/lab-02-gpio-pushbutton-debouncing/README.md)**.

### 6. Independent Exercise
Temporarily comment out the `GPIOC->MODER &= ~(3U << 26U);` line, halt in GDB while holding `B1`, and observe `x/1wx 0x48000810`. Record what `GPIOC->IDR` reads when the Schmitt trigger is disabled, then restore the line and implement a rising-edge software latch (`prev == false && curr == true`) that toggles `PA5` once per press.

---

## Worked Example 3: 170 MHz & 160 MHz PLL Boost Mode (`PWR->CR5@0x80`, `VOSF`, `HPRE=/2`)

**Concept IDs:** `STM32-RCC-001`, `STM32-RCC-002`, `CLK-PLL-001`, `STM32-PWR-001`, `STM32-FLASH-001`

### 1. Small Demo (Goal)
Safely accelerate STM32G474RE from its reset **16 MHz HSI16** clock to **170 MHz** (maximum frequency) or **160 MHz** (clean integer divider for 40 MHz ADC/SPI and $\le 160\text{ MHz}$ DAC boundary) without triggering a silicon brown-out or Flash fetch fault.

### 2. Explanation (Why Skipping Any Step Crashes the MCU)
Per **RM0440 §6.1.5** and **Table 9**, any `SYSCLK > 150 MHz` (both **160 MHz** and **170 MHz**!) requires **four mandatory prerequisites in exact order**:
1. **Voltage Scaling Ready:** Enable `PWREN`, select Range 1 (`PWR->CR1.VOS = 01b`), and wait until **`PWR->SR2.VOSF == 0`** (`0x40007014`, bit 10 — *NOT `PWR_SR1`!*).
2. **Range 1 Boost Mode:** Clear **`PWR->CR5.R1MODE = 0`** (offset **`0x80`** → `0x40007080`, bit 8 — *NOT offset `0x10`!*).
3. **Flash Wait States:** Program **`FLASH->ACR.LATENCY = 4WS`** (`0100b`, required for `136–170 MHz` in Boost Mode) + ART `PRFTEN`/`ICEN`/`DCEN`, and read back `LATENCY == 4WS`.
4. **RM0440 §6.1.5 Two-Step Ramp:** Set **`RCC->CFGR.HPRE = /2`** (85 MHz or 80 MHz intermediate HCLK) → switch `SW = PLL` → wait `SWS == PLL` → **wait $\ge 1\ \mu\text{s}$** → set `HPRE = /1`!

**Verified PLL Math ($f_{\text{PLL-IN}} \in [2.66, 16]\text{ MHz}$, $f_{\text{VCO}} \in [96, 344]\text{ MHz}$):**
- **170 MHz from HSI16 (16 MHz):** $M=4$ (`PLLM=3`) $\to 4\text{ MHz}$, $N=85 \to f_{\text{VCO}}=340\text{ MHz}$, $R=2$ (`PLLR=0`) $\to \mathbf{170\text{ MHz}}$. *(From 24 MHz HSE `X3`: $M=6$ [`PLLM=5`], $N=85$, $R=2$.)*
- **160 MHz from HSI16 (16 MHz):** $M=4$ (`PLLM=3`) $\to 4\text{ MHz}$, $N=80 \to f_{\text{VCO}}=320\text{ MHz}$, $R=2$ (`PLLR=0`) $\to \mathbf{160\text{ MHz}}$. *(From 24 MHz HSE `X3`: $M=6$ [`PLLM=5`], $N=80$, $R=2$.)*

### 3. Register-Level Implementation *(Statically Reviewed; 170 MHz version Compile-Verified in `clock_g4.c`)*
```c
#include <stdbool.h>
#include <stdint.h>
#include "stm32g474xx.h"

/* Configure HSI16 -> PLL to 170 MHz (pll_n = 85) OR 160 MHz (pll_n = 80) */
bool clock_init_hsi16_boost(uint32_t pll_n) {
    /* 1. Enable PWR clock + readback, verify Range 1 (VOS=01b) & VOSF==0 */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) { }

    /* 2. Enable Range 1 Boost Mode (PWR->CR5 @ 0x40007080, R1MODE = 0) */
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 3. Set Flash 4 Wait States + ART Prefetch/ICache/DCache & verify */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS | FLASH_ACR_PRFTEN |
                 FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) { }

    /* 4. Ensure HSI16 ready, disable PLL, wait PLLRDY == 0 */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) { }
    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U) { }

    /* 5. Program PLL: HSI16/4 = 4 MHz, VCO = 4 * pll_n (340 or 320 MHz), R = /2 */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI |
                   (3U << RCC_PLLCFGR_PLLM_Pos) |       /* M = 4 */
                   (pll_n << RCC_PLLCFGR_PLLN_Pos) |    /* N = 85 (170 MHz) or 80 (160 MHz) */
                   (0U << RCC_PLLCFGR_PLLR_Pos) |       /* R = 2 */
                   RCC_PLLCFGR_PLLREN;

    /* 6. Enable PLL and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) { }

    /* 7. Mandatory RM0440 §6.1.5 intermediate HPRE = /2 step (>= 1 us) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }

    for (uint32_t i = 0U; i < 120U; ++i) { __asm__ volatile("nop"); } /* > 1 us @ 85/80 MHz */

    /* 8. Ramp AHB prescaler to /1 (full 170 MHz or 160 MHz) */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE) | RCC_CFGR_HPRE_DIV1;
    SystemCoreClockUpdate();
    return true;
}
```

### 4. GDB Debugging Walkthrough
```gdb
(gdb) break main
(gdb) continue
(gdb) clock_regs                       # Built-in .gdbinit macro BEFORE switch:
                                       # RCC->CFGR == 0x00000005 (HSI16), PWR->CR5 == 0x00000100 (Normal)
(gdb) next                             # Step over Clock_Init_170MHz()
(gdb) clock_regs                       # AFTER switch:
                                       # RCC->CFGR    == 0x0000000F (SW=11b, SWS=11b, HPRE=0)
                                       # PWR->CR5     == 0x00000000 (R1MODE=0 Boost!)
                                       # PWR->SR2     == 0x00000000 (VOSF=0 Ready!)
                                       # FLASH->ACR   == 0x00040704 (4WS + PRFTEN + ICEN + DCEN)
(gdb) print /d SystemCoreClock         # Expect: 170000000 (or 160000000 for N=80)
```

### 5. Full Lab Link
- Complete HSE 24 MHz + HSI16 170 MHz Boost lab with `PA8` `MCO` verification: **[`Lab 03: RCC PLL 170 MHz Boost Mode Switch`](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md)** and production driver [`23-code-library/Drivers/clock_g4.c`](../23-code-library/Drivers/clock_g4.c).

### 6. Independent Exercise
Compute $M$, $N$, $R$, `PWR->CR5.R1MODE`, and `FLASH->ACR.LATENCY` to run at **160 MHz from the onboard 24 MHz HSE crystal (`X3`)** with `PLL_Q = 40 MHz` (`PLLQ = /8`) and `PLL_P = 40 MHz` (`PLLPDIV = 8`). Verify in GDB that `SystemCoreClock == 160000000` and `RCC->PLLCFGR` matches your bit-level hand calculation.

---

## Worked Example 4: `EXTI13` Interrupt + `__DSB()` Barrier + GDB `x/8wx $sp` Frame Inspection

**Concept IDs:** `PERIPH-EXTI-001`, `ARM-NVIC-001`, `ARM-EXC-002`, `RT-IRQ-002`

### 1. Small Demo (Goal)
Eliminate CPU polling by firing `EXTI15_10_IRQHandler` (`IRQ 40`) on every rising edge of Blue Button `B1` (`PC13`), clearing the pending flag safely, and inspecting the exact 8-word Cortex-M4 hardware exception stack frame in GDB.

### 2. Explanation
1. **Routing Path:** `PC13` (`GPIOC->MODER=00b`) → `SYSCFG->EXTICR[3]` (requires `RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN`) selects Port C for line 13 → `EXTI->RTSR1` arms rising edge (Active-HIGH `B1`) → `EXTI->IMR1` unmasks line 13 → `NVIC_EnableIRQ(EXTI15_10_IRQn)` unmasks IRQ 40.
2. **`rc_w1` Clearing:** `EXTI->PR1` is write-1-to-clear. Always write **`EXTI->PR1 = EXTI_PR1_PIF13;`** directly (never `|=`, which clears other pending EXTI bits!).
3. **`__DSB()` Before Exit (`ES0430 §2.1.3`):** Cortex-M4 write buffering can delay the `STR` to `EXTI->PR1` past `BX LR`, causing a spurious second ISR entry. `__DSB()` forces the clear to complete on the APB2 bus before exception return.
4. **Hardware Stacking:** Upon IRQ entry, Cortex-M4 automatically pushes **8 words (32 bytes)** onto the active stack: `[SP+0]=R0, [SP+4]=R1, [SP+8]=R2, [SP+12]=R3, [SP+16]=R12, [SP+20]=LR, [SP+24]=Return PC, [SP+28]=xPSR`, and loads `LR` with an `EXC_RETURN` magic value (e.g., `0xFFFFFFF9` for Thread/MSP non-FP).

### 3. Register-Level Implementation *(Statically Reviewed — Not Tested on Physical Hardware)*
```c
#include <stdint.h>
#include "stm32g474xx.h"

static volatile uint32_t g_press_count = 0U;

void exti13_init(void) {
    /* 1. Clock GPIOC (AHB2) and SYSCFG (APB2) + readback */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    /* 2. PC13 = Digital Input (00b), no pull (external pull-down on MB1367) */
    GPIOC->MODER &= ~(3U << (13U * 2U));
    GPIOC->PUPDR &= ~(3U << (13U * 2U));

    /* 3. Route Port C to EXTI line 13 in SYSCFG->EXTICR[3] (EXTICR4) */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~SYSCFG_EXTICR4_EXTI13_Msk) |
                        SYSCFG_EXTICR4_EXTI13_PC;

    /* 4. Rising edge (Active-HIGH B1), clear stale flag, unmask line 13 */
    EXTI->RTSR1 |= EXTI_RTSR1_RT13;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;
    EXTI->PR1    = EXTI_PR1_PIF13;         /* Direct write rc_w1 */
    EXTI->IMR1  |= EXTI_IMR1_IM13;

    /* 5. Configure NVIC priority (4 bits on STM32G474RE) and enable IRQ 40 */
    NVIC_SetPriority(EXTI15_10_IRQn, 5U);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler(void) {
    if ((EXTI->PR1 & EXTI_PR1_PIF13) != 0U) {
        EXTI->PR1 = EXTI_PR1_PIF13;        /* Direct write rc_w1 — NEVER |= */
        __DSB();                           /* Ensure flag clear drains before exit */
        g_press_count++;
        if ((GPIOA->ODR & (1U << 5U)) != 0U) {
            GPIOA->BSRR = GPIO_BSRR_BR5;
        } else {
            GPIOA->BSRR = GPIO_BSRR_BS5;
        }
    }
}
```

### 4. GDB Debugging Walkthrough
```gdb
(gdb) break EXTI15_10_IRQHandler
(gdb) continue
# Press blue B1 button on NUCLEO-G474RE -> target halts at ISR entry!
(gdb) info registers lr xpsr sp        # LR == 0xFFFFFFF9 (EXC_RETURN: Thread, MSP, basic frame)
                                       # xPSR[8:0] == 56 (0x38 = 16 core + 40 EXTI15_10_IRQn!)
(gdb) x/8wx $sp                        # Dump the 8 hardware-stacked words:
                                       # [0]=R0  [1]=R1  [2]=R2  [3]=R3
                                       # [4]=R12 [5]=LR  [6]=PC  [7]=xPSR
(gdb) info symbol *(uint32_t*)($sp+24) # Resolve exact function/line interrupted by B1!
(gdb) x/1wx 0x40010414                 # EXTI->PR1 before clear -> bit 13 == 1 (0x00002000)
(gdb) next                             # Step over EXTI->PR1 = EXTI_PR1_PIF13; __DSB();
(gdb) x/1wx 0x40010414                 # EXTI->PR1 after clear  -> bit 13 == 0 (0x00000000)
```

### 5. Full Lab Link
- Complete 3-way (Bare-Metal Register / ST LL / ST HAL) interrupt lab: **[`Lab 05: EXTI13 Rising-Edge Interrupt & NVIC`](../18-practical-laboratory/lab-05-exti-pushbutton-interrupt/README.md)**.

### 6. Independent Exercise
Use `NVIC->ISPR[1] = (1U << (40U - 32U));` (or write `40` to `NVIC->STIR`) from `main()` to trigger `EXTI15_10_IRQHandler` purely in software without pressing `B1`, set a breakpoint inside the handler, and verify `xPSR[8:0] == 56` and `x/8wx $sp` in GDB.

---

## Worked Example 5: `TIM2` Glitch-Free `EGR = UG` & `rc_w0` `SR = ~UIF`

**Concept IDs:** `PERIPH-TIM-002`, `EMB-REG-003`, `RT-IRQ-002`

### 1. Small Demo (Goal)
Generate a rock-solid **10 Hz (100 ms)** hardware periodic interrupt on 32-bit `TIM2` at **170 MHz** — with zero startup first-period glitch and zero risk of dropping concurrent timer flags.

### 2. Explanation (Why `PSC` Doesn't Take Effect Immediately & Why `SR &= ~UIF` Is a Bug)
1. **Shadow Registers & `EGR = UG`:** Writing `TIM2->PSC` only loads a *preload* register; the active prescaler stays at `0` until the first Update Event (`UEV`). Without forcing an update, the very first timer interval runs at the wrong frequency! Writing **`TIM2->EGR = TIM_EGR_UG;`** (direct write `=`, because `EGR` is **Write-Only**) copies `PSC` and `ARR` into their shadow registers immediately — **but also sets `TIM2->SR.UIF = 1`**, so you MUST clear `UIF` **before** setting `TIM2->DIER |= TIM_DIER_UIE` (otherwise the ISR fires prematurely on enable!).
2. **`rc_w0` Rule (`TIM2->SR = ~TIM_SR_UIF`):** Unlike `EXTI->PR1` (`rc_w1`), `TIMx->SR` is **`rc_w0` (write-0-to-clear, writing `1` has no effect)**. Writing `TIM2->SR &= ~TIM_SR_UIF;` performs a Read-Modify-Write that can overwrite a `CC1IF` flag that set between the read and write! Always write **`TIM2->SR = ~TIM_SR_UIF;`** directly.
3. **Math at 170 MHz (`PPRE1 = /1` $\implies f_{\text{TIM2}} = 170\text{ MHz}$):**
   $$\text{PSC} = 16{,}999 \implies f_{\text{CNT}} = \frac{170{,}000{,}000}{17{,}000} = 10{,}000\text{ Hz}, \qquad \text{ARR} = 999 \implies f_{\text{UEV}} = \frac{10{,}000}{1{,}000} = \mathbf{10\text{ Hz}}$$

### 3. Register-Level Implementation *(Statically Reviewed — Not Tested on Physical Hardware)*
```c
#include "stm32g474xx.h"

void tim2_init_10hz_at_170mhz(void) {
    /* 1. Enable TIM2 clock on APB1 + readback */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    (void)RCC->APB1ENR1;

    /* 2. Ensure timer stopped during configuration */
    TIM2->CR1  = 0U;
    TIM2->PSC  = 16999U;               /* 170 MHz / 17000 = 10 kHz tick */
    TIM2->ARR  = 999U;                 /* 10 kHz / 1000   = 10 Hz UEV   */
    TIM2->CNT  = 0U;

    /* 3. Force shadow register transfer (WO: direct '='), then clear spurious UIF (rc_w0) */
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = ~TIM_SR_UIF;

    /* 4. Enable Auto-Reload Preload (ARPE), Update IRQ, NVIC IRQ 28, and start counter */
    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC_SetPriority(TIM2_IRQn, 6U);
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void) {
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
        TIM2->SR = ~TIM_SR_UIF;        /* Atomic rc_w0 clear — NEVER &= ~TIM_SR_UIF */
        __DSB();
        if ((GPIOA->ODR & (1U << 5U)) != 0U) {
            GPIOA->BSRR = GPIO_BSRR_BR5;
        } else {
            GPIOA->BSRR = GPIO_BSRR_BS5;
        }
    }
}
```

### 4. GDB Debugging Walkthrough
```gdb
(gdb) break tim2_init_10hz_at_170mhz
(gdb) continue
(gdb) step                             # Step right past TIM2->EGR = TIM_EGR_UG;
(gdb) x/1wx 0x40000010                 # Observe TIM2->SR == 0x00000001 (UIF set by UG!)
(gdb) step                             # Step over TIM2->SR = ~TIM_SR_UIF;
(gdb) x/1wx 0x40000010                 # Confirm TIM2->SR == 0x00000000 (clean before UIE!)
(gdb) break TIM2_IRQHandler
(gdb) continue
(gdb) x/1wx 0x40000024                 # Inspect TIM2->CNT (near 0 right after rollover!)
```

### 5. Full Lab Link
- Complete 10 Hz timer lab: **[`Lab 06: General-Purpose Timer Periodic Interrupt`](../18-practical-laboratory/lab-06-general-purpose-timer-periodic-interrupt/README.md)**.

### 6. Independent Exercise
Recompute `PSC` and `ARR` for a **250 Hz (4 ms)** update rate at **160 MHz** (from Worked Example 3). Enable `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM2_STOP;` so `TIM2->CNT` freezes cleanly while you inspect registers in GDB.

---

## Worked Example 6: Decoding a Cortex-M4 HardFault (`CFSR`, `HFSR`, `MMFAR`, `BFAR`, Stacked `PC`)

**Concept IDs:** `ARM-FLT-001`, `REL-CRASH-001`, `MEM-NOINIT-001`, `TOOL-GDB-001`

### 1. Small Demo (Goal)
Intentionally trigger a Precise BusFault (dereferencing unmapped address `0x60000000`) and pinpoint the **exact faulting C source line and illegal address** in under 60 seconds using `SCB->CFSR`, `SCB->HFSR`, `SCB->BFAR`, and the stacked `PC`.

### 2. Explanation (How HardFault Forensics Works)
- By default, MemManage, BusFault, and UsageFault are disabled in `SCB->SHCSR`, so **all** faults escalate into `HardFault_Handler` with `SCB->HFSR.FORCED` (bit 30) set.
- Your naked-assembly trampoline (`HardFault_Handler` in [`startup_stm32g474re.c`](../23-code-library/templates/Src/startup_stm32g474re.c)) tests `EXC_RETURN` bit 2 (`TST LR, #4`) to pick **`MSP`** vs **`PSP`**, passes that pointer to `HardFault_Handler_C(uint32_t *stacked_frame)`, validates that the stack pointer lies inside SRAM/CCMRAM, and saves `CFSR`, `HFSR`, `MMFAR`, `BFAR`, and `R0..xPSR` into `.noinit` (`g_crash_log` in [`fault_handler.c`](../23-code-library/Drivers/fault_handler.c)).
- **Decoding `SCB->CFSR` (`0xE000ED28`):**
  - Bits `[7:0]` = **`MMFSR`** (`DACCVIOL`=b1, `MMARVALID`=b7 → check `SCB->MMFAR`)
  - Bits `[15:8]` = **`BFSR`** (`PRECISERR`=b9, `IMPRECISERR`=b10, `BFARVALID`=b15 → check `SCB->BFAR`)
  - Bits `[31:16]` = **`UFSR`** (`UNDEFINSTR`=b16, `INVSTATE`=b17, `NOCP`=b19 [FPU off!], `UNALIGNED`=b24, `DIVBYZERO`=b25)

### 3. Register-Level Implementation *(Compile-Verified in `fault_handler.c` / `startup_stm32g474re.c`; Demo Trigger Statically Reviewed)*
```c
#include <stdint.h>
#include "fault_handler.h"
#include "stm32g474xx.h"

/* Deliberate Precise BusFault trigger (unmapped external memory region 0x60000000) */
uint32_t trigger_precise_busfault_demo(void) {
    volatile uint32_t *bad_ptr = (volatile uint32_t *)0x60000000UL;
    return *bad_ptr;   /* Hardware traps -> HardFault_Handler -> HardFault_Handler_C */
}
```

### 4. GDB Debugging Walkthrough
When the fault triggers while attached to OpenOCD/VS Code, `HardFault_Handler_C` executes `bkpt #0` (guarded by `CoreDebug->DHCSR.C_DEBUGEN`):
```gdb
(gdb) fault_regs                       # Built-in .gdbinit macro:
                                       # SCB->CFSR  (0xE000ED28) = 0x00008200
                                       #   -> Bit 9  (PRECISERR) = 1 (Precise BusFault!)
                                       #   -> Bit 15 (BFARVALID) = 1 (BFAR holds exact bad address!)
                                       # SCB->HFSR  (0xE000ED2C) = 0x40000000 (FORCED escalation)
                                       # SCB->BFAR  (0xE000ED38) = 0x60000000 (Exact illegal address!)
(gdb) crash_log                        # Built-in .gdbinit macro: prints g_crash_log struct
                                       # g_crash_log.pc = 0x080004a2 (exact faulting instruction!)
(gdb) list *(g_crash_log.pc)           # Maps PC directly back to C file & line number!
(gdb) disassemble /m g_crash_log.pc-8, g_crash_log.pc+8
```

### 5. Full Lab Link
- Complete 8-region MPU stack guard + persistent `.noinit` crash logger lab: **[`Lab 19: MPU Stack Guard & Naked-ASM HardFault Diagnostics`](../18-practical-laboratory/lab-19-hardfault-mpu-and-crash-diagnostics/README.md)** and driver [`23-code-library/Drivers/fault_handler.c`](../23-code-library/Drivers/fault_handler.c).

### 6. Independent Exercise
Call `FaultHandler_EnableDetailedFaults(true);` (which sets `SCB->CCR.DIV_0_TRP` bit 4), then execute a volatile integer divide-by-zero (`volatile int a = 10, b = 0; int c = a / b;`). Inspect `SCB->CFSR` in GDB and verify **bit 25 (`DIVBYZERO`, `0x02000000`)** is set and `list *(g_crash_log.pc)` points directly to the `SDIV` instruction!
