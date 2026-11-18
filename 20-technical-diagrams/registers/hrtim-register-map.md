# HRTIM1 (High-Resolution Timer — 184 ps) Register Map

**Target MCU:** STM32G474RE (LQFP64 — Flagship STM32G474 Digital Power Peripheral!)  
**Base Addresses (APB2):**
- `HRTIM1` (Master): `0x40016800` (`APB2PERIPH_BASE + 0x6800UL`)
- `HRTIM1_TIMA..HRTIM1_TIMF`: `0x40016880` (`TIMA`), `0x40016900` (`TIMB`), `0x40016980` (`TIMC`), `0x40016A00` (`TIMD`), `0x40016A80` (`TIME`), `0x40016B00` (`TIMF`) (stride `0x80`)
- `HRTIM1_COMMON`: `0x40016B80` (`HRTIM1_BASE + 0x0380UL`)  
**RCC Clock Enable:** `RCC->APB2ENR |= RCC_APB2ENR_HRTIM1EN` (Bit 26; requires $f_{\text{HRTIM}} \ge 100\ \text{MHz}$, max 170 MHz for 184 ps DLL!)  
**IRQ Numbers (`stm32g474xx.h`):** `HRTIM1_Master_IRQn=67`, `TIMA=68`, `TIMB=69`, `TIMC=70`, `TIMD=71`, `TIME=72`, `FLT=73`, `TIMF=74`  
**DMAMUX1 Request IDs (RM0440 Table 91):** `HRTIM1_M=95`, `HRTIM1_A=96`, `HRTIM1_B=97`, `HRTIM1_C=98`, `HRTIM1_D=99`, `HRTIM1_E=100`, `HRTIM1_F=101`  
**Reference Manual:** RM0440, Chapter 27 (*High-resolution timer (HRTIM)*) | AN4539 (*HRTIM cookbook*) | ES0430 §2.11  
**Concept IDs:** `PERIPH-HRTIM-001`  
**Verification Status:** Every base address, struct offset, and IRQ verified directly against `stm32g474xx.h`; code snippets Statically Reviewed (Not Tested on Physical Hardware).

---

### 1. Architectural Overview (5.44 GHz Equivalent Resolution)

The STM32G474RE **HRTIM1** embeds an internal **Delay-Locked Loop (DLL)** multiplying the 170 MHz APB2 timer clock by **×32** to achieve an equivalent clock of $f_{\text{HRTIM\_EQ}} = 5.44\ \text{GHz}$ (**184 ps resolution**). It comprises **1 Master Timer** + **6 independent 16-bit Timer Units (`TIMA..TIMF`)** driving **12 complementary high-resolution outputs (`CHA1/2..CHF1/2`)**, 10 External Events (`EEV1..10`, e.g., from `COMP1..7`), 6 Hardware Fault inputs (`FLT1..6`), Burst Mode controller, and 10 ADC triggers.

---

### 2. Master Timer Registers (`HRTIM_Master_TypeDef` @ `0x40016800`)

| Register | Offset | Access | Description |
|---|---|---|---|
| `MCR` | `0x000` | RW | Master control (`CKPSC[2:0]`, `CONT`, `RETRIG`, `HALF`, `MCEN`, `TACEN..TFCEN` timer counter enables, `PREEN`) |
| `MISR` | `0x004` | RO | Master interrupt status (`MCMP1..4`, `MREP`, `SYNC`, `MUPD`) |
| `MICR` | `0x008` | WO | Master interrupt clear (`MCMP1C..4C`, `MREPC`, `SYNCC`, `MUPDC`) |
| `MDIER` | `0x00C` | RW | Master interrupt & DMA enable |
| `MCNTR` | `0x010` | RW | Master 16-bit counter register |
| `MPER` | `0x014` | RW | Master period register |
| `MREP` | `0x018` | RW | Master repetition counter |
| `MCMP1R..MCMP4R` | `0x01C`, `0x024..0x02C` | RW | Master compare 1–4 registers (for inter-phase synchronization of TIMA–TIMF) |

