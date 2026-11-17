# EXTI & SYSCFG Register Map (External Interrupts)

**Target MCU:** STM32G474RE  
**SYSCFG Base:** `0x40010000` (`APB2PERIPH_BASE + 0x0000UL`)  
**EXTI Base:** `0x40010400` (`APB2PERIPH_BASE + 0x0400UL`)  
**Bus:** APB2  
**Reference Manual:** RM0440, Section 13 (*SYSCFG*) & Section 14 (*EXTI*)

---

### 1. Functional Architecture Overview
The Extended Interrupt and Event Controller (EXTI) manages asynchronous events from I/O pins and internal peripherals.
1. The 16 physical I/O pins of each port (PA0-PG0, PA1-PG1, ..., PA15-PG15) are multiplexed onto 16 EXTI lines via the **SYSCFG External Interrupt Configuration Registers (`SYSCFG_EXTICR1-4`)**.
2. Only one pin of a given number can trigger an EXTI line at any time (e.g., you can use `PA0` OR `PB0` as an interrupt source, but not both simultaneously).
3. The selected EXTI line feeds edge detection circuitry (configurable for Rising, Falling, or Both edges).
4. When triggered, the EXTI line sets its bit in `EXTI_PR1` and asserts the corresponding IRQ line to the Cortex-M4 NVIC.

---

### 2. SYSCFG Register Overview

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `SYSCFG_MEMRMP` | `0x00` | `0x00000000` | Memory remap register |
| `SYSCFG_CFGR1` | `0x04` | `0x00000000` | Configuration register 1 |
| `SYSCFG_EXTICR1`| `0x08` | `0x00000000` | External interrupt configuration register 1 (Lines 0 to 3) |
| `SYSCFG_EXTICR2`| `0x0C` | `0x00000000` | External interrupt configuration register 2 (Lines 4 to 7) |
| `SYSCFG_EXTICR3`| `0x10` | `0x00000000` | External interrupt configuration register 3 (Lines 8 to 11) |
| `SYSCFG_EXTICR4`| `0x14` | `0x00000000` | External interrupt configuration register 4 (Lines 12 to 15) |

#### `SYSCFG_EXTICRx` Bitfield Encoding
Each register configures 4 EXTI lines using 4 bits per line:
- `0000`: Port A selected
- `0001`: Port B selected
- `0010`: Port C selected (e.g., PC13 Blue Button on NUCLEO-G474RE)
- `0011`: Port D selected

---

### 3. EXTI Register Overview

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `EXTI_IMR1` | `0x00` | `0xFF820000` | Interrupt mask register 1 (Lines 0 to 31; 1 = unmasked) |
| `EXTI_EMR1` | `0x04` | `0x00000000` | Event mask register 1 |
| `EXTI_RTSR1`| `0x08` | `0x00000000` | Rising trigger selection register 1 (1 = enable rising edge trigger) |
| `EXTI_FTSR1`| `0x0C` | `0x00000000` | Falling trigger selection register 1 (1 = enable falling edge trigger) |
| `EXTI_SWIER1`| `0x10`| `0x00000000` | Software interrupt event register 1 (1 = generate software trigger) |
| `EXTI_PR1` | `0x14` | `0x00000000` | Pending register 1 (**rc_w1**: write `1` to clear pending flag!) |

---

### 4. CMSIS Code Example: Configure PC13 User Pushbutton Interrupt
```c
/* 1. Enable GPIOC clock (AHB2) and SYSCFG clock (APB2) */
RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

/* 2. Configure PC13 as Digital Input with Pull-up */
GPIOC->MODER &= ~(3U << (13U * 2U));
GPIOC->PUPDR &= ~(3U << (13U * 2U));
GPIOC->PUPDR |=  (1U << (13U * 2U)); /* Pull-up */

/* 3. Route PC13 to EXTI13 in SYSCFG_EXTICR4 (Bits [7:4] = 0010b for Port C) */
SYSCFG->EXTICR[3] &= ~(0xFU << 4U);
SYSCFG->EXTICR[3] |=  (2U   << 4U);

/* 4. Configure Falling Edge Trigger on EXTI Line 13 (Button press pulls LOW) */
EXTI->RTSR1 &= ~(1U << 13U);
EXTI->FTSR1 |=  (1U << 13U);

/* 5. Unmask Interrupt on Line 13 */
EXTI->IMR1 |= (1U << 13U);

/* 6. Enable Interrupt in NVIC (EXTI15_10_IRQn on STM32G4) */
NVIC_SetPriority(EXTI15_10_IRQn, 2);
NVIC_EnableIRQ(EXTI15_10_IRQn);

/* 7. Interrupt Service Routine Implementation */
void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR1 & (1U << 13U)) {
        /* CRITICAL: Clear pending flag by writing '1' to it (rc_w1) */
        EXTI->PR1 = (1U << 13U);
        
        /* User Application Action: Toggle LED on PA5 */
        GPIOA->ODR ^= (1U << 5U);
    }
}
```
