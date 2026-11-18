# Lab 05: EXTI Pushbutton Interrupt, NVIC Priority & 3-Way API Comparison

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Queue:** `ONBOARD — READY FOR AGENT 4` *(Not Yet Executed on Physical NUCLEO-G474RE Hardware)*

> **Clock Configuration Profile:** **Primary = `16 MHz HSI16` (Reset Default)** | **160 MHz / 170 MHz Boost Compatibility:** Identical `SYSCFG`/`EXTI`/`NVIC` registers (`__DSB()` becomes even more critical at `160 MHz` / `170 MHz`) | **Reusable Drivers:** [`23-code-library/Drivers/exti_g4.h`](../../23-code-library/Drivers/exti_g4.h) (`EXTI_ConfigLine()`, `EXTI_RegisterCallback()`, `EXTI_DispatchRange()`, `EXTI_ClearPending()`), [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h), & [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h).

---

## 1. Objective & Concept IDs

Route the NUCLEO-G474RE onboard Blue Button (`B1` on `PC13`, **Active-HIGH** per UM2505) through `SYSCFG` to `EXTI13` with **Rising-Edge** trigger, configure NVIC priority grouping and `EXTI15_10_IRQn`, clear the `rc_w1` pending flag safely with a Cortex-M4 `__DSB()` barrier, and toggle `LD2` atomically via `BSRR` — plus compare **Direct Register vs LL vs HAL** interrupt patterns.

- **Target Concept IDs:** `PERIPH-EXTI-001` (External Interrupts), `PERIPH-SYSCFG-001` (EXTICR Pin Routing), `ARM-NVIC-001` (Priority Grouping & Preemption), `EMB-BAR-001` (`__DSB()` Posted-Write Barrier)
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

`EXTI13` edge detection is asynchronous to `SYSCLK` and requires no frequency-dependent prescalers. However, at `160 MHz` and `170 MHz` Range 1 Boost, the AHB-to-APB2 posted-write latency (`EXTI->PR1` clear) relative to CPU instruction retirement makes the trailing **`__DSB()`** (ES0430 §2.1.3 / ARM Errata 838869) strictly mandatory:

| Clock Profile | `clock_g4.h` API | `SYSCFG`/`EXTI`/`NVIC` Registers | `EXTI->PR1` Clear & Barrier | Interrupt Stacking Latency (`12` Core Cycles) |
|---|---|---|---|---|
| **`16 MHz HSI16` *(Primary here)*** | Reset default | `EXTICR[3]=0x20`, `RTSR1=0x2000`, `IMR1=0x2000` | `EXTI->PR1 = EXTI_PR1_PIF13; __DSB();` | `~750 ns` |
| **`160 MHz Range 1 Boost`** | `Clock_Init_160MHz()` | **Identical** (`EXTI` is clock-independent) | `EXTI->PR1 = EXTI_PR1_PIF13; __DSB();` | `~75.0 ns` |
| **`170 MHz Range 1 Boost`** | `Clock_Init_170MHz()` | **Identical** (`EXTI` is clock-independent) | `EXTI->PR1 = EXTI_PR1_PIF13; __DSB();` | `~70.6 ns` |

- **Reusable Production Driver (`23-code-library/Drivers/exti_g4.h`):** Replace boilerplate setup with `GPIO_ConfigInput(GPIOC, 13U, GPIO_PULL_NONE)`, `EXTI_RegisterCallback(13U, on_btn, NULL)`, `EXTI_ConfigLine(GPIOC, 13U, EXTI_EDGE_RISING, 3U)`, and call `EXTI_DispatchRange(10U, 15U)` inside `EXTI15_10_IRQHandler()` (includes `EXTI_ClearPending()` + `__DSB()` automatically).

> [!IMPORTANT]
> **Why `__DSB()` Is Mandatory Before ISR Exit on Cortex-M4:** Peripheral writes to APB2 (`EXTI->PR1 = EXTI_PR1_PIF13`) traverse the Cortex-M4 write buffer and AHB-to-APB2 bridge. If the ISR executes `BX LR` before the write reaches the EXTI peripheral, NVIC still sees `EXTI13` asserted and immediately tail-chains a **spurious duplicate ISR entry**! Executing `__DSB()` forces the write to complete before exception return.