---

### 3. Timer Unit x Registers (`HRTIM_Timerx_TypeDef` @ `0x40016880 + 0x80*x`)

| Register | Offset | Access | Description |
|---|---|---|---|
| `TIMxCR` | `0x000` | RW | Timer x control (`CKPSCx[2:0]`: `000` = ×32 DLL 184 ps, `CONT`, `PREEN`, `TxREPU`, `TxRSTU`) |
| `TIMxISR` / `TIMxICR` | `0x004` / `0x008` | RO / WO | Timer x status & clear (`CMP1..4`, `REP`, `UPD`, `CPT1/2`, `SETx1/RSTx1`, `DLYPRT`) |
| `TIMxDIER` | `0x00C` | RW | Timer x interrupt & DMA request enable |
| `CNTxR` / `PERxR` | `0x010` / `0x014` | RW | 16-bit counter & period registers (min valid `PERxR` depends on `CKPSCx` per RM0440 Table 216) |
| `REPxR` | `0x018` | RW | Repetition counter |
| `CMP1xR..CMP4xR` | `0x01C`, `0x024..0x02C` | RW | Compare 1–4 registers (`CMP1CxR` @ `0x020` for auto-delayed compare) |
| `CPT1xR` / `CPT2xR` | `0x030` / `0x034` | RO | Capture 1 & 2 registers |
| **`DTxR`** | `0x038` | RW | Dead-time register (`DTRx[8:0]` rising, `SDTRx`, `DTPRSC[12:10]`, `DTFx[24:16]` falling, `SDTFx`, lock bits) |
| **`SETx1R` / `RSTx1R`** | `0x03C` / `0x040` | RW | Crossbar Set/Reset event sources for Output 1 (`PER`, `CMP1..4`, `MSTPER`, `EXTEVNT1..10`) |
| **`SETx2R` / `RSTx2R`** | `0x044` / `0x048` | RW | Crossbar Set/Reset event sources for Output 2 |
| `EEFxR1..3` | `0x04C`, `0x050`, `0x070` | RW | External event filtering & blanking (ignore switch-node ringing!) |
| `RSTxR` | `0x054` | RW | Counter reset event sources (e.g., Master Period or Compare for phase-shift) |
| `OUTxR` | `0x064` | RW | Output config (`POL1/2`, `IDLEM1/2`, `IDLES1/2`, `FAULT1/2[1:0]` safe fault state, **`DTEN` bit 8** dead-time enable) |
| `FLTxR` | `0x068` | RW | Fault 1–6 enable & lock for Timer x |
| `TIMxCR2` | `0x06C` | RW | Dual-channel DAC trigger, up-down mode (`UDM`), rollover modes |

---

### 4. Common Control Registers (`HRTIM_Common_TypeDef` @ `0x40016B80`)

| Register | Offset | Access | Description |
|---|---|---|---|
| `CR1` / `CR2` | `0x000` / `0x004` | RW | Update disable (`MUDIS`, `TAUDIS..TFUDIS`), software update (`MSWU`, `TASWU..TFSWU`), software reset |
| `ISR` / `ICR` / `IER` | `0x008` / `0x00C` / `0x010` | RO / WO / RW | Fault flags (`FLT1..6`, `SYSFLT`), **`DLLRDY` (Bit 16)**, `BMPER` |
| **`OENR` / `ODISR`** | `0x014` / `0x018` | RW / WO | Output enable / disable (`TA1OEN`, `TA2OEN` .. `TF1OEN`, `TF2OEN`) |
| `ODSR` | `0x01C` | RO | Output disable status (detects fault-disabled outputs) |
| `BMCR..BMPER` | `0x020..0x02C` | RW | Burst mode controller (light-load skip-cycle operation) |
| `EECR1..3` | `0x030..0x038` | RW | External Event 1–10 source selection (`COMP1–7`, pins), polarity, sensitivity, digital filter |
| `ADC1R..ADC4R` | `0x03C..0x048` | RW | ADC trigger 1–4 crossbar (`ADCER/ADCUR/ADCPS1/2` @ `0x78..0x84`) |
| **`DLLCR`** | `0x04C` | RW | **DLL Calibration Control (`CAL` b0, `CALEN` b1 periodic auto-cal, `CALRTE[3:2]` calibration rate)** |
| `FLTINR1..4` | `0x050`, `0x054`, `0x088`, `0x08C` | RW | Fault 1–6 input enable, polarity, source (pin vs COMP), filter, blanking |
| `BDMUPR..BDTFUPR` | `0x058..0x074` | RW | Burst DMA update registers (`BDMADR` @ `0x070`) |

