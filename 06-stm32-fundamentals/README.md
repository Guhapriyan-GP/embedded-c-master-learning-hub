# Stage 06: STM32 Platform Architecture, STM32G474RET6 Silicon & ES0430 Errata

> **Curriculum Track:** Level 1–2 Platform Silicon Architecture (`STM32-*`, `G4-*`)  
> **Exact Silicon Verified:** **STM32G474RET6** (Category 3, **LQFP64** package, 512 KB Dual-Bank Flash, 128 KB SRAM, 170 MHz Cortex-M4F, `DEV_ID = 0x469`)  
> **Primary Ground Truth:** `stm32g474xx.h`, **RM0440**, **DS12288**, **UM2505**, **ES0430**  
> **Code Verification Policy:** All register addresses, offsets, and C snippets below are **Statically Reviewed directly against `stm32g474xx.h` (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives

Upon completing Stage 06, you will navigate STM32G474RET6 silicon architecture and official ST technical manuals with principal-engineer accuracy:

| Concept ID | Scope Layer | Level | Mastery Objective |
|---|---|---|---|
| `STM32-ARCH-001` | `[STM32 Generic / G474 Exact]` | L1 | Compare STM32 families (F103, F446RE, G0, **G474RE**, H7), trace the multi-layer AHB Bus Matrix, and map every STM32G474RE peripheral to its exact bus (**specifically correcting AHB2 `0x50000000` for ADC1–5 AND DAC1–4!**). |
| `STM32-FLASH-001..002` | `[G474 Exact]` | L1–L2 | Master 512 KB Flash Dual-Bank (`DBANK=1`, 2 KB pages, RWW) vs Single-Bank (`DBANK=0`, 4 KB pages), 64-bit double-word + 8-bit ECC, ART Accelerator, and wait-state tables. |
| `STM32-SRAM-001` | `[G474 Exact]` | L1–L2 | Partition the 128 KB SRAM hierarchy: **SRAM1 (80 KB `0x20000000`)**, **SRAM2 (16 KB `0x20014000`, parity + Standby retention)**, and **CCM-SRAM (32 KB `0x10000000` 0-WS I/D-Bus + `0x20018000` S-Bus DMA alias)**. |
| `PERIPH-GPIO-001`, `PERIPH-AES-001`, `PERIPH-FMC-001` | `[G474 Exact]` | L1 | Identify all **52 exact LQFP64 GPIOs** and distinguish **silicon non-existence (NO AES/CRYP on G474)** vs **LQFP64 unbonded peripherals (NO GPIOE, SPI4, or FMC)**. |
| `STM32-PWR-003` | `[G474 Exact]` | L1 | Handle the STM32G4 **UCPD dead-battery 5.1 kΩ pull-down trap** on `PA9`/`PA10` via `PWR->CR3.UCPD_DBDIS`. |
| `STM32-BOOT-001`, `STM32-RESET-001` | `[G474 Exact]` | L2 | Configure boot modes (`PB8-BOOT0`, `FLASH->OPTR` `nSWBOOT0`/`nBOOT0`/`nBOOT1`/`NRST_MODE`, empty-Flash check, 28 KB ST System ROM bootloader `0x1FFF0000`) and decode all 7 reset flags in `RCC->CSR`. |
| `STM32-DBGMCU-001`, `STM32-ERRATA-001` | `[G474 Exact]` | L2 | Read `DBGMCU->IDCODE` (`0x469`), factory `UID_BASE`/`FLASHSIZE_BASE`, navigate RM0440/DS12288/PM0214/UM2505/ES0430 accurately, and apply mandatory **ES0430 Errata** workarounds. |

---

## 2. Prerequisites

- [Stage 01b — Digital Electronics & NUCLEO-G474RE Hardware](../01b-digital-electronics-and-hw-integration/README.md) (`HWSW-PCB-001..002`).
- [Stage 04 — ARM Cortex-M4F Microarchitecture](../04-arm-cortex-m/README.md) (`ARM-MEM-001..002`, `ARM-CORE-001`).
- [Stage 05 — GNU Toolchain, Linker Scripts & Debugging](../05-toolchain-and-debugging/README.md) (`TOOL-LD-001`, `TOOL-GDB-001`).

---

## 3. Detailed Technical Theory

### 3.1 STM32 Family Positioning Matrix (`STM32-ARCH-001`)

| Feature | STM32F103C8 | STM32G071RB | STM32F446RE | **STM32G474RET6 (Target)** | STM32H743VI |
|---|---|---|---|---|---|
| **Core / ISA** | Cortex-M3 (ARMv7-M) | Cortex-M0+ (ARMv6-M) | Cortex-M4F (ARMv7E-M) | **Cortex-M4F (ARMv7E-M)** | Cortex-M7 (ARMv7E-M + DP-FPU) |
| **Max SYSCLK** | 72 MHz | 64 MHz | 180 MHz | **170 MHz** | 480 MHz |
| **Flash / SRAM** | 64 KB / 20 KB | 128 KB / 36 KB | 512 KB / 128 KB | **512 KB Dual-Bank ECC / 128 KB (incl. 32 KB CCM)** | 2 MB / 1 MB |
| **Mixed-Signal / Math** | 2× 12-bit 1 MSPS ADC | 1× 12-bit 2.5 MSPS ADC | 3× 12-bit 2.4 MSPS ADC, 2× DAC | **5× 12-bit 4 MSPS ADC, 4× DAC (7 ch), 6× OPAMP, 7× COMP, HRTIM (184 ps), CORDIC, FMAC** | 3× 16-bit ADC, 2× DAC, L1 Cache |
| **Comms** | 1× bxCAN, USB FS | UCPD, 2× I2C, 4× USART | 2× bxCAN, USB OTG | **3× FDCAN, USB FS + CRS, UCPD1, QSPI, SAI1, 6× UART/LPUART** | 2× FDCAN, Ethernet, USB HS |
| **Primary Domain** | Legacy / low-cost | Entry-level ultra-value | Legacy general-purpose | **Digital power, FOC motor control, mixed-signal industrial** | High-end HMI, audio, DSP |

---

### 3.2 STM32G474RET6 Multi-Layer AHB Bus Matrix & Exact Peripheral Bus Map (`STM32-ARCH-001`)

The STM32G474RE interconnects **5 bus masters** (`CPU I-Code`, `CPU D-Code`, `CPU S-Bus`, `DMA1`, `DMA2`) to **multiple bus slaves** (Flash, SRAM1, SRAM2, CCM-SRAM, AHB1, AHB2, APB1, APB2, AHB3) through a **Multi-Layer AHB Bus Matrix**. Concurrent masters accessing *different* slaves experience **zero arbitration stalls** — for example, the CPU can execute a 170 MHz control ISR from **CCM-SRAM (`0x10000000`)** over I-Code/D-Code simultaneously while **DMA1** streams ADC samples into **SRAM1 (`0x20000000`)**!

**Exact STM32G474RE Bus Hierarchy (Verified Against `stm32g474xx.h`, All Buses Up to 170 MHz):**

| Bus Domain | Base Address | Clock Enable Register(s) | Exact Peripherals Mapped on STM32G474RET6 (LQFP64) |
|---|---|---|---|
| **APB1** | `0x40000000` | `RCC->APB1ENR1`, `APB1ENR2` | `TIM2–TIM7`, `CRS`, `TAMP`, `RTC`, `WWDG`, `IWDG`, `SPI2/I2S2`, `SPI3/I2S3`, `USART2`, `USART3`, `UART4`, `UART5`, `I2C1–I2C4`, `USB` (+1 KB PMA `0x40006000`), `FDCAN1–3` (+Message RAM `0x4000A400`), `PWR`, `LPTIM1`, `LPUART1`, `UCPD1` |
| **APB2** | `0x40010000` | `RCC->APB2ENR` | `SYSCFG`, `VREFBUF`, `COMP1–7`, `OPAMP1–6`, `EXTI`, `TIM1`, `SPI1`, `TIM8`, `USART1`, `TIM15–17`, `TIM20`, `SAI1`, **`HRTIM1`** *(Note: `SPI4` is unbonded on LQFP64)* |
| **AHB1** | `0x40020000` | `RCC->AHB1ENR` | `DMA1` (8 ch), `DMA2` (8 ch), `DMAMUX1` (16 ch + 4 ReqGen), **`CORDIC`** (`0x40020C00`), `RCC` (`0x40021000`), **`FMAC`** (`0x40021400`), `FLASH` regs (`0x40022000`), `CRC` (`0x40023000`) |
| **AHB2 (GPIO)** | `0x48000000` | `RCC->AHB2ENR` | `GPIOA` (`0x48000000`), `GPIOB` (`0x48000400`), `GPIOC` (`0x48000800`), `GPIOD` (`0x48000C00`), `GPIOF` (`0x48001400`), `GPIOG` (`0x48001800`) *(NO `GPIOE` pins on LQFP64!)* |
| **AHB2 (Analog & RNG)** | **`0x50000000`** | `RCC->AHB2ENR` | **`ADC1–ADC5`** (`0x50000000..0x500007FF`), **`DAC1–DAC4`** (`0x50000800..0x500017FF`), **`RNG`** (`0x50060800`) — **CRITICAL CORRECTION: On STM32G474RE, DAC1–4 and ADC1–5 are on AHB2 (`0x50000000`), NOT APB1!** |
| **AHB3** | `0x90000000` / `0xA0001000` | `RCC->AHB3ENR` | **`QUADSPI`** (XIP `0x90000000`, regs `0xA0001000`). *(`FMC` at `0xA0000000` is unbonded on LQFP64!)* |

---

### 3.3 Memory Architecture: 512 KB Dual-Bank Flash & 128 KB SRAM (`STM32-FLASH-001..002`, `STM32-SRAM-001`)

| Region | Address Range | Size | Verified STM32G474RE Hardware Capabilities |
|---|---|---|---|
| **Main Flash** | `0x08000000–0x0807FFFF` | **512 KB** | **Dual-Bank (`DBANK=1`, default):** Bank 1 (`0x08000000`, 256 KB) + Bank 2 (`0x08040000`, 256 KB), **2 KB pages**, supports **Read-While-Write (RWW)** for seamless OTA/EEPROM emulation! **Single-Bank (`DBANK=0`):** 512 KB, **4 KB pages**. Programmed strictly in **64-bit double-words** with **8-bit SECDED ECC** (`FLASH->ECCR`). ART Accelerator (Prefetch + 1 KB ICache + 256 B DCache); requires **4 WS** at 170 MHz Boost Mode. |
| **CCM-SRAM (Native)** | **`0x10000000–0x10007FFF`** | **32 KB** | Connected to CPU **I-Code & D-Code** buses for **0-wait-state execution & data at 170 MHz**! Supports hardware byte parity (`SYSCFG->CFGR2.SPF`) and 1 KB page write-protect (`SYSCFG->SWPR`). **NOT accessible by DMA at `0x10000000`!** |
| **SRAM1** | `0x20000000–0x20013FFF` | **80 KB** | Main System SRAM (S-Bus & DMA1/DMA2 accessible; first 32 KB hardware parity-checked when `SRAM_PE` enabled; bit-band alias `0x22000000`). |
| **SRAM2** | `0x20014000–0x20017FFF` | **16 KB** | Contiguous after SRAM1 (96 KB linear), **hardware byte parity**, optional **Standby retention** (`PWR->CR3.RRS = 1`), bit-band alias `0x22280000`. |
| **CCM-SRAM (DMA Alias)** | **`0x20018000–0x2001FFFF`** | **32 KB** | **S-Bus contiguous alias** of the *exact same* 32 KB physical CCM-SRAM (making 128 KB contiguous `0x20000000–0x2001FFFF`) — **accessible by DMA1/DMA2** via BusMatrix! Bit-band alias `0x22300000`. *(Never allocate the same physical CCM memory simultaneously via both `0x10000000` and `0x20018000`!)* |
| **System Memory (ROM)** | `0x1FFF0000–0x1FFF6FFF` | **28 KB** | ST Factory Bootloader (supports USART1/2/3, I2C2/3/4, SPI1/2/3, FDCAN1, USB DFU). |
| **OTP / Factory Cal / Option Bytes** | `0x1FFF7000–0x1FFF787F` | 1 KB OTP + Cal + OB | `PACKAGE_BASE` (`0x1FFF7500`), 96-bit `UID_BASE` (`0x1FFF7590`), `FLASHSIZE_BASE` (`0x1FFF75E0` = `512`), `TS_CAL1`/`TS_CAL2`/`VREFINT_CAL`, Option Bytes (`0x1FFF7800`). |

---

### 3.4 Exact LQFP64 Pinout Boundaries & Non-Existent vs Unbonded Peripherals (`PERIPH-GPIO-001`, `PERIPH-AES-001`, `PERIPH-FMC-001`)

A frequent mistake when reading `stm32g474xx.h` (which serves LQFP48 through UFBGA121 packages) is assuming every declared peripheral is physically wired out on **STM32G474RET6 (LQFP64)**:

1. **Exact 52 Bonded GPIOs on LQFP64:**
   - `GPIOA`: **16 pins** (`PA0–PA15`)
   - `GPIOB`: **16 pins** (`PB0–PB15`)
   - `GPIOC`: **16 pins** (`PC0–PC15`)
   - `GPIOD`: **1 pin** (`PD2` only!)
   - `GPIOF`: **2 pins** (`PF0`, `PF1` — wired to 24 MHz crystal `X3` on NUCLEO-G474RE)
   - `GPIOG`: **1 pin** (`PG10-NRST` only!)
   - **Total: $16 + 16 + 16 + 1 + 2 + 1 = \mathbf{52\text{ GPIOs}}$.**
2. **Unbonded on LQFP64 (`NOT_APPLICABLE` for external I/O on STM32G474RET6):**
   - **`GPIOE`:** 0 pins bonded on LQFP64.
   - **`SPI4` (`0x40013C00`, `SPI4_IRQn = 84`):** All `SPI4` alternate-function pins live on Port E → unusable externally on LQFP64.
   - **`FMC` / `FSMC` (`0xA0000000`, `FMC_IRQn = 48`):** Requires $\ge 100$-pin package (DS12288 Table 2) → unavailable on LQFP64.
   - **Parallel ETM Trace (`TRACED0..3` on `PE2–PE6`):** Unbonded on LQFP64 (use single-wire SWO on `PB3` instead).
3. **Does NOT Exist on STM32G474 Silicon (`PERIPH-AES-001`: `NOT_APPLICABLE`):**
   - **Hardware AES / CRYP / HASH DOES NOT EXIST on STM32G474RE!** It exists *only* on **STM32G483 / STM32G484** (verified: zero `AES`/`CRYP` symbols in `stm32g474xx.h`, and IRQ 85 is reserved/skipped).
4. **UCPD Dead-Battery Pull-Down Trap (`STM32-PWR-003`):**
   - Out of reset, STM32G474 enables internal **5.1 kΩ USB-C PD dead-battery pull-downs** on **`PA9`/`PA10`** (`UCPD1_DBCC1/2`). Always enable `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;` and set **`PWR->CR3 |= PWR_CR3_UCPD_DBDIS;`** when using `PA9`/`PA10` for UART1, I2C2, TIM1, or GPIO!

---

### 3.5 Boot Modes, Reset Sources (`RCC->CSR`) & `DBGMCU` (`STM32-BOOT-001`, `STM32-RESET-001`, `STM32-DBGMCU-001`)

1. **Boot Configuration (`FLASH->OPTR` & `PB8-BOOT0`, RM0440 §2.6):**
   - **`nSWBOOT0 = 1` (Factory Default):** Hardware pin **`PB8-BOOT0`** selects boot space (`BOOT0 = 0` → Main Flash `0x08000000`; `BOOT0 = 1, nBOOT1 = 1` → 28 KB ST System Memory ROM Bootloader `0x1FFF0000`; `BOOT0 = 1, nBOOT1 = 0` → Embedded SRAM1 `0x20000000`).
   - **`nSWBOOT0 = 0`:** Frees `PB8` from boot-pin duty and uses software option bit `nBOOT0` instead.
   - **Empty-Flash Check (`FLASH->ACR.EMPTY`):** If the first 64 bits of Main Flash are `0xFFFFFFFF_FFFFFFFF`, hardware automatically boots into the ST System Memory ROM bootloader even with `BOOT0 = 0`!
   - **`NRST_MODE[29:28]`** configures `PG10-NRST` as legacy reset-only, GPIO-only (`PG10`), or bidirectional reset.
2. **Reset Cause Decoding (`RCC->CSR`, `0x40021094`):**
   At boot, read and latch all 7 sticky reset flags *before* clearing them via `RCC->CSR |= RCC_CSR_RMVF; RCC->CSR &= ~RCC_CSR_RMVF;`:
   - `OBLRSTF` (bit 25): Option-Byte Loader reset
   - `PINRSTF` (bit 26): External `NRST` pin reset (always set alongside BOR/POR!)
   - `BORRSTF` (bit 27): Brown-Out / Power-On reset
   - `SFTRSTF` (bit 28): Software reset (`NVIC_SystemReset()`)
   - `IWDGRSTF` (bit 29): Independent Watchdog reset
   - `WWDGRSTF` (bit 30): Window Watchdog reset
   - `LPWRRSTF` (bit 31): Illegal Low-Power entry reset
3. **`DBGMCU->IDCODE` (`0xE0042000`):** Bits `[11:0]` = `DEV_ID = 0x469` (Category 3 STM32G474/484); Bits `[31:16]` = `REV_ID` (silicon revision Z/Y/X — cross-check against **ES0430**!).

---

### 3.6 Navigating Official ST Documentation & Mandatory ES0430 Errata Checklist (`STM32-ERRATA-001`)

#### Authoritative STM32G474RE Document Map (Corrected Chapters & Tables)

| Document | Title | Verified High-Value Sections |
|---|---|---|
| **RM0440** | *STM32G4 Series Reference Manual* | Ch. 2 (Memory/BusMatrix/Bit-Banding), Ch. 3 (Flash/ECC/OB), **Ch. 6 (PWR)**, **Ch. 7 (RCC)**, Ch. 9 (GPIO), Ch. 10 (SYSCFG), **Ch. 12 (DMA)**, **Ch. 13 (DMAMUX)**, **Ch. 15 (EXTI)**, Ch. 16–18 (CRC/CORDIC/FMAC), Ch. 21 (ADC), Ch. 22 (DAC), Ch. 28–30 (TIM1/8/20, TIM2–5), Ch. 37/40 (USART Rev8/9), Ch. 39/41 (I2C), Ch. 40/42 (SPI), Ch. 47 (DBG). |
| **DS12288** | *STM32G474xB/xC/xE Datasheet* | Table 2 (LQFP64 peripheral counts), **Table 12 (Pin definitions `FT`/`TT_a`)**, **Table 13 (Alternate Functions)**, **Table 14 (Voltage ratings)**, **Table 15 (Current limits $\pm 20\text{ mA}$ / $160\text{ mA}$)**, §5.3.5 (Supply current), §5.3.9–10 (PLL/clocks), §5.3.19 (ADC $\le 60\text{ MHz}$). |
| **PM0214** | *STM32 Cortex-M4 Programming Manual* | Core registers, NVIC, SysTick, SCB Faults, MPU, FPU. |
| **UM2505** | *STM32G4 Nucleo-64 Boards (MB1367)* | STLINK-V3E, `JP6` IDD vs `JP5` 5V_SEL, B1 Active-HIGH `PC13`, LD2 `PA5`, X3 24 MHz, Arduino/Morpho schematics. *(Note: `UM2553` is STM32CubeIDE Quick Start — NOT the Nucleo manual!)* |
| **ES0430** | *STM32G471/473/474/483/484 Device Errata* | **Mandatory bare-metal silicon workarounds** (see below). |
| **Key ST ANs** | Application Notes | `AN5093` (G4 HW Dev), `AN2606` (ROM Bootloader), `AN2834` (ADC Accuracy), `AN4539` (HRTIM Cookbook), `AN5305` (FMAC), `AN5325` (CORDIC), `AN4899` (GPIO Low-Power), `AN4989` (Debug Toolbox). |

#### Mandatory ES0430 Silicon Errata Highlights (`STM32-ERRATA-001`)

Every bare-metal STM32G474RE driver MUST honor these **ES0430** silicon workarounds:
1. **§2.1.2 (Cortex-M4 Core):** Interrupted `VDIV`/`VSQRT` FPU instructions can corrupt state if an ISR is $\le 2$ instructions with lazy stacking — ensure ISRs are $> 2$ instructions or disable `FPCCR.LSPEN`.
2. **§2.1.3 (ARM Erratum 838869):** Store immediate clearing a peripheral interrupt flag right before ISR exit can cause a spurious double-entry — **always place `__DSB();` before returning from an ISR** after writing `EXTI->PR1`, `TIMx->SR`, etc.!
3. **§2.2.1 (System Debug):** Full JTAG without `NJTRST` (`PB4`) breaks debug attach — **always use 2-wire SWD (`PA13`/`PA14`)**.
4. **§2.2.2–2.2.3 (Flash RWW & ECC):** Always flush Flash **DCache (`FLASH->ACR |= FLASH_ACR_DCRST;`)** after programming/erasing Flash during Read-While-Write before reading back, and handle `FLASH->ECCR` safely if reset occurs mid-erase.
5. **§2.2.4 (RCC/RTC):** Do not rely on LSI clocking RTC simultaneously with LSE CSS enabled — clock RTC directly from LSE (`X2` 32.768 kHz).
6. **§2.3.1 (DMA — CRITICAL):** Writing the global interrupt clear bit **`DMA_IFCR.CGIFx`** simultaneously with a transfer error prevents DMA channel auto-disable and drops `TEIFx`! **RULE: ALWAYS clear individual `CTCIFx`, `CHTIFx`, `CTEIFx` bits — NEVER write `CGIFx`!**
7. **§2.4.1 / §2.4.3 (DMAMUX):** Guard against concurrent overrun flag clear races in `DMAMUX1_ChannelStatus->CFR` and `DMAMUX1_RequestGenStatus->RGCFR`.

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **Multi-Layer AHB Bus Matrix** | Crossbar interconnect letting CPU I-Code, D-Code, S-Bus, DMA1, and DMA2 simultaneously access distinct memory/peripheral slaves without bus stalls. |
| **CCM-SRAM Dual Mapping** | STM32G474RE's 32 KB Core-Coupled SRAM mapped simultaneously at `0x10000000` (CPU 0-WS I/D-Bus, no DMA) and `0x20018000` (S-Bus contiguous alias, DMA-accessible). |
| **Read-While-Write (RWW)** | Dual-Bank Flash capability (`DBANK = 1`) allowing CPU code execution from Bank 1 while erasing or programming pages in Bank 2 without stalling the CPU. |
| **Silicon vs Package Availability** | Distinction between peripherals existing on the silicon die (`stm32g474xx.h`) vs peripherals physically bonded out to pins on a specific package (LQFP64 = 52 GPIOs; no GPIOE/SPI4/FMC). |
| **Errata Sheet (`ES0430`)** | Official silicon manufacturer document listing verified hardware bugs per silicon revision (`REV_ID`) and mandatory firmware workarounds. |

---

## 5. Register-Level Explanations

Verified directly against `stm32g474xx.h`:
- **`RCC->CSR` (`0x40021094`):** Bits `[31:25]` hold the 7 latched reset flags (`LPWRRSTF`..`OBLRSTF`); Bit `23` (`RMVF`) clears all 7 flags when written `1` then `0`.
- **`DBGMCU->IDCODE` (`0xE0042000`):** Bits `[11:0]` = `0x469` (STM32G474/484 Category 3); Bits `[31:16]` = silicon revision ID.
- **`FLASH->OPTR` (`0x40022020`):** `RDP[7:0]`, `BOR_LEV[10:8]`, `DBANK[22]`, `nBOOT1[23]`, `SRAM_PE[24]` (inverted: `0` = enabled), `nSWBOOT0[26]`, `nBOOT0[27]`, `NRST_MODE[29:28]`.
- **Factory Read-Only Info (`stm32g474xx.h`):** `UID_BASE` = `0x1FFF7590UL` (96-bit unique device ID), `FLASHSIZE_BASE` = `0x1FFF75E0UL` (16-bit Flash KB = `512`), `PACKAGE_BASE` = `0x1FFF7500UL`.

---

## 6. Relevant Formulas & Calculations

### 6.1 Total STM32G474RET6 SRAM & Flash Partitioning
$$\text{SRAM}_{\text{total}} = \underbrace{80\text{ KB}}_{\text{SRAM1}} + \underbrace{16\text{ KB}}_{\text{SRAM2}} + \underbrace{32\text{ KB}}_{\text{CCM-SRAM}} = 128\text{ KB} \quad (\texttt{0x20000000}..\texttt{0x2001FFFF})$$
$$\text{Flash Pages} = \begin{cases} 2 \text{ banks} \times 128 \text{ pages} \times 2\text{ KB} = 512\text{ KB} & (\texttt{DBANK} = 1) \\ 1 \text{ bank} \times 128 \text{ pages} \times 4\text{ KB} = 512\text{ KB} & (\texttt{DBANK} = 0) \end{cases}$$

### 6.2 APB Timer Clock Doubler Rule (RM0440 Ch. 7)
$$f_{\text{TIMx\_CLK}} = \begin{cases} f_{\text{PCLKx}} & \text{if } \texttt{PPREx} = /1 \\ 2 \times f_{\text{PCLKx}} & \text{if } \texttt{PPREx} \in \{/2, /4, /8, /16\} \end{cases}$$

---

## 7. Official Documentation References

- **ST RM0440** (*STM32G4 Series Reference Manual*): Ch. 2, Ch. 3, Ch. 6, Ch. 7, Ch. 9, Ch. 47.
- **ST DS12288** (*STM32G474xB/xC/xE Datasheet*): Tables 2, 12, 13, 14, 15.
- **ST ES0430** (*STM32G471xx/473xx/474xx/483xx/484xx Device Errata*): §2.1–§2.4.
- **ST UM2505** (*STM32G4 Nucleo-64 Boards MB1367 User Manual*).
- **ST AN2606** (*STM32 Microcontroller System Memory Boot Mode*) & **ST AN5093** (*STM32G4 Hardware Development*).

---

## 8. Verified Code Examples

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** directly against `stm32g474xx.h` (Not Tested on Physical Hardware).

### 8.1 Silicon Identity, Unique ID & `RCC->CSR` Reset-Cause Decoder (`STM32-ARCH-001`, `STM32-RESET-001`, `STM32-DBGMCU-001`)

```c
#include "stm32g474xx.h"
#include <stdint.h>

typedef struct {
    uint16_t dev_id;       /* Expected 0x0469 for STM32G474 */
    uint16_t rev_id;       /* Cross-check with ES0430 */
    uint16_t flash_kb;     /* Expected 512 for STM32G474RET6 */
    uint32_t uid[3];       /* 96-bit factory Unique Device ID */
    uint32_t reset_flags;  /* Raw RCC->CSR bits [31:25] */
} stm32g4_sysinfo_t;

void stm32g4_read_sysinfo_and_clear_reset(stm32g4_sysinfo_t *info)
{
    uint32_t idcode = DBGMCU->IDCODE;
    info->dev_id    = (uint16_t)(idcode & DBGMCU_IDCODE_DEV_ID_Msk);
    info->rev_id    = (uint16_t)((idcode & DBGMCU_IDCODE_REV_ID_Msk) >> DBGMCU_IDCODE_REV_ID_Pos);
    info->flash_kb  = *(const volatile uint16_t *)FLASHSIZE_BASE;

    const volatile uint32_t *uid_ptr = (const volatile uint32_t *)UID_BASE;
    info->uid[0] = uid_ptr[0];
    info->uid[1] = uid_ptr[1];
    info->uid[2] = uid_ptr[2];

    /* Latch reset cause BEFORE clearing sticky flags */
    info->reset_flags = RCC->CSR & (RCC_CSR_LPWRRSTF | RCC_CSR_WWDGRSTF |
                                    RCC_CSR_IWDGRSTF | RCC_CSR_SFTRSTF  |
                                    RCC_CSR_BORRSTF  | RCC_CSR_PINRSTF  |
                                    RCC_CSR_OBLRSTF);

    /* Clear sticky reset flags (RM0440 §7.4.27) */
    RCC->CSR |= RCC_CSR_RMVF;
    RCC->CSR &= ~RCC_CSR_RMVF;
}
```

---

## 9. Practical STM32G474RE Experiments

1. **GDB Silicon Identity & Factory Calibration Inspection (`STM32-ARCH-001`, `STM32-DBGMCU-001`):**
   - Attach GDB via OpenOCD to NUCLEO-G474RE and execute:
     ```gdb
     x/1wx 0xE0042000   # DBGMCU->IDCODE: verify lower 12 bits == 0x469
     x/1hx 0x1FFF75E0   # FLASHSIZE_BASE: verify 0x0200 (512 KB)
     x/3wx 0x1FFF7590   # UID_BASE: 96-bit factory Unique ID
     x/1wx 0x40022020   # FLASH->OPTR: verify RDP==0xAA (Level 0) and DBANK bit 22
     ```
2. **Reset Cause Forensics (`STM32-RESET-001`):**
   - Call `stm32g4_read_sysinfo_and_clear_reset(&info)` at boot, then compare `info.reset_flags` after (a) power cycle (`BORRSTF | PINRSTF`), (b) pressing black button B2 (`PINRSTF`), (c) calling `NVIC_SystemReset()` (`SFTRSTF`), and (d) watchdog expiry in [Lab 13 — IWDG](../18-practical-laboratory/lab-13-iwdg-hardware-watchdog/README.md) (`IWDGRSTF`).
3. **CCM-SRAM Dual-Mapping Proof (`STM32-SRAM-001`):**
   - Write `0xCAFEBABEUL` to `*(volatile uint32_t *)0x10000000UL` in GDB, then execute `x/1wx 0x20018000` — confirm it reads back `0xCAFEBABE` immediately, proving both addresses map to the exact same physical 32 KB CCM-SRAM!
   - **Debugging Checkpoint:** Inspect `x/1wx 0x4002104C` (`RCC->AHB2ENR`) and confirm enabling `DAC1` (`bit 16`) and `ADC12` (`bit 13`) live on **AHB2**, not APB1!

---

## 10. Common Mistakes & Anti-Patterns

1. **Enabling `DAC1` clock in `RCC->APB1ENR1` instead of `RCC->AHB2ENR`:** On STM32F4, DAC was on APB1; on **STM32G474RE**, `DAC1–DAC4` (`0x50000800..0x500017FF`) are on **AHB2** (`RCC->AHB2ENR`)!
2. **Attempting DMA transfers to/from `0x10000000` (CCM-SRAM native):** DMA1/DMA2 cannot access `0x10000000` — point DMA to the S-Bus alias **`0x20018000`** (or SRAM1/SRAM2)!
3. **Writing `DMA1->IFCR = DMA_IFCR_CGIF1;`:** Violates **ES0430 §2.3.1** and drops transfer errors. Always clear `CTCIF1`/`CHTIF1`/`CTEIF1` individually!
4. **Assuming STM32G474RE has hardware AES, FMC, SPI4, or GPIOE:** AES does not exist on G474 silicon (G484 only), and GPIOE/SPI4/FMC are unbonded on LQFP64.
5. **Forgetting to clear `RCC->CSR` via `RMVF` after reading reset flags:** Sticky reset flags accumulate across multiple resets, misleading post-mortem diagnostics.
6. **Forgetting `PWR->CR3 |= PWR_CR3_UCPD_DBDIS` on `PA9`/`PA10`:** Leaves internal 5.1 kΩ pull-downs active from reset.

---

## 11. Hands-On Exercises

1. **Documentation Trace Exercise:** Using **DS12288 Table 12 & Table 13** and **RM0440**, trace `USART1_TX` (`PA9`) and `DAC1_OUT1` (`PA4`) all the way from physical LQFP64 pin number → pin type (`FT_c` vs `TT_a`) → AF number → peripheral base address → `RCC` enable register & bit position → `IRQn` number.
2. **Bus Matrix Concurrency Design:** Design a zero-contention memory layout on STM32G474RE for a 100 kHz digital power controller where CPU executes a PID ISR, DMA1 reads `ADC1` into a ping-pong buffer, and DMA2 streams telemetry to `USART2`. Assign each code/data block to Flash, CCM-SRAM (`0x10000000`), SRAM1, or SRAM2 and justify every placement.
3. **ES0430 Audit Checklist:** Review a sample bare-metal DMA + EXTI + Flash driver and annotate the exact lines where ES0430 §2.1.3 (`__DSB()`), §2.2.2 (`DCRST`), and §2.3.1 (avoid `CGIFx`) apply.

---

## 12. Assessment Questions

1. On STM32G474RE, which bus and base address host `DAC1–DAC4` and `ADC1–ADC5`? Which `RCC` register enables their bus clock?
2. Explain the exact difference between accessing STM32G474RE CCM-SRAM at `0x10000000` vs `0x20018000`. Why should a linker script NEVER allocate independent variables in both windows overlapping the same offsets?
3. List all 6 GPIO ports bonded out on the STM32G474RET6 LQFP64 package and verify the total count of 52 GPIOs. Why are `AES`, `FMC`, and `SPI4` unavailable on NUCLEO-G474RE?
4. What happens if Main Flash is completely blank (`0xFFFFFFFF_FFFFFFFF` at `0x08000000`) and you power on STM32G474RE with `PB8-BOOT0 = 0`?
5. What exact bug occurs if you write `DMA1->IFCR = DMA_IFCR_CGIF1` per **ES0430 §2.3.1**, and what is the mandatory workaround?
6. How do you distinguish a power-on brownout reset from an `IWDG` timeout reset or `NVIC_SystemReset()` inside `main()`?

---

## 13. Measurable Completion Criteria

- [ ] **Silicon Architecture Accuracy Verified:** Accurately map every STM32G474RET6 bus (APB1, APB2, AHB1, AHB2 GPIO & Analog/DAC/RNG, AHB3 QSPI), 512 KB Dual-Bank Flash, 128 KB SRAM (80+16+32 dual-mapped), and exact 52 LQFP64 GPIOs.
- [ ] **Live GDB Silicon Readout Complete:** Read and decode `DBGMCU->IDCODE` (`0x469` + `REV_ID`), `FLASHSIZE_BASE` (`512`), `UID_BASE`, `FLASH->OPTR`, and `RCC->CSR` reset flags in GDB.
- [ ] **Platform Traps & Errata Mastered:** Apply `PWR_CR3_UCPD_DBDIS`, CCM-SRAM DMA alias rules, and all 7 ES0430 errata workarounds (including avoiding `DMA_IFCR.CGIFx` and placing `__DSB()` before ISR return).
- [ ] **Official Manual Fluency:** Rapidly locate verified register offsets in RM0440, pin AF/electrical limits in DS12288, MB1367 schematics in UM2505, and workarounds in ES0430.
