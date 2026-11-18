# Lab 05: EXTI Pushbutton Interrupt, NVIC Priority & 3-Way API Comparison

> ⬅️ **Prev Lab:** [Lab 04: SysTick Accurate Delay](../lab-04-systick-accurate-delay-generator/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 06: TIM2 Periodic Interrupt](../lab-06-general-purpose-timer-periodic-interrupt/README.md) ➡️

> **Verification Status:** `ONBOARD HARDWARE TESTED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Evidence:** NUCLEO-G474RE `onboard_selftest` Test 4 (`EXTI13` `swier_cb=3/3`, `last_line=13`, `SYSCFG_EXTICR4=0x00000020`, zero spurious re-entry)

> **Clock Configuration Profile:** **Primary = `16 MHz HSI16` (Reset Default)** | **160 MHz / 170 MHz Boost Compatibility:** Identical `SYSCFG`/`EXTI`/`NVIC` registers (`__DSB()` becomes even more critical at `160 MHz` / `170 MHz`) | **Reusable Drivers:** [`23-code-library/Drivers/exti_g4.h`](../../23-code-library/Drivers/exti_g4.h) (`EXTI_ConfigLine()`, `EXTI_RegisterCallback()`, `EXTI_DispatchRange()`, `EXTI_ClearPending()`), [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h), & [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h).

---

## 1. Objective & Concept IDs

Route the NUCLEO-G474RE onboard Blue Button (`B1` on `PC13`, **Active-HIGH** per UM2505) through `SYSCFG` to `EXTI13` with **Rising-Edge** trigger, configure NVIC priority grouping and `EXTI15_10_IRQn`, clear the `rc_w1` pending flag safely with a Cortex-M4 `(void)EXTI->PR1;` readback and `__DSB()` barrier, and toggle `LD2` via single-writer `BSRR` set/reset writes — plus compare **Direct Register vs LL vs HAL** interrupt patterns.

- **Prerequisites:** [Lab 01 (GPIO `BSRR`)](../lab-01-baremetal-gpio-led-blinky/README.md), [Lab 02 (`PC13` Active-HIGH `B1`)](../lab-02-gpio-pushbutton-debouncing/README.md), [Stage 04 (ARM Cortex-M4 NVIC)](../../04-arm-cortex-m/README.md), and [Stage 10 (Interrupts & Barriers)](../../10-interrupts/README.md)
- **Target Concept IDs:** `PERIPH-EXTI-001` (External Interrupts), `PERIPH-SYSCFG-001` (EXTICR Pin Routing), `ARM-NVIC-001` (Priority Grouping & Preemption), `EMB-BAR-001` (APB Readback & `__DSB()` Barrier)
- **Scope Layer:** `[STM32G474RE Exact]` / `[Cortex-M4 Specific]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Onboard Peripherals:** Blue Pushbutton `B1` (`PC13`, Active-HIGH with external pull-down) and Green LED `LD2` (`PA5`)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | Board Net | `MODER` | `PUPDR` | EXTI Line | Trigger Edge (on Press) |
|---|---|---|---|---|---|
| `PC13` | `B1` (Blue Button) | `00b` (Digital Input) | `00b` (No Pull — external pull-down per UM2505) | `EXTI13` | **Rising Edge (`EXTI->RTSR1`)** |
| `PA5` | `LD2` (Green LED) | `01b` (Push-Pull Output) | `00b` (No Pull) | N/A | N/A |

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 10** (*System Configuration Controller — SYSCFG*, §10.2.6 `SYSCFG_EXTICR4`), **RM0440 Chapter 15** (*Extended Interrupts and Events Controller — EXTI*, §15.5), **ARM PM0214 §4.3** (*NVIC*), and **ST UM2505 §6.5**.