---

### 5. CMSIS Code Example: DLL Calibration + 200 kHz 184 ps Complementary PWM (`CHA1`/`CHA2`)

> **Hardware Note:** `PA8` (`HRTIM1_CHA1`) and `PA9` (`HRTIM1_CHA2`) use **AF13**. Remember to set `PWR->CR3 |= PWR_CR3_UCPD_DBDIS;` so the reset-enabled UCPD dead-battery pull-down on `PA9` does not load `CHA2`!  
> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

void HRTIM1_TIMA_200kHz_Init(void) {
    /* 1. Release PA9 dead-battery pull-down & enable HRTIM1 + GPIOA clocks */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR3      |= PWR_CR3_UCPD_DBDIS;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;
    RCC->APB2ENR  |= RCC_APB2ENR_HRTIM1EN;
    (void)RCC->APB2ENR;

    /* 2. MANDATORY: Start DLL calibration + periodic auto-cal (~14 us rate) & wait DLLRDY */
    HRTIM1_COMMON->DLLCR = HRTIM_DLLCR_CAL | HRTIM_DLLCR_CALEN |
                           (3U << HRTIM_DLLCR_CALRTE_Pos);
    while (!(HRTIM1_COMMON->ISR & HRTIM_ISR_DLLRDY)) { }

    /* 3. Configure TIMA: CKPSC = 000 (x32 = 5.44 GHz, 184 ps), Continuous, Preload */
    HRTIM1_TIMA->TIMxCR = HRTIM_TIMCR_CONT | HRTIM_TIMCR_PREEN;
    HRTIM1_TIMA->PERxR  = 27200U; /* 5.44 GHz / 200 kHz = 27200 counts */
    HRTIM1_TIMA->CMP1xR = 13600U; /* 50.00% duty cycle (1 count = 184 ps!) */

    /* 4. Output 1 set on Period, reset on CMP1; Dead-time enabled (drives CHA2 complementary) */
    HRTIM1_TIMA->SETx1R = HRTIM_SET1R_PER;
    HRTIM1_TIMA->RSTx1R = HRTIM_RST1R_CMP1;
    HRTIM1_TIMA->DTxR   = (3U << HRTIM_DTR_DTPRSC_Pos) | (100U << HRTIM_DTR_DTR_Pos) |
                          (100U << HRTIM_DTR_DTF_Pos);
    HRTIM1_TIMA->OUTxR  = HRTIM_OUTR_DTEN;

    /* 5. Route PA8 (CHA1) & PA9 (CHA2) to AF13, Very High Speed */
    GPIOA->AFR[1]  = (GPIOA->AFR[1]  & ~((0xFU << 0U) | (0xFU << 4U))) | ((13U << 0U) | (13U << 4U));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3U << 16U)  | (3U << 18U)))  | ((3U << 16U)  | (3U << 18U));
    GPIOA->MODER   = (GPIOA->MODER   & ~((3U << 16U)  | (3U << 18U)))  | ((2U << 16U)  | (2U << 18U));

    /* 6. Enable CHA1 & CHA2 outputs and start Timer A counter */
    HRTIM1_COMMON->OENR = HRTIM_OENR_TA1OEN | HRTIM_OENR_TA2OEN;
    HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_TACEN;
}
```
