# EXTI & SYSCFG Register Map (External Interrupts & System Config)

**Target MCU:** STM32G474RE (NUCLEO-G474RE MB1367)  
**SYSCFG Base:** `0x40010000` (`APB2PERIPH_BASE + 0x0000UL`)  
**EXTI Base:** `0x40010400` (`APB2PERIPH_BASE + 0x0400UL`)  
**Bus:** APB2  
**Reference Manual:** RM0440, Chapter 10 (*SYSCFG*) & Chapter 15 (*Extended interrupt and event controller (EXTI)*)  
**Concept IDs:** `PERIPH-EXTI-001`, `GPIO-FMP-001`, `STM32-SRAM-001`, `PROF-REL-001`, `PROF-REL-002`  
**Verification Status:** Every register offset and bit position verified directly against `stm32g474xx.h` and UM2505; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. Functional Architecture Overview
1. **SYSCFG** routes any GPIO port (`PA..PG`) for pin number $k \in [0..15]$ onto EXTI line $k$ via `SYSCFG->EXTICR[0..3]` (only one port per line number at a time). SYSCFG also controls CCM-SRAM hardware erase (`SCSR`), safety lockup/parity fault routing to Timer Break (`CFGR2`), and 1 KB CCM-SRAM write protection (`SWPR`).
2. **EXTI** manages **44 interrupt/event lines (Lines 0–43)**: Lines `0–15` are GPIO external interrupts; Lines `16–43` connect internal wakeup/event sources (PVD=16, RTC Alarm=17, USB Wakeup=18, TAMP/LSECSS=19, RTC WKUP=20, COMP1–7=21/22/29–33, I2C/USART/LPUART/LPTIM/PVM/UCPD).
3. `SYSCFG` requires `RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN` before accessing its registers; EXTI registers are always clocked on APB2 and its asynchronous edge latches operate in Stop 0 / Stop 1 modes.

---

### 2. SYSCFG Register Overview (`SYSCFG_TypeDef` @ `0x40010000`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `SYSCFG_MEMRMP` | `0x000` | Boot-dependent | RW | Memory remap register (`MEM_MODE[2:0]`, `FB_MODE` bit 8 dual-bank swap status) |
| `SYSCFG_CFGR1` | `0x004` | `0x7C000001` | RW | Config 1: `BOOSTEN` (b8), `ANASWVDD` (b9), **`I2C_PB6_FMP` (b16)**, **`I2C_PB7_FMP` (b17)**, **`I2C_PB8_FMP` (b18)**, **`I2C_PB9_FMP` (b19)**, **`I2C1_FMP` (b20)**, **`I2C2_FMP` (b21)**, **`I2C3_FMP` (b22)**, **`I2C4_FMP` (b23)** (20 mA Fm+ drive on `FT_f` pads — `GPIO-FMP-001`), `FPU_IE[31:26]` |
| `SYSCFG_EXTICR1` (`EXTICR[0]`) | `0x008` | `0x00000000` | RW | EXTI Lines 0–3 port selector (4 bits/line: `0=PA`, `1=PB`, `2=PC`, `3=PD`, `4=PE` [unbonded on LQFP64], `5=PF`, `6=PG`) |
| `SYSCFG_EXTICR2` (`EXTICR[1]`) | `0x00C` | `0x00000000` | RW | EXTI Lines 4–7 port selector |
| `SYSCFG_EXTICR3` (`EXTICR[2]`) | `0x010` | `0x00000000` | RW | EXTI Lines 8–11 port selector |
| `SYSCFG_EXTICR4` (`EXTICR[3]`) | `0x014` | `0x00000000` | RW | EXTI Lines 12–15 port selector |
| `SYSCFG_SCSR` | `0x018` | `0x00000000` | RW | CCM-SRAM control & status (`CCMER` bit 0 hardware erase, `CCMBSY` bit 1 busy) |
| `SYSCFG_CFGR2` | `0x01C` | `0x00000000` | RW | Safety config 2 (`CLL` Cortex lockup→TIM break, `SPL` SRAM parity→TIM break, `PVDL` PVD→TIM break, `ECCL` Flash ECC→TIM break, **`SPF` bit 8** SRAM parity error flag `rc_w1`) |
| `SYSCFG_SWPR` | `0x020` | `0x00000000` | RW (write-1-once) | CCM-SRAM write protection (32 bits = 32 × 1 KB pages; once set, locked until MCU reset) |
| `SYSCFG_SKR` | `0x024` | `0x00000000` | WO | CCM-SRAM key register (write `0xCA` then `0x53` to unlock `SCSR.CCMER`) |

---

