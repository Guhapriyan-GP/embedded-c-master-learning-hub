# STM32G474RE Register Quick Reference (1-Page-Per-Subsystem Desk Card)

> **100% Ground-Truth Verified:** Every base address, offset, IRQ number, reset value, and access rule below is verified directly against [`stm32g474xx.h`](../23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h), [`core_cm4.h`](../23-code-library/Drivers/CMSIS/Core/Include/core_cm4.h), **RM0440**, and [`20-technical-diagrams/registers/`](../20-technical-diagrams/registers/rcc-register-map.md).
> **Access Legend:** `rw` = Read/Write | `r` = Read-Only | `w` = Write-Only (direct `=`) | **`rc_w1`** = Write `1` to clear (direct `=`, **NEVER `|=`**) | **`rc_w0`** = Write `0` to clear (direct `= ~mask`, **NEVER `&=`**).

---

## 1. RCC (`0x40021000`, AHB1)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `RCC->CR` | `0x00` | `0x00000100` | `rw`/`r` | **`HSION` b8**, **`HSIRDY` b10** *(NOT b0/b1 like F4!)*, `HSEON` b16, `HSERDY` b17, `HSEBYP` b18, `PLLON` b24, `PLLRDY` b25 |
| `RCC->CFGR` | `0x08` | `0x00000005` | `rw`/`r` | `SW[1:0]` (`00`=Res, `01`=HSI, `10`=HSE, `11`=PLL), `SWS[3:2]`, `HPRE[7:4]` (`1000`=`/2`), `PPRE1[10:8]`, `PPRE2[13:11]`, `MCOSEL[27:24]` |
| `RCC->PLLCFGR` | `0x0C` | `0x00001000` | `rw` | `PLLSRC[1:0]` (`10`=HSI, `11`=HSE), `PLLM[7:4]` ($M-1$), `PLLN[14:8]`, `PLLPEN` b16, `PLLQEN` b20, **`PLLREN` b24**, `PLLR[26:25]`, `PLLPDIV[31:27]` |
| `RCC->AHB1ENR` | `0x48` | `0x00000100` | `rw` | `DMA1EN` b0, `DMA2EN` b1, **`DMAMUX1EN` b2**, `CORDICEN` b3, `FMACEN` b4, `CRCEN` b12 |
| `RCC->AHB2ENR` | `0x4C` | `0x00000000` | `rw` | `GPIOA..GEN` b0..6, `ADC12EN` b13, `ADC345EN` b14, **`DAC1..4EN` b16..19** *(DAC is AHB2!)*, `RNGEN` b26 |
| `RCC->APB1ENR1` | `0x58` | `0x00000400` | `rw` | `TIM2..7EN` b0..5, `WWDGEN` b11, `SPI2/3EN` b14..15, `USART2/3EN` b17..18, `I2C1..3EN` b21..22/30, `FDCANEN` b25, **`PWREN` b28**, `LPTIM1EN` b31 |
| `RCC->APB2ENR` | `0x60` | `0x00000000` | `rw` | **`SYSCFGEN` b0**, `TIM1EN` b11, `SPI1EN` b12, `TIM8EN` b13, `USART1EN` b14, `HRTIM1EN` b26 |
| `RCC->CCIPR` | `0x88` | `0x00000000` | `rw` | Independent kernel clocks (`USARTxSEL`, `LPUART1SEL`, `I2CxSEL`, `FDCANSEL`, `CLK48SEL`, `ADC12SEL`) |
| `RCC->CSR` | `0x94` | `0x0C000000` | `rw`/`r` | `LSION` b0, `LSIRDY` b1, `RMVF` b23, reset flags `[31:25]` (`IWDGRSTF` b29, `WWDGRSTF` b30) |

**Gotcha:** Always do a dummy readback `(void)RCC->AHB2ENR;` after enabling any peripheral clock!

---