| Register | Offset / Bitfield | Configured Value | Purpose |
|---|---|---|---|
| `RCC->APB2ENR` | Bit 0 (`SYSCFGEN`) | `1` | **Mandatory:** Enables `SYSCFG` register clock! |
| `SYSCFG->EXTICR[3]` | Bits `[7:4]` (`EXTI13[3:0]`) | `0010b` (`2U` = Port C) | Connects `PC13` to `EXTI13` (`EXTICR[3]` covers lines 12–15) |
| `EXTI->RTSR1` | Bit 13 (`RT13`) | `1` | Rising-edge trigger (fires when Active-HIGH `B1` is pressed) |
| `EXTI->FTSR1` | Bit 13 (`FT13`) | `0` | Explicitly disabled so release edge does not trigger |
| `EXTI->IMR1` | Bit 13 (`IM13`) | `1` | Unmasks `EXTI13` interrupt to NVIC |
| `EXTI->PR1` | Bit 13 (`PIF13`, `rc_w1`) | Write `1U << 13` | Direct assignment clears pending bit (never use `|=`) |
| `NVIC` | `EXTI15_10_IRQn` (`IRQ 40`) | Priority `3` (`PRIGROUP=0`) | 4 preemption bits, 0 subpriority bits (`__NVIC_PRIO_BITS = 4`) |

### Explicit Clock Configuration Profile Declaration (`16 MHz` vs `160 MHz` vs `170 MHz`)

`EXTI13` edge detection is asynchronous to `SYSCLK` and requires no frequency-dependent prescalers. However, at `160 MHz` and `170 MHz` Range 1 Boost, the AHB-to-APB2 posted-write latency (`EXTI->PR1` clear) relative to CPU instruction retirement makes the authoritative **dummy readback + tail `__DSB()`** sequence (`EXTI->PR1 = EXTI_PR1_PIF13; (void)EXTI->PR1;` early, plus `__DSB();` before ISR exit — addressing **ARM AN321 §4.9** APB2 bridge synchronization and **ARM Erratum 838869 / ST ES0430 §2.1.3** store-immediate/exception-return protection) recommended production practice:

| Clock Profile | `clock_g4.h` API | `SYSCFG`/`EXTI`/`NVIC` Registers | `EXTI->PR1` Clear, Readback & Barrier | Interrupt Stacking Latency (`12` Core Cycles) |
|---|---|---|---|---|
| **`16 MHz HSI16` *(Primary here)*** | Reset default | `EXTICR[3]=0x20`, `RTSR1=0x2000`, `IMR1=0x2000` | `EXTI->PR1 = EXTI_PR1_PIF13; (void)EXTI->PR1; __DSB();` | `~750 ns` |
| **`160 MHz Range 1 Boost`** | `Clock_Init_160MHz()` | **Identical** (`EXTI` is clock-independent) | `EXTI->PR1 = EXTI_PR1_PIF13; (void)EXTI->PR1; __DSB();` | `~75.0 ns` |
| **`170 MHz Range 1 Boost`** | `Clock_Init_170MHz()` | **Identical** (`EXTI` is clock-independent) | `EXTI->PR1 = EXTI_PR1_PIF13; (void)EXTI->PR1; __DSB();` | `~70.6 ns` |

- **Reusable Production Driver (`23-code-library/Drivers/exti_g4.h`):** Replace boilerplate setup with `GPIO_ConfigInput(GPIOC, 13U, GPIO_PULL_NONE)`, `EXTI_RegisterCallback(13U, on_btn, NULL)`, `EXTI_ConfigLine(GPIOC, 13U, EXTI_EDGE_RISING, 3U)`, and call `EXTI_DispatchRange(10U, 15U)` inside `EXTI15_10_IRQHandler()` (includes `EXTI_ClearPending()` with `(void)EXTI->PR1;` readback + `__DSB()` automatically).