---

## 6. Code Implementation

### 6.1 Approach A — Direct Register Access (CMSIS Bare-Metal)

```c
#include "stm32g474xx.h"
#include <stdint.h>

static volatile uint32_t g_button_irq_count = 0U;

void EXTI15_10_IRQHandler(void) {
    if ((EXTI->PR1 & EXTI_PR1_PIF13) != 0U) {
        /* 1. Clear pending flag via direct write-1 (rc_w1) — NEVER use |= */
        EXTI->PR1 = EXTI_PR1_PIF13;

        /* 2. Atomic LED toggle via BSRR (never non-atomic ODR ^= inside an ISR!) */
        if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
            GPIOA->BSRR = GPIO_BSRR_BR5;
        } else {
            GPIOA->BSRR = GPIO_BSRR_BS5;
        }

        g_button_irq_count++;

        /* 3. Data Synchronization Barrier: ensure PR1 clear reaches APB2
         *    before exception return to prevent spurious re-entry */
        __DSB();
    }
}

int main(void) {
    /* 1. Enable GPIOA, GPIOC (AHB2) and SYSCFG (APB2) clocks + sync readbacks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    volatile uint32_t tmpreg = RCC->APB2ENR;
    (void)tmpreg;
    __DSB();

    /* 2. PA5 -> Push-Pull Output */
    GPIOA->MODER  &= ~(3U << (5U * 2U));
    GPIOA->MODER  |=  (1U << (5U * 2U));
    GPIOA->OTYPER &= ~(1U << 5U);

    /* 3. PC13 -> Digital Input (00b), No Pull (external pull-down on NUCLEO-G474RE) */
    GPIOC->MODER &= ~(3U << (13U * 2U));
    GPIOC->PUPDR &= ~(3U << (13U * 2U));

    /* 4. Map PC13 to EXTI13 in SYSCFG->EXTICR[3] bits [7:4] = 0010b (Port C) */
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13_Msk;
    SYSCFG->EXTICR[3] |=  SYSCFG_EXTICR4_EXTI13_PC;

    /* 5. Configure EXTI13 for Rising Edge (Active-HIGH B1 press) */
    EXTI->RTSR1 |=  EXTI_RTSR1_RT13;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;
    EXTI->PR1    =  EXTI_PR1_PIF13; /* Clear any stale pending flag */
    EXTI->IMR1  |=  EXTI_IMR1_IM13;

    /* 6. Configure NVIC: 4 preemption priority bits, 0 subpriority bits */
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

```c
/* Requires stm32g4xx_ll_bus.h, stm32g4xx_ll_system.h, stm32g4xx_ll_exti.h */
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_gpio.h"

void Lab05_LL_EXTI13_Init(void) {
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_NO);
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13);

    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_13);
    LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_13);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_13);

    NVIC_SetPriority(EXTI15_10_IRQn, 3U);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}
```

### 6.3 Approach C — STM32Cube HAL Comparison

```c
/* Requires stm32g4xx_hal.h; EXTI15_10_IRQHandler calls HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13) */
#include "stm32g4xx_hal.h"

