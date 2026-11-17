# Timers (TIMx) Register Map & PWM Engine

**Target MCU:** STM32G474RE  
**TIM2 Base:** `0x40000000` (APB1, 32-bit General Purpose)  
**TIM3 Base:** `0x40000400` (APB1, 16-bit General Purpose)  
**TIM1 Base:** `0x40012C00` (APB2, 16-bit Advanced Motor Control)  
**Reference Manual:** RM0440, Chapters 21-29 (*Timers*)

---

### 1. General-Purpose Timer Register Overview

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `TIMx_CR1` | `0x00` | `0x00000000` | Control register 1 (CEN counter enable, DIR direction, ARPE auto-reload preload) |
| `TIMx_CR2` | `0x04` | `0x00000000` | Control register 2 (MMS Master Mode Selection: Reset, Enable, Update, Compare) |
| `TIMx_SMCR` | `0x08` | `0x00000000` | Slave mode control register (Encoder mode, external trigger, reset mode) |
| `TIMx_DIER` | `0x0C` | `0x00000000` | DMA/Interrupt enable register (UIE update interrupt enable, CCxIE, CCxDE) |
| `TIMx_SR` | `0x10` | `0x00000000` | Status register (UIF update interrupt flag, CCxIF capture/compare flags) |
| `TIMx_EGR` | `0x14` | `0x00000000` | Event generation register (UG bit forces immediate shadow transfer) |
| `TIMx_CCMR1`| `0x18` | `0x00000000` | Capture/compare mode register 1 (Channels 1 & 2: PWM mode selection) |
| `TIMx_CCMR2`| `0x1C` | `0x00000000` | Capture/compare mode register 2 (Channels 3 & 4) |
| `TIMx_CCER` | `0x20` | `0x00000000` | Capture/compare enable register (CCxE output enable, CCxP polarity) |
| `TIMx_CNT` | `0x24` | `0x00000000` | Counter register (Current count value) |
| `TIMx_PSC` | `0x28` | `0x00000000` | Prescaler register (Divides timer clock by $\text{PSC} + 1$) |
| `TIMx_ARR` | `0x2C` | `0xFFFFFFFF` | Auto-reload register (Period register defining overflow value) |
| `TIMx_CCR1` | `0x34` | `0x00000000` | Capture/compare register 1 (Duty cycle value for Channel 1) |

---

### 2. Timebase & Frequency Calculations

The timer update event frequency ($f_{\text{UEV}}$) is derived from the peripheral timer clock ($f_{\text{TIM\_CLK}}$):

$$f_{\text{UEV}} = \frac{f_{\text{TIM\_CLK}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}$$

**PWM Duty Cycle Equation (Edge-aligned, PWM Mode 1):**

$$\text{Duty Cycle (\%)} = \left( \frac{\text{CCR}}{\text{ARR} + 1} \right) \times 100\%$$

*(Note on STM32G4 Clock Multiplier: If the APB prescaler is 1, $f_{\text{TIM\_CLK}} = f_{\text{PCLK}}$. If the APB prescaler is $>1$, the hardware automatically multiplies the APB clock by 2 for the timer).*

---

### 3. Critical Bitfields for PWM Generation

#### A. `TIMx_CR1` — Offset `0x00`
- **Bit 0 (`CEN`):** Counter enable (1 = start counting).
- **Bit 7 (`ARPE`):** Auto-reload preload enable (1 = `ARR` register is buffered through shadow register).

#### B. `TIMx_CCMR1` — Offset `0x18` (Channel 1)
- **Bits [1:0] (`CC1S`):** Capture/Compare 1 selection (`00` = Channel configured as Output).
- **Bit 3 (`OC1PE`):** Output compare 1 preload enable (1 = `CCR1` uses shadow register; prevents mid-cycle glitching).
- **Bits [6:4] (`OC1M`):** Output compare 1 mode:
  - `110`: **PWM Mode 1** (Channel 1 is active/HIGH while $\text{CNT} < \text{CCR1}$, else inactive/LOW).
  - `111`: **PWM Mode 2** (Channel 1 is inactive/LOW while $\text{CNT} < \text{CCR1}$, else active/HIGH).

#### C. `TIMx_CCER` — Offset `0x20`
- **Bit 0 (`CC1E`):** Capture/Compare 1 output enable (1 = route output signal to external pin).
- **Bit 1 (`CC1P`):** Capture/Compare 1 output polarity (0 = Active HIGH, 1 = Active LOW).

---

### 4. CMSIS Code Example: 20 kHz PWM on TIM2 Channel 1 (PA0)
```c
/* Assumptions: SYSCLK = HCLK = PCLK1 = 170 MHz */

/* 1. Enable TIM2 (APB1) and GPIOA (AHB2) bus clocks */
RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

/* 2. Configure PA0 to Alternate Function AF1 (TIM2_CH1) */
GPIOA->MODER &= ~(3U << (0U * 2U));
GPIOA->MODER |=  (2U << (0U * 2U));
GPIOA->AFR[0] &= ~(0xFU << (0U * 4U));
GPIOA->AFR[0] |=  (1U   << (0U * 4U)); /* AF1 = TIM2_CH1 */

/* 3. Configure Timebase for 20 kHz PWM:
 *    Target: 20 kHz (Period = 50 microseconds)
 *    f_TIM = 170 MHz. Let PSC = 0 (Divide by 1).
 *    ARR = (170,000,000 / 20,000) - 1 = 8500 - 1 = 8499 */
TIM2->PSC = 0U;
TIM2->ARR = 8499U;

/* 4. Configure 50% Duty Cycle: CCR1 = 8500 / 2 = 4250 */
TIM2->CCR1 = 4250U;

/* 5. Configure Channel 1 for PWM Mode 1 with Preload */
TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;      /* Output mode */
TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
TIM2->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos); /* PWM Mode 1 (110b) */
TIM2->CCMR1 |= TIM_CCMR1_OC1PE;     /* Preload enable */

/* 6. Enable Channel 1 Output in CCER */
TIM2->CCER |= TIM_CCER_CC1E;

/* 7. Enable Auto-reload Preload (ARPE) and Start Counter (CEN) */
TIM2->CR1 |= TIM_CR1_ARPE;
TIM2->EGR |= TIM_EGR_UG;            /* Force register update */
TIM2->CR1 |= TIM_CR1_CEN;            /* Enable timer */
```