## 2. PWR (`0x40007000`, APB1) & FLASH (`0x40022000`, AHB1)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `PWR->CR1` | `0x00` | `0x00000200` | `rw` | `LPMS[2:0]` (`000`=Stop0, `001`=Stop1, `011`=Standby, `1xx`=Shutdown — *no Stop 2!*), `DBP` b8, `VOS[10:9]` (`01`=Range1) |
| `PWR->CR3` | `0x08` | `0x00008000` | `rw` | `RRS` b8 (SRAM2 retain), **`UCPD_DBDIS` b14** *(Set `1` to disable 5.1 kΩ pull-downs on `PA9/PA10/PB6/PB4`!)* |
| `PWR->SR2` | **`0x14`** | `0x00000000` | `r` | **`VOSF` b10** (`0` = Voltage scaling ready; lives in `SR2`, NOT `SR1`!) |
| **`PWR->CR5`** | **`0x80`** | `0x00000100` | `rw` | **`R1MODE` b8** (`0` = Range 1 Boost >150 MHz; `1` = Range 1 Normal ≤150 MHz) |
| `FLASH->ACR` | `0x00` | `0x00040600` | `rw` | **`LATENCY[3:0]` (`4WS` for 136–170 MHz Boost)**, `PRFTEN` b8, `ICEN` b9, `DCEN` b10, `DCRST` b12 *(ES0430 flush!)* |
| `FLASH->SR` | `0x10` | `0x00000000` | `rc_w1`/`r` | Error flags `[15:0]`, `BSY` b16 |
| `FLASH->CR` | `0x14` | `0xC0000000` | `rw` | `PG`, `PER`, `PNB[9:3]`, `BKER` b11, `STRT`, `OBL_LAUNCH`, `OPTLOCK`, `LOCK` (unlock via `KEYR`: `0x45670123`, `0xCDEF89AB`) |
| `FLASH->ECCR` | `0x18` | `0x00000000` | `rw`/`rc_w1` | `ECCC` b30 (1-bit corrected), `ECCD` b31 (2-bit NMI) |

---

## 3. GPIO (`GPIOA=0x48000000`, `GPIOB=0x48000400`, `GPIOC=0x48000800`, AHB2)
*Also `GPIOD=0x48000C00`, `GPIOF=0x48001400`, `GPIOG=0x48001800`. **`GPIOE` unbonded on LQFP64!***

| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `GPIOx->MODER` | `0x00` | `A:0xABFFFFFF` `C:0xFFFFFFFF` | `rw` | 2 bits/pin: `00`=Input, `01`=Output, `10`=AF, **`11`=Analog (STM32G4 RESET DEFAULT!)** |
| `GPIOx->OTYPER` | `0x04` | `0x00000000` | `rw` | 1 bit/pin: `0`=Push-Pull, `1`=Open-Drain |
| `GPIOx->OSPEEDR` | `0x08` | `A:0x0C000000` | `rw` | 2 bits/pin: `00`=Low, `01`=Med, `10`=High, `11`=Very High |
| `GPIOx->PUPDR` | `0x0C` | `A:0x64000000` | `rw` | 2 bits/pin: `00`=None, `01`=Pull-Up (~40 kΩ), `10`=Pull-Down (~40 kΩ) |
| `GPIOx->IDR` | `0x10` | `0x0000XXXX` | `r` | Input logic level (reads `0` if `MODER=11b`!) |
| `GPIOx->ODR` | `0x14` | `0x00000000` | `rw` | Output latch (avoid RMW in ISRs — use `BSRR`/`BRR`) |
| `GPIOx->BSRR` | `0x18` | `0x00000000` | **`w`** | Atomic set `[15:0]` / reset `[31:16]` (`BSy` wins over `BRy`) |
| `GPIOx->AFR[0..1]` | `0x20`/`0x24` | `0x00000000` | `rw` | 4 bits/pin (`AF0..AF15`): set **before** `MODER=10b` |
| `GPIOx->BRR` | `0x28` | `0x00000000` | **`w`** | Atomic reset `[15:0]` (no `+16` shift needed) |

---

## 4. SYSCFG (`0x40010000`, APB2) & EXTI (`0x40010400`, APB2)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `SYSCFG->CFGR1` | `0x04` | `0x7C000001` | `rw` | `I2C_PBx_FMP` / `I2Cx_FMP` (20 mA Fm+), `BOOSTEN`, `FPU_IE[31:26]` |
| `SYSCFG->EXTICR[0..3]` | `0x08..0x14` | `0x00000000` | `rw` | 4 bits/line (`0=PA`, `1=PB`, `2=PC`, `3=PD`, `5=PF`, `6=PG`); requires `RCC->APB2ENR.SYSCFGEN` |
| `EXTI->IMR1` | `0x00` | `0xFF820000` | `rw` | Interrupt unmask lines `0..31` (`1`=enabled) |
| `EXTI->RTSR1` / `FTSR1` | `0x08`/`0x0C` | `0x00000000` | `rw` | Rising / Falling trigger (`PC13` B1 = **Rising `RTSR1`**!) |
| `EXTI->PR1` | `0x14` | `0x00000000` | **`rc_w1`** | Pending flags `0..31`: **Direct `EXTI->PR1 = mask;` + `__DSB();`** (IRQ `EXTI15_10_IRQn = 40`) |