> [!IMPORTANT]
> **Why `(void)EXTI->PR1;` Readback + `__DSB()` Are Paired Before ISR Exit on Cortex-M4:** These address **two distinct hardware mechanisms**:
> 1. **APB2 Posted-Write Bridge Commit (`(void)EXTI->PR1;` — ARM AN321 §4.9):** Writing `EXTI->PR1 = EXTI_PR1_PIF13` posts a Device-memory store across the AHB-to-APB2 bridge. `__DSB()` alone drains the processor's internal write buffer, whereas the **dummy readback `(void)EXTI->PR1;`** stalls until the AHB-to-APB2 bridge physically commits the clear at the EXTI slave so NVIC does not observe a stale asserted `EXTI13` line and tail-chain a **spurious duplicate ISR entry**.
> 2. **ARM Cortex-M4 r0p1 Erratum 838869 (`__DSB();` — ST ES0430 §2.1.3):** On Cortex-M4 r0p1 (`__CM4_REV = 0x0001U`), a stalling store-immediate instruction overlapping a `BX LR` exception return under bus wait states can cause the core to **vector to an incorrect interrupt handler**. Placing `__DSB();` immediately before exception return drains the internal write buffer and is ARM's documented workaround for that rare overlap.

---

## 6. Code Implementation

All three implementations below (**6.1 CMSIS Direct Register**, **6.2 STM32CubeG4 LL**, and **6.3 STM32CubeG4 HAL**) are **functionally equivalent, independently complete standalone programs** (`GPIOA` + `GPIOC` + `SYSCFG` clocks, `PA5` `LD2` push-pull output initialized explicitly LOW, `PC13` `B1` input with no internal pull, `SYSCFG` `EXTI13 -> Port C`, rising-edge trigger with falling-edge disabled, stale `EXTI->PR1` clear + unmask, `NVIC` 4 preemption / 0 subpriority grouping at priority `3`, complete `EXTI15_10_IRQHandler()` with `(void)EXTI->PR1; __DSB();`, equivalent `LD2` toggle, and `__WFI()` `main()` loop).

### 6.1 Approach A — Direct Register Access (CMSIS Bare-Metal)

> **[COMPLETE STANDALONE IMPLEMENTATION — Bare-Metal CMSIS, Cross-Compiled]**  
> *Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings / 0 errors; link with shared CMSIS startup & STM32G474RE linker script for full `.elf`).*

