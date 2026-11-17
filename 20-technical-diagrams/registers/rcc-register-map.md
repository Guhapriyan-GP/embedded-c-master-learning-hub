# RCC (Reset and Clock Control) Register Map

**Target MCU:** STM32G474RE  
**Base Address:** `0x40021000` (`AHB1PERIPH_BASE + 0x1000UL`)  
**Bus:** AHB1  
**Reference Manual:** RM0440, Section 6.4 (*RCC registers*)

---

### 1. Register Overview Table

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `RCC_CR` | `0x00` | `0x00000063` | Clock control register (HSI, HSE, PLL enable/ready flags) |
| `RCC_ICSCR` | `0x04` | `0x40000000` | Internal clock sources calibration register |
| `RCC_CFGR` | `0x08` | `0x00000005` | Clock configuration register (SYSCLK switch, AHB/APB prescalers, MCO) |
| `RCC_PLLCFGR` | `0x0C` | `0x00001000` | PLL configuration register (Source, PLLM, PLLN, PLLR, PLLQ, PLLP) |
| `RCC_CIER` | `0x18` | `0x00000000` | Clock interrupt enable register |
| `RCC_CIFR` | `0x1C` | `0x00000000` | Clock interrupt flag register |
| `RCC_AHB1ENR` | `0x48` | `0x00000100` | AHB1 peripheral clock enable register (DMA1, DMA2, DMAMUX, Flash, CRC) |
| `RCC_AHB2ENR` | `0x4C` | `0x00000000` | AHB2 peripheral clock enable register (GPIOA-G, ADC1-5, DAC1-4) |
| `RCC_APB1ENR1` | `0x58` | `0x00000000` | APB1 peripheral clock enable register 1 (TIM2-7, USART2-3, I2C1-3, PWR) |
| `RCC_APB2ENR` | `0x60` | `0x00000000` | APB2 peripheral clock enable register (SYSCFG, TIM1, SPI1, USART1, TIM8) |

---

### 2. Critical Register Bitfields

#### A. `RCC_CR` (Clock Control Register) — Offset `0x00`
- **Bit 0 (`HSION`):** HSI16 clock enable (RW). Reset: `1` (HSI16 is ON after reset).
- **Bit 1 (`HSIRDY`):** HSI16 clock ready flag (RO). Set by hardware when HSI16 oscillator is stable.
- **Bit 16 (`HSEON`):** HSE crystal clock enable (RW). Reset: `0`.
- **Bit 17 (`HSERDY`):** HSE crystal clock ready flag (RO).
- **Bit 24 (`PLLON`):** Main PLL enable (RW). Reset: `0`. Must remain disabled while configuring `RCC_PLLCFGR`.
- **Bit 25 (`PLLRDY`):** Main PLL clock ready flag (RO). Hardware sets this bit when the PLL is locked.

#### B. `RCC_CFGR` (Clock Configuration Register) — Offset `0x08`
- **Bits [1:0] (`SW`):** System clock switch (RW).
  - `00`: HSI16 selected as SYSCLK
  - `01`: HSE selected as SYSCLK
  - `11`: PLL "R" clock selected as SYSCLK
- **Bits [3:2] (`SWS`):** System clock switch status (RO). Confirms currently active SYSCLK source.
- **Bits [7:4] (`HPRE`):** AHB prescaler (RW).
  - `0000`: SYSCLK not divided (`HCLK = SYSCLK`)
  - `1000`: SYSCLK divided by 2
  - `1001`: SYSCLK divided by 4
- **Bits [10:8] (`PPRE1`):** APB1 prescaler (RW). `000`: HCLK not divided (`PCLK1 = HCLK`).
- **Bits [13:11] (`PPRE2`):** APB2 prescaler (RW). `000`: HCLK not divided (`PCLK2 = HCLK`).

#### C. `RCC_PLLCFGR` (PLL Configuration Register) — Offset `0x0C`
- **Bits [1:0] (`PLLSRC`):** PLL input clock source (`00`: No clock, `10`: HSI16, `11`: HSE).
- **Bits [7:4] (`PLLM`):** Division factor $M$ for PLL input clock (`0000`: Div 1, `0001`: Div 2 ... $\text{Divider} = M + 1$).
- **Bits [14:8] (`PLLN`):** Main PLL multiplication factor $N$ (Range: 8 to 127).
- **Bit 24 (`PLLREN`):** Main PLL "R" clock output enable (Must be set to `1` to drive SYSCLK).
- **Bits [26:25] (`PLLR`):** Main PLL division factor $R$ for SYSCLK (`00`: Div 2, `01`: Div 4, `10`: Div 6, `11`: Div 8).

---

### 3. CMSIS Code Example: Configuring 170 MHz SYSCLK
```c
/* 1. Enable HSI16 and wait for ready flag */
RCC->CR |= RCC_CR_HSION;
while (!(RCC->CR & RCC_CR_HSIRDY));

/* 2. Configure PLL: Source = HSI16 (16 MHz), M = 4, N = 85, R = 2
 *    VCO_in = 16 MHz / 4 = 4 MHz
 *    VCO_out = 4 MHz * 85 = 340 MHz
 *    PLL_R (SYSCLK) = 340 MHz / 2 = 170 MHz */
RCC->PLLCFGR = (2U << RCC_PLLCFGR_PLLSRC_Pos)   |  /* HSI16 source */
               ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos) |  /* M = 4 */
               (85U << RCC_PLLCFGR_PLLN_Pos)     |  /* N = 85 */
               (0U << RCC_PLLCFGR_PLLR_Pos)      |  /* R = 2 (00b) */
               RCC_PLLCFGR_PLLREN;                  /* Enable PLL_R output */

/* 3. Enable PLL and wait for lock */
RCC->CR |= RCC_CR_PLLON;
while (!(RCC->CR & RCC_CR_PLLRDY));

/* 4. Switch SYSCLK to PLL */
RCC->CFGR &= ~RCC_CFGR_SW;
RCC->CFGR |= RCC_CFGR_SW_PLL;
while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
```