---

## 5. TIMx (`TIM2=0x40000000` 32b APB1, `TIM3=0x40000400` 16b APB1, `TIM1=0x40012C00` Adv APB2)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `TIMx->CR1` | `0x00` | `0x00000000` | `rw` | `CEN` b0, `UDIS` b1, `URS` b2, `OPM` b3, `DIR` b4, `CMS[6:5]`, `ARPE` b7 |
| `TIMx->DIER` | `0x0C` | `0x00000000` | `rw` | `UIE` b0, `CC1IE..CC4IE` b1..4, `UDE` b8 (`TIM2_IRQn=28`, `TIM3_IRQn=29`) |
| `TIMx->SR` | `0x10` | `0x00000000` | **`rc_w0`** | `UIF` b0, `CC1IF..CC4IF` b1..4 — **Direct `TIMx->SR = ~TIM_SR_UIF;` (NEVER `&=`)!** |
| `TIMx->EGR` | `0x14` | `0x00000000` | **`w`** | **`UG` b0** — Direct `TIMx->EGR = TIM_EGR_UG;` loads `PSC`/`ARR` shadows & sets `UIF` |
| `TIMx->CCMR1/2` | `0x18`/`0x1C` | `0x00000000` | `rw` | `OCxPE` (preload), `OCxM` (`110b` = PWM Mode 1) |
| `TIMx->CCER` | `0x20` | `0x00000000` | `rw` | `CCxE` output enable, `CCxNE` complementary enable |
| `TIMx->CNT`/`PSC`/`ARR` | `0x24`/`0x28`/`0x2C` | `0`/`0`/`0xFFFF(FFFF)` | `rw` | Counter, 16-bit Prescaler ($\div(\text{PSC}+1)$), Auto-Reload |
| `TIMx->CCR1..4` | `0x34..0x40` | `0x00000000` | `rw` | Capture/Compare duty registers |
| `TIMx->BDTR` | `0x44` | `0x00000000` | `rw` | `DTG[7:0]`, `BKE` b12, `AOE` b14, **`MOE` b15 (MUST set `1` on `TIM1/8/15/16/17/20`!)** |
| `TIMx->DCR` / `DMAR` | `0x3DC`/`0x3E0` | `0x00000000` | `rw` | DMA burst control/window *(STM32G4 offset shifted from F4!)* |

---

## 6. USART (`USART2=0x40004400` APB1, `USART1=0x40013800` APB2, `LPUART1=0x40008000` APB1)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `USARTx->CR1` | `0x00` | `0x00000000` | `rw` | `UE` b0, `RE` b2, `TE` b3, `IDLEIE` b4, `RXNEIE` b5, `TXEIE` b7, `OVER8` b15, `FIFOEN` b29 |
| `USARTx->BRR` | `0x0C` | `0x00000000` | `rw` | 16-bit divisor (`1476` = `0x05C4` @ 170 MHz / 115200); 20-bit on LPUART1 |
| `USARTx->ISR` | `0x1C` | `0x020000C0` | `r` | `PE` b0, `FE` b1, `NE` b2, **`ORE` b3** *(blocks RX until cleared!)*, `IDLE` b4, `RXNE` b5, `TC` b6, `TXE` b7 |
| `USARTx->ICR` | `0x20` | `0x00000000` | **`w`** | Clear flags (`ORECF`, `IDLECF`, `TCCF`, `FECF`) via direct write `=` |
| `USARTx->RDR` / `TDR` | `0x24`/`0x28` | `0x00000000` | `r`/`rw` | Separate Receive/Transmit data registers *(no legacy `DR`!)* (`USART2_IRQn=38`) |
| `USARTx->PRESC` | `0x2C` | `0x00000000` | `rw` | Input clock prescaler `/1`..`/256` |

---

## 7. SPI (`SPI1=0x40013000` APB2, `SPI2=0x40003800` APB1, `SPI3=0x40003C00` APB1)
*`SPI4` is unbonded on LQFP64!*

| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `SPIx->CR1` | `0x00` | `0x00000000` | `rw` | `CPHA` b0, `CPOL` b1, `MSTR` b2, `BR[5:3]`, `SPE` b6, `SSI` b8, `SSM` b9 |
| `SPIx->CR2` | `0x04` | `0x00000700` | `rw` | `DS[11:8]` (`0111`=8-bit), **`FRXTH` b12 (MUST set `1` for 8-bit RXNE!)** |
| `SPIx->SR` | `0x08` | `0x00000002` | `r`/`rc_w0` | `RXNE` b0, `TXE` b1, `OVR` b6, `BSY` b7 |
| `SPIx->DR` | `0x0C` | `0x00000000` | `rw` | **Use `*(__IO uint8_t *)&SPIx->DR` for 8-bit frames** (16-bit write packs 2 bytes!) |

---

## 8. I2C (`I2C1=0x40005400`, `I2C2=0x40005800`, `I2C3=0x40007800`, `I2C4=0x40008400`, APB1)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `I2Cx->CR1` | `0x00` | `0x00000000` | `rw` | `PE` b0 (configure `TIMINGR` while `PE=0`), `DNF[11:8]`, `ANFOFF` b12 |
| `I2Cx->CR2` | `0x04` | `0x00000000` | `rw` | **`SADD[7:1]` (`addr7 << 1`)**, `RD_WRN` b10, `START` b13, `STOP` b14, `NBYTES[23:16]`, `RELOAD` b24, `AUTOEND` b25 |
| `I2Cx->TIMINGR` | `0x10` | `0x00000000` | `rw` | `SCLL[7:0]`, `SCLH[15:8]`, `SDADEL[19:16]`, `SCLDEL[23:20]`, `PRESC[31:28]` |
| `I2Cx->ISR` | `0x18` | `0x00000001` | `r`/`rw` | `TXIS` b1, `RXNE` b2, **`NACKF` b4**, `STOPF` b5, `TC` b6, `TCR` b7, `BUSY` b15 |
| `I2Cx->ICR` | `0x1C` | `0x00000000` | **`w`** | `NACKCF` b4, `STOPCF` b5, `BERRCF` b8, `ARLOCF` b9 |
| `I2Cx->RXDR`/`TXDR` | `0x24`/`0x28` | `0x00000000` | `r`/`rw` | Receive/Transmit data *(NOT `DR`/`RDR`!)* (`I2C1_EV_IRQn=31`, `I2C1_ER_IRQn=32`) |

---

## 9. ADC (`ADC1=0x50000000`, `ADC12_COMMON=0x50000300`, AHB2) & DAC (`DAC1=0x50000800`, AHB2)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `ADCx->ISR` | `0x00` | `0x00000000` | **`rc_w1`** | `ADRDY` b0, `EOC` b2, `EOS` b3, `OVR` b4 — direct `=` clear! |
| **`ADCx->CR`** | `0x08` | **`0x20000000`** | `rw` | **`DEEPPWD` b29 (reset=1!)**, `ADVREGEN` b28 (wait 20 µs), `ADCAL` b31 (before `ADEN`), `ADEN` b0, `ADSTART` b2 |
| `ADCx->CFGR` | `0x0C` | `0x80000000` | `rw` | `DMAEN` b0, `DMACFG` b1 (`1`=Circular), `RES[4:3]`, `CONT` b13 |
| `ADCx->SMPR1/2` | `0x14`/`0x18` | `0x00000000` | `rw` | 3 bits/ch (`110b`=247.5, `111b`=640.5 cycles; temp sensor needs $\ge 5\ \mu\text{s}$) |
| `ADCx->SQR1` / `DR` | `0x30`/`0x40` | `0x00000000` | `rw`/`r` | `L[3:0]` (count−1), `SQ1..4`; reading `DR` clears `EOC` |
| `ADC12_COMMON->CCR` | `+0x08` | `0x00000000` | `rw` | **`CKMODE[17:16]` = `11b` (`HCLK/4 = 42.5 MHz <= 60 MHz`!)**, `VREFEN` b22, `VSENSESEL` b23 |
| `DACx->CR` / `DHR12R1` | `0x00`/`0x08` | `0x00000000` | `rw` | `EN1` b0, `TEN1` b1, `TSEL1[5:2]`; 12-bit right-aligned data |
| **`DACx->MCR`** | `0x3C` | `0x00000000` | `rw` | **`HFSEL[15:14]` = `10b` MANDATORY when `HCLK > 160 MHz`** (170 MHz)! |

---