void Lab05_HAL_EXTI13_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_IT_RISING; /* Automatically enables SYSCFG + EXTICR + RTSR1 + IMR1 */
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3U, 0U);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_13) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
```

### 6.4 3-Way Comparison Summary

- **Direct Register:** Explicit control of `PR1` (`rc_w1`), `SYSCFGEN`, and `__DSB()`; minimum interrupt latency (12 cycles hardware stacking + ~8 cycles body).
- **STM32 LL:** Identical runtime latency to Direct Register when compiled with `-O2`, while providing self-documenting macro names (`LL_SYSCFG_EXTI_PORTC`).
- **STM32 HAL:** Automatically enables `SYSCFGEN` inside `HAL_GPIO_Init()`, but `HAL_GPIO_EXTI_IRQHandler()` adds function call overhead and weak-callback indirection.

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
| Occasional double-trigger right after clearing `PR1` | Posted APB2 write did not retire before ISR exit | Add `__DSB();` before leaving `EXTI15_10_IRQHandler` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `SYSCFG->EXTICR[3]` bits `[7:4]` equal `2` (Port C) and `EXTI->RTSR1` bit 13 equals `1`.
- [ ] Pressing `B1` increments `g_button_irq_count` on press (not release).
- [ ] ISR uses atomic `GPIOA->BSRR` and ends with `__DSB()`.

---

## 11. Extension Exercises

1. **ISR-Safe Debouncing:** Instead of toggling the LED directly in `EXTI15_10_IRQHandler`, mask `EXTI13` inside the ISR and start a 25 ms one-shot timer (TIM6/SysTick) that re-samples `PC13` and re-enables `EXTI13` once bounce settles.
2. **Nested Priority Preemption:** Add a higher-priority `SysTick` (Priority `1`) and lower-priority `EXTI15_10_IRQn` (Priority `3`), and inspect the `IPSR` register in GDB during nested preemption.
3. **Zero-Latency Event Wakeup (`__WFE`):** Replace `EXTI->IMR1` with `EXTI->EMR1` and `__WFI()` with `__WFE()` to wake the CPU without stacking an exception frame.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` *(Self-Contained Onboard — Queued for Physical Execution)* |
| **Objective** | Route Active-HIGH `B1` (`PC13`) via `SYSCFG->EXTICR[3]` to rising-edge `EXTI13` (`EXTI15_10_IRQn`), toggle `LD2` (`PA5`) atomically via `BSRR`, and clear `rc_w1` `EXTI->PR1` with `__DSB()` while comparing Register vs LL vs HAL. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,284 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `SYSCFG`, `EXTI`, `NVIC`, `GPIOA` (`PA5` `LD2`), `GPIOC` (`PC13` `B1`). Zero external wiring required (also supports software self-test via `EXTI->SWIER1` / `EXTI_TriggerSoftware(13)`). |
| **Hardware Availability** | `ONBOARD — READY FOR AGENT 4` |
| **Pin Configuration** | `PC13`: `MODER[27:26] = 00b`, `PUPDR[27:26] = 00b`, `EXTICR[3][7:4] = 0010b` (Port C), Rising Edge `RTSR1[13] = 1`; `PA5`: `MODER[11:10] = 01b`, `OTYPER[5] = 0`. |
| **Expected Behavior** | CPU sleeps in `__WFI()` until `B1` rising edge (or software trigger `EXTI->SWIER1 = (1U << 13)`), enters `EXTI15_10_IRQHandler`, clears `PR1[13]`, toggles `LD2`, increments `g_button_irq_count`, and returns without spurious re-entry. |
| **Actual Observations** | Pending Agent 4 Onboard Test *(not yet executed on physical silicon)*. |
| **Register Values** | Reset: `RCC->APB2ENR = 0`, `SYSCFG->EXTICR[3] = 0x0000`, `EXTI->IMR1 = 0x1F800000` (lines 0..15 masked). Configured: `RCC->APB2ENR[0] = 1` (`SYSCFGEN`), `SYSCFG->EXTICR[3][7:4] = 0x2`, `EXTI->RTSR1[13] = 1`, `EXTI->FTSR1[13] = 0`, `EXTI->IMR1[13] = 1`, `NVIC` `PRIGROUP = 0`, `EXTI15_10_IRQn (40)` prio = `3`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` break on `EXTI15_10_IRQHandler`, inspect `p/x SYSCFG->EXTICR[3]`, `p/x EXTI->RTSR1`, `p/x EXTI->PR1`, and verify `p g_button_irq_count` increments cleanly. |
| **Remaining Limitations** | Raw mechanical button edge interrupt without timer blanking may trigger multiple counts on noisy tactile bounce (see Extension Exercise 1 and Lab 02 FSM). Physical board execution pending Agent 4. |