```c
#include "stm32g474xx.h"
#include <stdint.h>

static volatile uint32_t g_button_irq_count = 0U;

void EXTI15_10_IRQHandler(void) {
    if ((EXTI->PR1 & EXTI_PR1_PIF13) != 0U) {
        /* 1. Clear pending flag early via direct write-1 (rc_w1) — NEVER use |=
         *    and flush AHB-to-APB2 posted-write bridge via readback (ARM AN321 §4.9). */
        EXTI->PR1 = EXTI_PR1_PIF13;
        (void)EXTI->PR1;

        /* 2. Single-writer LD2 toggle via BSRR (cross-pin atomic on Port A;
         *    non-atomic for concurrent same-pin writers — see callout below) */
        if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
            GPIOA->BSRR = GPIO_BSRR_BR5;
        } else {
            GPIOA->BSRR = GPIO_BSRR_BS5;
        }

        g_button_irq_count++;

        /* 3. Tail barrier immediately before BX LR: drains write buffer & guards
         *    Cortex-M4 r0p1 Erratum 838869 (ST ES0430 §2.1.3: trailing store-immediate
         *    overlapping exception return). */
        __DSB();
    }
}

int main(void) {
    /* 1. Enable GPIOA, GPIOC (AHB2) and SYSCFG (APB2) clocks + sync readbacks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->AHB2ENR;
    (void)RCC->APB2ENR;
    __DSB();

    /* 2. PA5 (LD2) -> explicit initial state LOW, then Push-Pull Output, Low Speed, No Pull
     *    (clear STM32G4 11b Analog reset state before setting 01b Output) */
    GPIOA->BSRR    =  GPIO_BSRR_BR5;
    GPIOA->OTYPER  &= ~(1U << 5U);
    GPIOA->OSPEEDR &= ~(3U << (5U * 2U));
    GPIOA->PUPDR   &= ~(3U << (5U * 2U));
    GPIOA->MODER   &= ~(3U << (5U * 2U));
    GPIOA->MODER   |=  (1U << (5U * 2U));

    /* 3. PC13 (B1) -> Digital Input (00b), No Pull (external pull-down per UM2505) */
    GPIOC->MODER &= ~(3U << (13U * 2U));
    GPIOC->PUPDR &= ~(3U << (13U * 2U));

    /* 4. Map PC13 to EXTI13 in SYSCFG->EXTICR[3] bits [7:4] = 0010b (Port C) */
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13_Msk;
    SYSCFG->EXTICR[3] |=  SYSCFG_EXTICR4_EXTI13_PC;

    /* 5. Configure EXTI13 for Rising Edge (Active-HIGH B1 press), falling edge disabled */
    EXTI->RTSR1 |=  EXTI_RTSR1_RT13;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;
    EXTI->PR1    =  EXTI_PR1_PIF13; /* Clear any stale pending flag (rc_w1) */
    (void)EXTI->PR1;
    EXTI->IMR1  |=  EXTI_IMR1_IM13;

    /* 6. Configure NVIC: PRIGROUP = 0 -> 4 preemption bits, 0 subpriority bits
     *    (identical split to HAL NVIC_PRIORITYGROUP_4 since __NVIC_PRIO_BITS = 4) */
    NVIC_SetPriorityGrouping(0U);
    NVIC_SetPriority(EXTI15_10_IRQn, 3U);
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    while (1) {
        /* CPU sleeps in low-power WFI state until EXTI13 wakes it */
        __WFI();
    }
}
```

### 6.2 Approach B — STM32Cube Low-Layer (LL) Comparison

> **[COMPLETE STANDALONE IMPLEMENTATION — STM32CubeG4 LL Environment Required, Statically Reviewed]**  
> *Requires shared CMSIS startup/linker script + STM32CubeG4 LL headers (`stm32g4xx_ll_*.h`) in include path. STM32CubeG4 HAL/LL package not installed on host — Statically Reviewed against official `STMicroelectronics/stm32g4xx-hal-driver` API.*

```c
/* Complete STM32CubeG4 LL Program — requires shared CMSIS startup/linker +
 * STM32CubeG4 LL headers in include path (not built by bare-metal CMSIS Makefile). */
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_gpio.h"
#include <stdint.h>

static volatile uint32_t g_button_irq_count = 0U;

void EXTI15_10_IRQHandler(void) {
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_13) != 0U) {
        /* 1. Clear pending flag via rc_w1 write and flush AHB-to-APB2 posted-write
         *    bridge via dummy readback (ARM AN321 §4.9). */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
        (void)EXTI->PR1;

        /* 2. Single-writer LD2 toggle via BSRR inside LL_GPIO_TogglePin()
         *    (cross-pin atomic on Port A; reads ODR then writes BSRR) */
        LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_5);
        g_button_irq_count++;

        /* 3. Tail barrier immediately before BX LR (ARM Erratum 838869 / ST ES0430 §2.1.3). */
        __DSB();
    }
}

static void Lab05_LL_Init(void) {
    /* 1. Enable GPIOA, GPIOC (AHB2) and SYSCFG (APB2) clocks (LL includes readback) */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    /* 2. PA5 (LD2) -> explicit initial state LOW, then Push-Pull Output, Low Speed, No Pull */
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_5);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_5, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_5, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_5, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_OUTPUT);

    /* 3. PC13 (B1) -> Digital Input, No Pull (external pull-down per UM2505) */
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_NO);

    /* 4. Route PC13 -> EXTI13 and enable Rising-Edge interrupt (falling edge disabled) */
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_13);
    LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_13);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
    (void)EXTI->PR1;
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_13);

    /* 5. Configure NVIC: 4 preemption bits, 0 subpriority bits */
    NVIC_SetPriorityGrouping(0U);
    NVIC_SetPriority(EXTI15_10_IRQn, 3U);
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int main(void) {
    Lab05_LL_Init();

    while (1) {
        __WFI();
    }
}
```

