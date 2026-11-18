# Stage 04 Mastery Exam: STM32G474RE Clock Boost, Peripherals, Comms, Analog & DMA

**Aligned Stages:** Stages 06–11 (`07-rcc-and-clocks/`, `08-gpio/`, `09-timers/`, `10-interrupts/`, `11-communication/`, `12-adc-dac-analog/`, `13-dma/`)  
**Concept IDs:** `STM32-RCC-*`, `STM32-PWR-001`, `STM32-FLASH-001`, `PERIPH-GPIO/EXTI/TIM/UART/SPI/I2C/FDCAN/ADC/DAC/DMA-001`  
**Total Points:** 100 | **Pass Threshold:** $\ge 85$ Points | **Answer Key:** [`answer-keys-and-rubrics.md`](answer-keys-and-rubrics.md#stage-04-answer-key--rubric)

---

### Part A: Clock Boost, GPIO/EXTI & Timer Calculations (40 Points)

1. **170 MHz PLL Synthesis & Safe RM0440 §6.1.5 Boost Sequence (15 Points):**
   Using the NUCLEO-G474RE onboard **24 MHz HSE crystal (`X3`)**, select valid `(M, N, R, P)` parameters so that $f_{\text{PLL-IN}} \in [2.66, 16]\ \text{MHz}$, $f_{\text{VCO}} \in [96, 344]\ \text{MHz}$, $f_{\text{SYSCLK}} = 170\ \text{MHz}$, and $f_{\text{PLL-P}} \le 60\ \text{MHz}$ for ADC. List the exact 5 mandatory pre-switch & transition steps involving `PWR_CR5` (offset `0x80`), `PWR_SR2.VOSF`, `FLASH_ACR` (4 WS), and `RCC_CFGR.HPRE = /2`.

2. **GPIO Reset State & NUCLEO-G474RE `B1` (`PC13`) EXTI Polarity (10 Points):**
   (a) What is the STM32G4 `GPIOx_MODER` reset default for non-debug pins, and why did ST choose it over Input mode?
   (b) Per **UM2505**, what is the hardware polarity of User Button `B1` (`PC13`) on NUCLEO-G474RE (MB1367)? Specify `PUPDR`, `EXTI` edge register (`RTSR1` vs `FTSR1`), and `EXTI->PR1` clear syntax.

3. **Timer PWM & Complementary Dead-Time Calculation (15 Points):**
   With $f_{\text{TIM-CLK}} = 170\ \text{MHz}$ (`PSC = 0`):
   (a) Calculate `ARR` for an exact $32.0\ \text{kHz}$ edge-aligned PWM and state the resolution in counts.
   (b) For `TIM1` complementary outputs (`CH1`/`CH1N`), compute the exact `BDTR.DTG[7:0]` value for a $500\ \text{ns}$ dead-time ($t_{\text{DTS}} = 5.882\ \text{ns}$). What happens if `BDTR.MOE` is left `0`, and how does `COMP1` → `BKIN` protect an inverter bridge?

---

### Part B: Serial Buses (UART, SPI, I2C, FDCAN), Analog & DMA/DMAMUX (60 Points)

4. **USART Baud Rate & LPUART1 Formula (10 Points):**
   (a) Calculate `USART2->BRR` for 921,600 bps at $f_{\text{CK}} = 170\ \text{MHz}$ (`OVER8 = 0`, `PRESC = /1`) and its percentage baud error.
   (b) Why does `LPUART1` fail if you write the same `BRR` value as `USART2`? State the `LPUART1` BRR equation.

5. **SPI 8-Bit FIFO Trap & I2C Repeated-START / `PB8-BOOT0` Hazard (15 Points):**
   (a) Why does a bare-metal 8-bit `SPI1` polling transfer hang forever on `while (!(SPI1->SR & SPI_SR_RXNE))` if `SPI1->CR2.FRXTH` is left at reset default `0`, and why must software write `*(__IO uint8_t *)&SPI1->DR`?
   (b) Explain how `I2C_CR2.AUTOEND = 0` → `ISR.TC` → `AUTOEND = 1` executes a Repeated-START register read. Why does using `PB8` (`I2C1_SCL`) with only internal ~40 kΩ pull-up fail on NUCLEO-G474RE?

6. **FDCAN1 Nominal Bit Timing & Fixed Message RAM (10 Points):**
   With `FDCANSEL = HSE (24 MHz)` and `FDCAN_CONFIG->CKDIV = 0 (/1)`, compute valid `NBTP` fields (`NBRP`, `NTSEG1`, `NTSEG2`) for exact **500 kbps** at an **83.3% sample point** (16 TQ/bit). Where in memory does `FDCAN1`'s Message RAM begin, and how many bytes are allocated per instance?

7. **ADC Clock Compliance, Sampling Time & DAC `HFSEL` (15 Points):**
   (a) Why is `ADC12_COMMON->CCR.CKMODE = 01` (`HCLK/1`) illegal at 170 MHz? Which `CKMODE` setting complies with DS12288's 60 MHz / 52 MHz limit?
   (b) Given source impedance $R_{\text{AIN}} = 10\ \text{k}\Omega$, internal switch $R_{\text{ADC}} \approx 1\ \text{k}\Omega$, and $C_{\text{ADC}} = 5\ \text{pF}$, calculate minimum settling time for 12-bit accuracy ($\ln(2^{12+2}) \approx 9.7$ time constants) and select the minimum `SMPRx` encoding at $f_{\text{ADC}} = 42.5\ \text{MHz}$.
   (c) Which register and bitfield MUST be set on `DAC1` when operating at `HCLK = 170 MHz`?

8. **DMA1 + DMAMUX1 Circular Ping-Pong & ES0430 Erratum (10 Points):**
   Write the bare-metal initialization snippet routing **`ADC1`** (state exact RM0440 Table 91 Request ID) to `DMA1_Channel1` in 16-bit circular mode with Half-Transfer and Transfer-Complete interrupts. State the exact **ES0430 §2.3.1** rule for clearing flags in `DMA1->IFCR`, and give the exact DMAMUX Request IDs for `USART2_RX` and `USART2_TX`.