### 3. EXTI Register Overview (`EXTI_TypeDef` @ `0x40010400`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `EXTI_IMR1` | `0x000` | `0xFF820000` | RW | Interrupt mask register 1 (Lines 0–31; `1` = unmasked) |
| `EXTI_EMR1` | `0x004` | `0x00000000` | RW | Event mask register 1 (Lines 0–31; wakes WFE without ISR) |
| `EXTI_RTSR1` | `0x008` | `0x00000000` | RW | Rising trigger selection register 1 (Lines 0–31) |
| `EXTI_FTSR1` | `0x00C` | `0x00000000` | RW | Falling trigger selection register 1 (Lines 0–31) |
| `EXTI_SWIER1` | `0x010` | `0x00000000` | RW | Software interrupt event register 1 (`1` = trigger pending latch in software) |
| `EXTI_PR1` | `0x014` | `0x00000000` | **rc_w1** | Pending register 1 (**Direct write `EXTI->PR1 = mask;` — NEVER `|=`!** Follow with `(void)EXTI->PR1; __DSB();` in ISRs per ARM AN321 §4.9 & ES0430 §2.1.3) |
| `EXTI_IMR2` | `0x020` | `0x00000187` | RW | Interrupt mask register 2 (Lines 32–43) |
| `EXTI_EMR2` | `0x024` | `0x00000000` | RW | Event mask register 2 (Lines 32–43) |
| `EXTI_RTSR2` | `0x028` | `0x00000000` | RW | Rising trigger selection register 2 (Lines 32–43) |
| `EXTI_FTSR2` | `0x02C` | `0x00000000` | RW | Falling trigger selection register 2 (Lines 32–43) |
| `EXTI_SWIER2` | `0x030` | `0x00000000` | RW | Software interrupt event register 2 (Lines 32–43) |
| `EXTI_PR2` | `0x034` | `0x00000000` | **rc_w1** | Pending register 2 (Lines 32–43; direct write `EXTI->PR2 = mask;` — NEVER `|=`!) |

**NVIC IRQ Mapping (`stm32g474xx.h`):** `EXTI0_IRQn = 6`, `EXTI1_IRQn = 7`, `EXTI2_IRQn = 8`, `EXTI3_IRQn = 9`, `EXTI4_IRQn = 10`, `EXTI9_5_IRQn = 23`, `EXTI15_10_IRQn = 40`.

---

### 4. CMSIS Code Example: NUCLEO-G474RE User Button B1 (`PC13`) Rising-Edge Interrupt

> **UM2505 Hardware Polarity Verification:** On NUCLEO-G474RE (MB1367), User Button B1 (`PC13`) has an **external pull-down** and connects to `VDD` when pressed (**Active-HIGH → Rising Edge `RTSR1`**). Do **not** enable an internal pull-up (it fights the external pull-down).  
> **Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`; Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

void EXTI13_ButtonB1_Init(void) {
    /* 1. Enable GPIOC (AHB2) and SYSCFG (APB2) clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    /* 2. Configure PC13 as Digital Input (MODER13 = 00b), No Internal Pull (external PD on MB1367) */
    GPIOC->MODER &= ~(3U << (13U * 2U));
    GPIOC->PUPDR &= ~(3U << (13U * 2U));

    /* 3. Route PC13 to EXTI13 in SYSCFG_EXTICR4 (EXTICR[3] bits [7:4] = 0010b for Port C) */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~SYSCFG_EXTICR4_EXTI13_Msk) |
                        SYSCFG_EXTICR4_EXTI13_PC;

    /* 4. Configure Active-HIGH Rising Edge Trigger (RTSR1) on EXTI Line 13 */
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;
    EXTI->RTSR1 |=  EXTI_RTSR1_RT13;

    /* 5. Clear any stale pending flag (rc_w1 direct assignment) and unmask Line 13 */
    EXTI->PR1   = EXTI_PR1_PIF13;
    EXTI->IMR1 |= EXTI_IMR1_IM13;

    /* 6. Configure priority and enable EXTI15_10_IRQn (IRQ 40) in NVIC */
    NVIC_SetPriority(EXTI15_10_IRQn, 2U);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF13) {
        /* Clear pending flag via direct write (rc_w1 — NEVER use |=) */
        EXTI->PR1 = EXTI_PR1_PIF13;

        /* Sibling-pin-safe single-writer PA5 toggle via atomic BSRR set/reset
         * (avoids ODR RMW corruption of sibling pins; concurrent same-pin writers need a critical section) */
        if (GPIOA->ODR & (1U << 5U)) {
            GPIOA->BSRR = GPIO_BSRR_BR5;
        } else {
            GPIOA->BSRR = GPIO_BSRR_BS5;
        }
        /* 1. Dummy readback guarantees APB2 clear reaches EXTI slave (ARM AN321 §4.9).
         * 2. __DSB() drains core write buffer & mitigates ARM Erratum 838869 (ES0430 §2.1.3). */
        (void)EXTI->PR1;
        __DSB();
    }
}
```