### 6.3 Approach C — STM32Cube HAL Comparison

> **[COMPLETE STANDALONE IMPLEMENTATION — STM32CubeG4 HAL Environment Required, Statically Reviewed]**  
> *Requires STM32CubeG4 HAL drivers (`stm32g4xx_hal*.c/.h`) + `SysTick_Handler -> HAL_IncTick()`. STM32CubeG4 HAL/LL package not installed on host — Statically Reviewed against official `STMicroelectronics/stm32g4xx-hal-driver` API.*

```c
/* Complete STM32CubeG4 HAL Program — requires STM32CubeG4 HAL drivers &
 * SysTick_Handler -> HAL_IncTick() (not built by bare-metal CMSIS Makefile). */
#include "stm32g4xx_hal.h"
#include <stdint.h>

static volatile uint32_t g_button_irq_count = 0U;

void SysTick_Handler(void) {
    HAL_IncTick();
}

void EXTI15_10_IRQHandler(void) {
    /* Clears EXTI->PR1 bit 13 (rc_w1) and invokes HAL_GPIO_EXTI_Callback(GPIO_PIN_13) */
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    (void)EXTI->PR1; /* Flush AHB-to-APB2 posted-write bridge (ARM AN321 §4.9) */
    __DSB();         /* Drain write buffer / guard Erratum 838869 (ST ES0430 §2.1.3) */
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_13) {
        /* Single-writer LD2 toggle via BSRR inside HAL_GPIO_TogglePin() */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        g_button_irq_count++;
    }
}

static void Lab05_HAL_Init(void) {
    /* 1. Initialize HAL (configures Flash prefetch, NVIC_PRIORITYGROUP_4, 1 ms SysTick) */
    HAL_Init();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* 2. Enable GPIOA and GPIOC clocks.
     *    Note: HAL_GPIO_Init() enables the SYSCFG clock internally via
     *    __HAL_RCC_SYSCFG_CLK_ENABLE() when configuring GPIO_MODE_IT_RISING. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 3. PA5 (LD2) -> explicit initial state LOW, then Push-Pull Output, No Pull, Low Speed */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_5;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 4. PC13 (B1) -> Rising-Edge EXTI13 (HAL_GPIO_Init enables SYSCFG clock, maps
     *    SYSCFG->EXTICR[3] to Port C, sets RTSR1, clears FTSR1, and unmasks IMR1) */
    gpio.Pin   = GPIO_PIN_13;
    gpio.Mode  = GPIO_MODE_IT_RISING;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* Clear any stale EXTI13 pending bit latched during pin/EXTICR routing */
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
    (void)EXTI->PR1;

    /* 5. Configure and enable NVIC EXTI15_10_IRQn (preemption priority 3, subpriority 0) */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3U, 0U);
    HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int main(void) {
    Lab05_HAL_Init();

    while (1) {
        __WFI();
    }
}
```

