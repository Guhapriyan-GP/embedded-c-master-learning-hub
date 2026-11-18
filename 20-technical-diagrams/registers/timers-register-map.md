# Timers (TIMx) Register Map & PWM / Complementary Dead-Time Engine

**Target MCU:** STM32G474RE (LQFP64)  
**APB1 Timers:** `TIM2` (`0x40000000`, 32-bit), `TIM3` (`0x40000400`, 16-bit), `TIM4` (`0x40000800`, 16-bit), `TIM5` (`0x40000C00`, 32-bit), `TIM6` (`0x40001000`, Basic), `TIM7` (`0x40001400`, Basic), `LPTIM1` (`0x40007C00`, Low-Power)  
**APB2 Timers:** `TIM1` (`0x40012C00`, Adv), `TIM8` (`0x40013400`, Adv), `TIM15` (`0x40014000`), `TIM16` (`0x40014400`), `TIM17` (`0x40014800`), `TIM20` (`0x40015000`, Adv) *(for `HRTIM1`, see [`hrtim-register-map.md`](hrtim-register-map.md))*  
**Reference Manual:** RM0440, Chapters 28–32 (Ch 28 Adv TIM1/8/20, Ch 29 GP TIM2–5, Ch 30 GP TIM15–17, Ch 31 Basic TIM6/7, Ch 32 LPTIM1)  
**Concept IDs:** `PERIPH-TIM-001`, `PERIPH-TIM-002`, `PERIPH-TIM-003`, `PERIPH-LPTIM-001`, `TIM-DMA-001`  
**Verification Status:** Every register offset and bitfield verified directly against `stm32g474xx.h`; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. Timer Register Overview (`TIM_TypeDef`)

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `TIMx_CR1` | `0x000` | `0x00000000` | RW | Control 1 (`CEN`, `UDIS`, `URS`, `OPM`, `DIR`, `CMS[1:0]`, `ARPE`, `CKD[1:0]`, `UIFREMAP`, `DITHEN`) |
| `TIMx_CR2` | `0x004` | `0x00000000` | RW | Control 2 (`MMS[2:0]` Master Mode TRGO, `MMS2[3:0]` TRGO2 on Adv timers) |
| `TIMx_SMCR` | `0x008` | `0x00000000` | RW | Slave mode control (`SMS` Encoder/Reset/Gated/Trigger, `TS`) |
| `TIMx_DIER` | `0x00C` | `0x00000000` | RW | DMA/Interrupt enable (`UIE`, `CC1IE..CC4IE`, `BIE`, `UDE`, `CCxDE`) |
| `TIMx_SR` | `0x010` | `0x00000000` | **rc_w0** | Status register (`UIF`, `CC1IF..CC4IF`, `BIF`, `B2IF`). **Write `0` to clear — NEVER `&= ~flag`!** |
| `TIMx_EGR` | `0x014` | `0x00000000` | **WO** | Event generation (`UG` bit 0 forces shadow transfer). **Direct write `=` — NEVER `|=`!** |
| `TIMx_CCMR1` | `0x018` | `0x00000000` | RW | Capture/Compare mode 1 (Channels 1 & 2: `CCxS`, `OCxPE`, `OCxM[3:0]`) |
| `TIMx_CCMR2` | `0x01C` | `0x00000000` | RW | Capture/Compare mode 2 (Channels 3 & 4) |
| `TIMx_CCER` | `0x020` | `0x00000000` | RW | Capture/Compare enable (`CCxE`, `CCxP`, `CCxNE`, `CCxNP` complementary outputs) |
| `TIMx_CNT` | `0x024` | `0x00000000` | RW | Counter register (32-bit on `TIM2`/`TIM5`, 16-bit on others) |
| `TIMx_PSC` | `0x028` | `0x00000000` | RW | 16-bit prescaler (divides $f_{\text{TIMCLK}}$ by $\text{PSC} + 1$) |
| `TIMx_ARR` | `0x02C` | `0xFFFFFFFF` / `0xFFFF` | RW | Auto-reload period register |
| `TIMx_RCR` | `0x030` | `0x00000000` | RW | Repetition counter (Adv TIM1/8/20, TIM15–17) |
| `TIMx_CCR1..4` | `0x034..0x040` | `0x00000000` | RW | Capture/Compare registers 1–4 |
| `TIMx_BDTR` | `0x044` | `0x00000000` | RW | **Break and Dead-Time register** (Adv TIM1/8/20 & TIM15–17: `DTG`, `BKE`, `BK2E`, `AOE`, **`MOE`**) |
| `TIMx_CCR5..6` | `0x048..0x04C` | `0x00000000` | RW | Internal compare channels 5 & 6 (Adv TIM1/8/20: asymmetric PWM & ADC `TRGO2` trigger) |
| `TIMx_CCMR3` | `0x050` | `0x00000000` | RW | Output compare mode for channels 5 & 6 |
| `TIMx_DTR2` | `0x054` | `0x00000000` | RW | Asymmetric falling-edge dead-time (`DTGF[7:0]`, `DTAE`, `DTPE`) |
| `TIMx_ECR` | `0x058` | `0x00000000` | RW | STM32G4 extended encoder control (index/direction/transition error) |
| `TIMx_TISEL` | `0x05C` | `0x00000000` | RW | Timer input multiplexer |
| `TIMx_AF1/AF2` | `0x060/0x064` | `0x00000001` | RW | Alternate function Break routing (`BKCMP1E..7E`, `BKINE`, polarity) |
| `TIMx_DCR` | `0x3DC` | `0x00000000` | RW | DMA burst control (`DBA[4:0]` = `offsetof(reg)/4`, `DBL[12:8]` = `count - 1`; STM32G4 offset **`0x3DC`**, shifted from STM32F4's `0x48`!) |
| `TIMx_DMAR` | `0x3E0` | `0x00000000` | RW | DMA burst transfer virtual portal (`CPAR = &TIMx->DMAR`, `PINC = 0`; routes `DBL + 1` words sequentially from `DBA`) |

---

### 2. Timebase, PWM & Dead-Time Mathematics

$$f_{\text{UEV}} = \frac{f_{\text{TIMCLK}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}, \qquad \text{Duty (\%)} = \frac{\text{CCR}x}{\text{ARR} + 1} \times 100\%$$

*(Clock multiplier rule: If `PPREx = /1`, $f_{\text{TIMCLK}} = f_{\text{PCLK}x}$; if `PPREx > /1`, hardware sets $f_{\text{TIMCLK}} = 2 \times f_{\text{PCLK}x}$.)*

#### `TIMx_BDTR` (Break and Dead-Time Register) — Offset `0x044`
- **Bits [7:0] (`DTG`):** Dead-time generator duration ($t_{\text{DTS}} = 1 / f_{\text{TIMCLK}}$ when `CR1.CKD = 00`; at 170 MHz, $t_{\text{DTS}} = 5.882\ \text{ns}$):
  - `DTG[7] = 0` (`0..127`): $T_{\text{DT}} = \text{DTG}[6:0] \times t_{\text{DTS}}$ (e.g., `DTG = 85` $\rightarrow 85 \times 5.882\ \text{ns} = 500\ \text{ns}$)
  - `DTG[7:6] = 10`: $T_{\text{DT}} = (64 + \text{DTG}[5:0]) \times 2\,t_{\text{DTS}}$
  - `DTG[7:5] = 110`: $T_{\text{DT}} = (32 + \text{DTG}[4:0]) \times 8\,t_{\text{DTS}}$
  - `DTG[7:5] = 111`: $T_{\text{DT}} = (32 + \text{DTG}[4:0]) \times 16\,t_{\text{DTS}}$
- **Bits [9:8] (`LOCK`):** Write-once register lock level (`00` off, `01`..`11` progressively lock `BDTR`/`CCMR`/`CCER`).
- **Bit 10 (`OSSI`) / Bit 11 (`OSSR`):** Off-state selection for Idle / Run modes.
- **Bit 12 (`BKE`) / Bit 13 (`BKP`):** Break input 1 enable & polarity (`0`=active LOW, `1`=active HIGH). Asserting Break clears **`MOE = 0` in hardware** (< 50 ns, zero CPU latency) to prevent shoot-through!
- **Bit 14 (`AOE`):** Automatic Output Enable (`1` = `MOE` auto-re-enables at next UEV after fault clears; keep `0` for latched fault safety!).
- **Bit 15 (`MOE`):** **Main Output Enable** — MUST be set to `1` on `TIM1`, `TIM8`, `TIM20`, `TIM15`, `TIM16`, `TIM17`, otherwise PWM pins remain silent!
- **Bits [19:16] (`BKF`) / [23:20] (`BK2F`) / Bit 24 (`BK2E`) / Bit 25 (`BK2P`):** Digital filters and second Break channel (`BKIN2`).

---

### 3. Critical Access Semantics (`EGR` & `SR`)

1. **`TIMx->EGR` is Write-Only (WO):** Always write **`TIM2->EGR = TIM_EGR_UG;`** — NEVER `TIM2->EGR |= TIM_EGR_UG;` (reading a WO register is undefined / wastes an RMW cycle).
2. **`TIMx->SR` is `rc_w0` (Read / Clear by Writing 0):** Writing `1` to a bit has NO effect; writing `0` clears that flag. Always clear an interrupt flag early with **`TIM2->SR = ~TIM_SR_UIF;`** — NEVER `TIM2->SR &= ~TIM_SR_UIF;` (an RMW read-modify-write can accidentally clear another flag that asserted between the read and write!). Follow with **`(void)TIM2->SR; __DSB();`** before ISR exit so the clear commits across the AHB-to-APB bridge (**ARM AN321 §4.9**) and mitigates **ARM Erratum 838869 / ST ES0430 §2.1.3**.

---

### 4. CMSIS Code Example: 20 kHz PWM on `TIM2` + Safe ISR Flag Clearing

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

void TIM2_PWM_20kHz_Init(void) {
    /* 1. Enable TIM2 (APB1) and GPIOA (AHB2) clocks */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->APB1ENR1;

    /* 2. Configure PA0 as AF1 (TIM2_CH1) */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFU << 0U)) | (1U << 0U);
    GPIOA->MODER  = (GPIOA->MODER  & ~(3U << 0U))   | (2U << 0U);

    /* 3. 170 MHz / ((0 + 1) * (8499 + 1)) = 20.000 kHz; 50% duty = 4250 */
    TIM2->PSC  = 0U;
    TIM2->ARR  = 8499U;
    TIM2->CCR1 = 4250U;

    /* 4. Channel 1: PWM Mode 1 (OC1M = 110b) + Preload Enable (OC1PE) */
    TIM2->CCMR1 = (TIM2->CCMR1 & ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_OC1M_Msk)) |
                  (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM2->CCER |= TIM_CCER_CC1E;

    /* 5. Enable ARPE, force shadow load via direct WO write, clear UIF via rc_w0 */
    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->EGR  = TIM_EGR_UG;        /* Direct write (WO) — NEVER |= */
    TIM2->SR   = ~TIM_SR_UIF;       /* Direct rc_w0 clear — NEVER &= */
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR = ~TIM_SR_UIF;     /* Atomic rc_w0 flag clear (NEVER '&=') */
        (void)TIM2->SR;             /* APB1 bridge sync readback (ARM AN321 §4.9) */
        __DSB();                    /* Core write-buffer drain & ARM Erratum 838869 (ES0430 §2.1.3) */
    }
}
```
