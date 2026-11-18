# STM32G474RE & Cortex-M4 Register Abbreviations & Naming Conventions

Every register abbreviation below is verified directly against **`stm32g474xx.h`** and **`core_cm4.h`** for the **STM32G474RE** (highlighting STM32G4 register differences vs. legacy STM32F1/F4!).

**Concept ID:** `RES-GLOSS-001`

---

| Acronym | Full Hardware Register Name | Peripheral(s) & STM32G474RE-Exact Purpose |
|---|---|---|
| **ACR** | Access Control Register | `FLASH_ACR`: Wait states (`LATENCY`), Prefetch (`PRFTEN`), `ICEN`/`DCEN`, `ICRST`/`DCRST`. |
| **AFR / AFRL / AFRH** | Alternate Function Register | `GPIOx->AFR[0..1]`: Selects AF0–AF15 per pin (DS12288 Table 13). |
| **AHB1ENR / AHB2ENR / APB1ENR1 / APB2ENR** | Peripheral Bus Clock Enable | `RCC`: Gates bus clocks to peripherals (writes to unclocked peripherals are ignored!). |
| **AIRCR** | Application Interrupt & Reset Control | `SCB->AIRCR`: `VECTKEY` (`0x05FA`), `PRIGROUP` priority split, `SYSRESETREQ`. |
| **ARR** | Auto-Reload Register | `TIMx_ARR` / `LPTIM_ARR`: Defines timer overflow period ($\text{ARR} + 1$ ticks). |
| **BCDR** | Battery Charging Detector Register | `USB_BCDR`: Contains `DPPU` (bit 15) internal 1.5 kΩ D+ pull-up enable. |
| **BDCR** | Backup Domain Control Register | `RCC_BDCR`: LSE oscillator, RTC clock select/enable (requires `PWR_CR1.DBP = 1`). |
| **BDTR** | Break and Dead-Time Register | `TIM1/8/15/16/17/20_BDTR`: Dead-time `DTG`, Break `BKE`/`BK2E`, and mandatory **`MOE`** output enable. |
| **BFAR** | BusFault Address Register | `SCB->BFAR`: Exact faulting address when `SCB->CFSR.BFARVALID == 1`. |
| **BKP0R..BKP31R** | Backup Registers (32 × 32-bit) | **`TAMP->BKPxR`** (`0x40002500`): VBAT/Standby retained registers (auto-erased on tamper!). |
| **BRR** | Baud Rate / Bit Reset Register | **`USART_BRR`** (baud divisor) or **`GPIOx_BRR`** (offset `0x28`, atomic bit reset). |
| **BSRR** | Bit Set/Reset Register | `GPIOx_BSRR`: Atomic single-cycle pin set (`[15:0]`) and reset (`[31:16]`). |
| **CALFACT** | Calibration Factors Register | `ADC_CALFACT`: Single-ended and differential SAR ADC calibration trims. |
| **CCER** | Capture/Compare Enable Register | `TIMx_CCER`: Enables `CCxE`/`CCxNE` outputs and sets `CCxP`/`CCxNP` polarity. |
| **CCIPR / CCIPR2** | Independent Clock Config Registers | `RCC_CCIPR`/`CCIPR2`: Selects kernel clocks for USART, I2C, LPTIM, FDCAN, CLK48, ADC, QSPI. |
| **CCMR1–3** | Capture/Compare Mode Registers | `TIMx_CCMR1..3`: Configures PWM Mode 1/2, Input Capture, and preload (`OCxPE`). |
| **CCR** | *(Multi-Peripheral Name Collision!)* | **`TIMx_CCRx`** (Capture/Compare duty), **`DMA_CCRx`** (DMA channel config), **`ADCx_COMMON->CCR`** (`CKMODE` clock!), **`SCB->CCR`** (`DIV_0_TRP`/`STKALIGN`), **`DAC_CCR`** (offset cal), **`QUADSPI_CCR`** (frame config). |
| **CFGR / CFGR1 / CFGR2** | Configuration Register | `RCC_CFGR` (`SW`/`SWS`/`HPRE`), `ADC_CFGR`/`CFGR2`, `SYSCFG_CFGR1`/`CFGR2` (safety lockup), `CRS_CFGR`. |
| **CFSR** | Configurable Fault Status Register | `SCB->CFSR`: Packed `MMFSR[7:0]` + `BFSR[15:8]` + `UFSR[31:16]` fault diagnostics. |
| **CMAR / CPAR / CNDTR** | DMA Memory / Periph Addr / Count | `DMAx_Channelx`: Transfer memory pointer, peripheral pointer, and remaining item count. |
| **CNT** | Counter Register | `TIMx_CNT` / `LPTIM_CNT`: Current timer count (32-bit on `TIM2`/`TIM5`). |
| **CPACR** | Coprocessor Access Control Register | `SCB->CPACR`: Enables Cortex-M4F FPU (`CP10`/`CP11 = 0b11` + `DSB`/`ISB`). |
| **CR / CR1–CR5** | Control Register | Primary enable/config register (`RCC_CR`, `PWR_CR1..CR5` [note `CR5 @ 0x80`], `ADC_CR`, `USART_CR1..3`). |
| **CRRCR** | Clock Recovery RC Register | `RCC_CRRCR`: Enables and monitors internal 48 MHz **`HSI48`** (`HSI48ON`/`HSI48RDY`). |
| **CSR** | Control & Status Register | `RCC_CSR` (LSI + reset cause flags), `CORDIC_CSR`, `OPAMP_CSR`, `COMP_CSR`, `VREFBUF_CSR`. |
| **CYCCNT** | Cycle Count Register | `DWT->CYCCNT` (`0xE0001004`): 32-bit exact CPU clock cycle profiler. |
| **DBTP / NBTP** | Data / Nominal Bit Timing Regs | `FDCAN_DBTP` / `FDCAN_NBTP`: CAN-FD data-phase (BRS) and arbitration-phase bit timing. |
| **DCR / DMAR** | DMA Control / Burst Address | `TIMx_DCR` (`0x3DC`) & `TIMx_DMAR` (`0x3E0`): Timer DMA burst transfer interface. |
| **DHR12R1 / DOR1** | DAC Data Holding / Output Regs | `DAC_DHR12R1`: 12-bit right-aligned input; `DAC_DOR1`: active output value. |
| **DIER** | DMA / Interrupt Enable Register | `TIMx_DIER` / `HRTIM_MDIER`: Enables `UIE`, `CCxIE`, `UDE`, `CCxDE`. |
| **DLLCR** | DLL Calibration Control Register | `HRTIM1_COMMON->DLLCR`: Starts/auto-calibrates the ×32 184 ps Delay-Locked Loop. |
| **DR** | Data Register | **`SPI_DR`**, **`ADC_DR`**, **`CRC_DR`**, **`RNG_DR`**, **`RTC_DR`** *(Date)*. **NOT used by STM32G4 USART (`RDR`/`TDR`) or I2C (`RXDR`/`TXDR`)!** |
| **DTxR / DTR2** | Dead-Time Register | `HRTIM_DTxR` & `TIMx_DTR2`: Asymmetric rising/falling dead-time generation. |
| **ECCR** | Flash ECC Register | `FLASH_ECCR` (`0x18`): `ECCC` (1-bit corrected), `ECCD` (2-bit NMI), `ADDR_ECC`, `BK_ECC`. |
| **EGR** | Event Generation Register | `TIMx_EGR` (Write-Only!): Direct write `TIMx->EGR = TIM_EGR_UG;` (never `|=`). |
| **EXTICR** | External Interrupt Config Register | `SYSCFG->EXTICR[0..3]`: Maps GPIO port A–G onto EXTI lines 0–15. |
| **FPCCR** | FP Context Control Register | `FPU->FPCCR`: Controls FPU lazy stacking (`ASPEN`, `LSPEN`, `LSPACT`). |
| **HFSR** | HardFault Status Register | `SCB->HFSR`: `VECTTBL`, `FORCED` (escalated fault), `DEBUGEVT`. |
| **ICR** | Interrupt Clear Register | Write-only clear register in `USART_ICR`, `I2C_ICR`, `LPTIM_ICR`, `HRTIM_ICR`, `UCPD_ICR`. |
| **IDR** | Input Data / Independent Data Reg | `GPIOx_IDR` (pin digital levels); `CRC_IDR` (32-bit scratch). |
| **IFCR** | Interrupt Flag Clear Register | `DMA_IFCR`: Clear individual `CTCIFx`/`CHTIFx`/`CTEIFx` (avoid `CGIFx` per ES0430 §2.3.1). |
| **IMR1 / IMR2** | Interrupt Mask Register | `EXTI_IMR1` (lines 0–31) & `EXTI_IMR2` (lines 32–43). |
| **IP / IPR** | Interrupt Priority Registers | `NVIC->IP[0..239]` (byte view of 60×32-bit `IPR[0..59]`; top 4 bits `[7:4]` active on STM32G4). |
| **ISER / ICER** | Interrupt Set / Clear Enable | `NVIC->ISER[0..7]` / `ICER[0..7]`: Enable/disable external IRQs 0..101. |
| **ISR** | Interrupt & Status Register | `USART_ISR`, `I2C_ISR`, `ADC_ISR` (`rc_w1`), `DMA_ISR`, `HRTIM_ISR`. |
| **KEYR / OPTKEYR** | Flash Unlock Key Registers | `FLASH_KEYR` (`0x45670123`, `0xCDEF89AB`), `FLASH_OPTKEYR` (`0x08192A3B`, `0x4C5D6E7F`). |
| **KR / PR / RLR / WINR** | Independent Watchdog Registers | `IWDG`: Key (`0xCCCC`/`0x5555`/`0xAAAA`), Prescaler, 12-bit Reload, and 12-bit Window. |
| **LCKR** | Configuration Lock Register | `GPIOx_LCKR`: Locks pin configuration until reset via 4-step `LCKK` write sequence. |
| **MCR** | Mode / Master Control Register | `DAC_MCR` (`HFSEL` >160 MHz, `MODE1/2`), `HRTIM_MCR` (Master Timer control). |
| **MMFAR** | MemManage Fault Address Register | `SCB->MMFAR`: Exact faulting address when `SCB->CFSR.MMARVALID == 1`. |
| **MODER** | Port Mode Register | `GPIOx_MODER`: `00`=In, `01`=Out, `10`=AF, **`11`=Analog (STM32G4 reset default!)**. |
| **ODR** | Output Data Register | `GPIOx_ODR`: Output latch (avoid `^=`/`|=` in ISRs; use `BSRR`/`BRR`). |
| **OPTR** | Flash Option Register | `FLASH_OPTR`: `RDP`, `BOR_LEV`, `BFB2`, `DBANK`, `nSWBOOT0`, `nBOOT0`, `nBOOT1`, `NRST_MODE`. |
| **OSPEEDR / OTYPER / PUPDR** | GPIO Electrical Config Registers | Output slew rate (`OSPEEDR`), Push-Pull vs Open-Drain (`OTYPER`), Pull-Up/Down (`PUPDR`). |
| **PR1 / PR2** | Pending Register | `EXTI_PR1`/`PR2` (`rc_w1`): Direct write `EXTI->PR1 = mask;` to clear. |
| **PRESC** | Input Clock Prescaler Register | `USART_PRESC` (`0x2C`): Divides USART/LPUART kernel clock by `/1`..`/256`. |
| **PSC** | Timer Prescaler Register | `TIMx_PSC`: Divides timer clock by `PSC + 1`. |
| **RNR / RBAR / RASR** | MPU Region Config Registers | `MPU`: Region Number (`0..7`), Base Address, and Attribute/Size/AP/XN register. |
| **RCR** | Repetition Counter Register | `TIM1/8/15/16/17/20_RCR`: Decimates Update Events (`UEV`) by `RCR + 1` cycles. |
| **RDR / TDR** | Receive / Transmit Data Registers | **`USART_RDR` (`0x24`) & `USART_TDR` (`0x28`)** for **USART/UART/LPUART only** (NOT I2C!). |
| **RXDR / TXDR** | Receive / Transmit Data Registers | **`I2C_RXDR` (`0x24`) & `I2C_TXDR` (`0x28`)** for **I2C1–4** (and `UCPD_RXDR`/`TXDR`)! |
| **SCR** | System Control / Status Clear Reg | `SCB->SCR` (`SLEEPDEEP`, `SLEEPONEXIT`, `SEVONPEND`); `PWR_SCR` / `RTC_SCR` / `TAMP_SCR`. |
| **SCSR / SWPR** | CCM-SRAM Control & Write-Protect | `SYSCFG_SCSR` (hardware erase `CCMER`) & `SYSCFG_SWPR` (32×1 KB page write-protect). |
| **SEC1R / SEC2R** | Securable Memory Area Registers | `FLASH_SEC1R`/`SEC2R`: Root-of-trust hide-protected Flash pages (`SEC_SIZE`, `BOOT_LOCK`). |
| **SHCSR** | System Handler Control & State | `SCB->SHCSR`: Enables MemManage (b16), BusFault (b17), and UsageFault (b18). |
| **SMPR1 / SMPR2** | ADC Sample Time Registers | `ADC_SMPR1`/`2`: 3-bit sample cycles (`2.5`..`640.5` ADC clocks) per channel. |
| **SQR1–SQR4** | ADC Regular Sequence Registers | `ADC_SQR1..4`: Sequence length `L` and channel conversion order `SQ1..SQ16`. |
| **SR / SR1 / SR2** | Status Register | `TIMx_SR` (`rc_w0`), `SPI_SR`, `FLASH_SR`, `PWR_SR1` (`0x10`), **`PWR_SR2` (`0x14`, `VOSF` bit 10!)**. |
| **TIMINGR** | I2C Timing Register | `I2C_TIMINGR` (`0x10`): `PRESC`, `SCLDEL`, `SDADEL`, `SCLH`, `SCLL`. |
| **VTOR** | Vector Table Offset Register | `SCB->VTOR` (`0xE000ED08`): Relocates vector table (512-byte aligned on STM32G474RE). |
| **WPR** | RTC Write Protection Register | `RTC_WPR`: Write `0xCA` then `0x53` to unlock RTC registers. |
| **WRP1AR..WRP2BR** | Flash Write Protection Registers | `FLASH_WRPxAR/BR`: Locks bootloader Flash page ranges against runtime erase/program. |