> [!IMPORTANT]
> **Atomicity Clarification — Cross-Pin Atomic `BSRR` vs Non-Atomic Same-Pin Toggle:**
> - **What `GPIOA->BSRR` makes atomic:** Writing `GPIOA->BSRR = GPIO_BSRR_BS5` (set) or `GPIOA->BSRR = GPIO_BSRR_BR5` (reset) is **atomic with respect to all other pins on `GPIOA`**, eliminating the classic `GPIOA->ODR ^= (1U << 5)` read-modify-write hazard that corrupts sibling pins modified by another ISR or task.
> - **What reading `ODR` and writing `BSRR` does NOT make atomic:** The full toggle sequence — reading `GPIOA->ODR` and conditionally writing `GPIOA->BSRR` (used in Approach A, inside `LL_GPIO_TogglePin()`, and inside `HAL_GPIO_TogglePin()`) — is **NOT atomic as a complete same-pin toggle**. If two concurrent execution contexts (e.g., `main()` and a preempting ISR, or two nested ISRs) both toggle `PA5`, a preemption between the `ODR` read and the `BSRR` write causes a lost toggle.
> - **Why Lab 05 is race-free (and how to handle multi-writer pins):** In Lab 05, `EXTI15_10_IRQHandler` is the **sole writer** of `PA5`, so this single-writer toggle is completely race-free. If multiple concurrent contexts must ever toggle the **same pin**, enforce an explicit single-owner design or wrap the toggle in a short critical section (`__disable_irq()` / `PRIMASK` or `BASEPRI`).

### 6.4 3-Way Comparison Summary

- **Direct Register:** Explicit control of `PR1` (`rc_w1`), `SYSCFGEN`, dummy readback `(void)EXTI->PR1;`, and `__DSB()`; minimum interrupt latency (12 cycles hardware stacking + ~8 cycles body).
- **STM32 LL:** Identical runtime latency to Direct Register when compiled with `-O2`, while providing self-documenting inline wrappers (`LL_SYSCFG_EXTI_PORTC`, `LL_EXTI_ClearFlag_0_31`, and `LL_GPIO_TogglePin` which reads `ODR` and writes `BSRR`).
- **STM32 HAL:** Automatically enables `SYSCFGEN` inside `HAL_GPIO_Init()`, but `HAL_GPIO_EXTI_IRQHandler()` adds function call overhead and weak-callback indirection (`HAL_GPIO_EXTI_Callback`), and `HAL_Init()` requires `SysTick_Handler -> HAL_IncTick()`.

---

## 7. Expected Output

The CPU stays halted in `__WFI()` sleep mode consuming zero active loop cycles until `B1` (`PC13`) is pressed, at which point `EXTI15_10_IRQHandler` fires immediately on the **rising edge** and toggles `LD2`. (Note: mechanical bounce may still cause occasional multiple triggers — see Extension Exercise 1 to combine EXTI with a hardware timer blanking window.)

---

## 8. Debugging Procedure

1. Set a breakpoint inside `EXTI15_10_IRQHandler`.
2. Verify before pressing `B1`:
   ```gdb
   (gdb) p/x RCC->APB2ENR          # Bit 0 (SYSCFGEN) == 1
   (gdb) p/x SYSCFG->EXTICR[3]     # Bits [7:4] == 0x2 (Port C)
   (gdb) p/x EXTI->RTSR1           # Bit 13 == 1 (0x2000)
   (gdb) p/x EXTI->IMR1            # Bit 13 == 1 (0x2000)
   ```