## 10. DMA1/2 (`0x40020000`/`0x40020400`, AHB1) & DMAMUX1 (`0x40020800`, AHB1)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `DMAx->ISR` | `0x00` | `0x00000000` | `r` | `GIFn`, `TCIFn`, `HTIFn`, `TEIFn` (`n = 1..8`) |
| `DMAx->IFCR` | `0x04` | `0x00000000` | **`w`** | **ES0430 §2.3.1: Clear individual `CTCIFn`/`CHTIFn`/`CTEIFn` — NEVER `CGIFn`!** |
| `DMAx_Chn->CCR` | `0x08+0x14(n-1)` | `0x00000000` | `rw` | `EN`, `TCIE`, `HTIE`, `TEIE`, `DIR`, `CIRC`, `PINC`, `MINC`, `PSIZE`, `MSIZE`, `PL` |
| `DMAx_Chn->CNDTR/CPAR/CMAR` | `+0x04/+0x08/+0x0C` | `0x00000000` | `rw` | Count, Periph addr, Mem addr (**CCM-SRAM MUST use `0x20018000` alias!**) |
| `DMAMUX1_Ch0..15->CCR` | `0x00..0x3C` | `0x00000000` | `rw` | **`DMAREQ_ID[6:0]`** (`5`=ADC1, `10/11`=SPI1 RX/TX, `16/17`=I2C1 RX/TX, `24/25`=USART1, `26/27`=USART2) |

---

## 11. IWDG (`0x40003000`, APB1) & DBGMCU (`0xE0042000`)
| Register | Offset | Reset | Access | Critical Bits & Gotchas |
|---|---:|---:|---|---|
| `IWDG->KR` | `0x00` | `0x00000000` | **`w`** | `0xCCCC` Start, `0x5555` Unlock `PR`/`RLR`/`WINR`, `0xAAAA` Reload/Kick |
| `IWDG->PR` / `RLR` / `WINR` | `0x04`/`0x08`/`0x10` | `0`/`0xFFF`/`0xFFF` | `rw` | `PR=100b` (`/64`), 12-bit reload & window; poll `IWDG->SR == 0` (`PVU/RVU/WVU`) |
| `DBGMCU->APB1FZR1` | `0x08` | `0x00000000` | `rw` | Set `DBG_IWDG_STOP` (b12), `DBG_WWDG_STOP` (b11), `DBG_TIM2_STOP` (b0) for GDB |

---

## 12. ARM Cortex-M4 Core: SysTick, NVIC, SCB & DWT (PPB `0xE0000000+`)
| Register | Address | Access | Critical Bits & Gotchas |
|---|---:|---|---|
| `SysTick->CTRL/LOAD/VAL` | `0xE000E010..18` | `rw` | `ENABLE` b0, `TICKINT` b1, `CLKSOURCE` b2 (`1`=HCLK); 24-bit `LOAD` (`169999` = 1 ms @ 170 MHz) |
| `NVIC->ISER[0..7]` / `IP[0..239]` | `0xE000E100` / `0xE000E400` | `rw` | 102 external IRQs (`0..101`, max `FMAC_IRQn=101`), **4 priority bits `[7:4]`** |
| `SCB->VTOR` / `AIRCR` | `0xE000ED08` / `0x0C` | `rw` | `VTOR` **512-byte aligned**; `AIRCR` key `0x05FA`, `PRIGROUP[10:8]` (`0` or `3` for FreeRTOS) |
| `SCB->SHCSR` | `0xE000ED24` | `rw` | `MEMFAULTENA` b16, `BUSFAULTENA` b17, `USGFAULTENA` b18 |
| `SCB->CFSR` / `HFSR` | `0xE000ED28` / `0x2C` | **`rc_w1`** | `MMFSR[7:0]`, `BFSR[15:8]`, `UFSR[31:16]`; `HFSR.FORCED` b30 |
| `SCB->MMFAR` / `BFAR` | `0xE000ED34` / `0x38` | `rw` | Valid **only** if `CFSR.MMARVALID` (b7) / `CFSR.BFARVALID` (b15) == `1` |
| `SCB->CPACR` | `0xE000ED88` | `rw` | Set `0x00F00000UL` (`CP10`/`CP11=11b`) + `__DSB(); __ISB();` to enable FPU |
| `CoreDebug->DEMCR` / `DWT->CYCCNT` | `0xE000EDFC` / `0xE0001004` | `rw` | Set `DEMCR.TRCENA` (b24) then `DWT->CTRL.CYCCNTENA` (b0) for 1-cycle timing |