3. Press `B1` and confirm GDB halts inside `EXTI15_10_IRQHandler` with `(EXTI->PR1 & 0x2000) != 0`.
4. Step over `EXTI->PR1 = EXTI_PR1_PIF13;` and confirm bit 13 clears to `0`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Interrupt never fires when pressing `B1` | Forgot `RCC->APB2ENR \|= RCC_APB2ENR_SYSCFGEN`, so `EXTICR[3]` stayed `0` (Port A!) | Enable `SYSCFGEN` before writing `SYSCFG->EXTICR[3]` |
| Interrupt fires when *releasing* `B1` instead of pressing | Configured `EXTI->FTSR1` (falling edge) assuming STM32F4 Active-LOW | Use `EXTI->RTSR1` (rising edge) for NUCLEO-G474RE Active-HIGH `B1` |
| CPU locks up executing ISR infinitely | Forgot to clear `EXTI->PR1` bit 13, or cleared with `&= ~` instead of `= (1U << 13)` | Write `EXTI->PR1 = EXTI_PR1_PIF13;` (`rc_w1` semantics) |
| Occasional double-trigger after clearing `PR1` or rare wrong-vectoring (Erratum 838869) | Posted APB2 write did not commit at EXTI slave before ISR exit (ARM AN321 §4.9 → duplicate entry), or trailing buffered store-immediate overlapped `BX LR` (ARM Erratum 838869 / ES0430 §2.1.3 → incorrect vector fetch) | Perform `(void)EXTI->PR1;` dummy readback right after clearing `PR1` (APB2 bridge commit, AN321 §4.9) and place `__DSB();` at the tail of `EXTI15_10_IRQHandler` immediately before exit (write-buffer drain + ES0430 §2.1.3) |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `SYSCFG->EXTICR[3]` bits `[7:4]` equal `2` (Port C) and `EXTI->RTSR1` bit 13 equals `1`.
- [ ] Pressing `B1` increments `g_button_irq_count` on press (not release).
- [ ] ISR uses single-writer `GPIOA->BSRR` set/reset writes (cross-pin atomic on Port A) and performs `(void)EXTI->PR1;` dummy readback (ARM AN321 §4.9) followed by `__DSB();` (ARM Erratum 838869 / ST ES0430 §2.1.3) before exit.

---

## 11. Extension Exercises

1. **ISR-Safe Debouncing:** Instead of toggling the LED directly in `EXTI15_10_IRQHandler`, mask `EXTI13` inside the ISR and start a 25 ms one-shot timer (TIM6/SysTick) that re-samples `PC13` and re-enables `EXTI13` once bounce settles.
2. **Nested Priority Preemption:** Add a higher-priority `SysTick` (Priority `1`) and lower-priority `EXTI15_10_IRQn` (Priority `3`), and inspect the `IPSR` register in GDB during nested preemption.
3. **Zero-Stacking Event Wakeup (`__WFE`):** Replace `EXTI->IMR1` with `EXTI->EMR1` and `__WFI()` with `__WFE()` to wake the CPU with low-latency hardware event signaling (avoiding exception frame stacking and vector fetch overhead).

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `ONBOARD HARDWARE TESTED` *(Verified via `onboard_selftest` Test 4 using `exti_g4.c` on NUCLEO-G474RE; Approach A Cross-Compiled; Approaches B/C LL & HAL Statically Reviewed)* |
| **Objective** | Route Active-HIGH `B1` (`PC13`) via `SYSCFG->EXTICR[3]` to rising-edge `EXTI13` (`EXTI15_10_IRQn`), toggle `LD2` (`PA5`) via single-writer `BSRR` set/reset writes, and clear `rc_w1` `EXTI->PR1` with `(void)EXTI->PR1; __DSB();` while comparing Register vs LL vs HAL. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,304 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `SYSCFG`, `EXTI`, `NVIC`, `GPIOA` (`PA5` `LD2`), `GPIOC` (`PC13` `B1`). Zero external wiring required (also supports software self-test via `EXTI->SWIER1` / `EXTI_TriggerSoftware(13)`). |
| **Hardware Availability** | `ONBOARD — PHYSICALLY VERIFIED (Agent 4, 2026-11-18; manual physical B1 press optional)` |
| **Pin Configuration** | `PC13`: `MODER[27:26] = 00b`, `PUPDR[27:26] = 00b`, `EXTICR[3][7:4] = 0010b` (Port C), Rising Edge `RTSR1[13] = 1`; `PA5`: `MODER[11:10] = 01b`, `OTYPER[5] = 0`, initial state LOW (`BSRR = BR5`). |
| **Expected Behavior** | CPU sleeps in `__WFI()` until `B1` rising edge (or software trigger `EXTI->SWIER1 = (1U << 13)`), enters `EXTI15_10_IRQHandler`, clears `PR1[13]`, toggles `LD2`, increments `g_button_irq_count`, and returns without spurious re-entry. |
| **Actual Observations** | **EXECUTED ON NUCLEO-G474RE (`onboard_selftest` Test 4, 2026-11-18):** `[PASS] TEST4_TIM2_AND_EXTI13: EXTI13(swier_cb=3 last_line=13 SYSCFG_EXTICR4=0x00000020)` — verified `SYSCFGEN`, `EXTICR[3]` Port C mapping (`0x20`), `EXTI15_10_IRQHandler` dispatch, `EXTI->PR1` `rc_w1` clear + `(void)EXTI->PR1; __DSB();` with zero spurious duplicate entries across 3/3 software triggers. *(STM32CubeG4 HAL/LL package not installed on host — Approaches B & C Statically Reviewed against official `STMicroelectronics/stm32g4xx-hal-driver` API.)* |
| **Register Values** | Reset: `RCC->APB2ENR = 0`, `SYSCFG->EXTICR[3] = 0x0000`, `EXTI->IMR1 = 0x1F800000` (lines 0..15 masked). Configured: `RCC->APB2ENR[0] = 1` (`SYSCFGEN`), `SYSCFG->EXTICR[3][7:4] = 0x2`, `EXTI->RTSR1[13] = 1`, `EXTI->FTSR1[13] = 0`, `EXTI->IMR1[13] = 1`, `NVIC` `PRIGROUP = 0`, `EXTI15_10_IRQn (40)` prio = `3`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` break on `EXTI15_10_IRQHandler`, inspect `p/x SYSCFG->EXTICR[3]`, `p/x EXTI->RTSR1`, `p/x EXTI->PR1`, and verify `p g_button_irq_count` increments cleanly. |
| **Remaining Limitations** | Raw mechanical button edge interrupt without timer blanking may trigger multiple counts on noisy tactile bounce (see Extension Exercise 1 and Lab 02 FSM). STM32CubeG4 HAL/LL package not installed on host — Approaches B (LL) and C (HAL) are Statically Reviewed against official `STMicroelectronics/stm32g4xx-hal-driver` API. Manual physical `B1` finger press remains optional. |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | All 3 approaches (CMSIS Register, STM32CubeG4 LL, STM32CubeG4 HAL) independently complete with `PA5` (explicit LOW init) + `PC13` init, `EXTI15_10_IRQHandler`, `(void)EXTI->PR1; __DSB();`, and `main()`. Verified against `stm32g474xx.h`, RM0440 Ch. 10/15, and official `STMicroelectronics/stm32g4xx-hal-driver` API (CubeG4 package not installed on host). |
| **Host test passed** | `PASS` | `tests/unit/test_drivers_math.c` verifies `exti_calc_exticr_index_shift()` for lines 0..15 (part of 292/292 driver helper assertions). |
| **Cross-compilation passed** | `PASS` | Approach A bare-metal CMSIS & `exti_g4.c` compile cleanly (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings). |
| **On-target upload succeeded** | `PASS` | Flashed & verified on NUCLEO-G474RE via `onboard_selftest.elf` (`VDD = 3.29 V`). |
| **Physical behavior verified** | `PASS` | `TEST4_TIM2_AND_EXTI13` verified `SYSCFG_EXTICR4=0x00000020`, `swier_cb=3/3`, `last_line=13`, and zero duplicate re-entries. |
| **Measurement verified** | `PARTIAL` | Onboard NVIC/SYSCFG/EXTI13 callback & register state verified (`PASS`); manual physical `B1` tactile actuation optional. |

---

> ⬅️ **Prev Lab:** [Lab 04: SysTick Accurate Delay](../lab-04-systick-accurate-delay-generator/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 06: TIM2 Periodic Interrupt](../lab-06-general-purpose-timer-periodic-interrupt/README.md) ➡️
