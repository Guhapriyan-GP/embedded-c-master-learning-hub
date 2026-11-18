# OPAMP1–6, COMP1–7 & VREFBUF (Mixed-Signal Analog Front-End) Register Map

**Target MCU:** STM32G474RET6 (LQFP64 — **6× 13 MHz OPAMPs**, **7× 16.7 ns Comparators**, **1× Precision VREFBUF**)  
**Base Addresses (APB2 — ALL clocked by `RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN`, Bit 0!):**
- `VREFBUF`: `0x40010030`
- `COMP1..COMP7`: `0x40010200` (`COMP1`), `0x40010204` (`COMP2`), `0x40010208` (`COMP3`), `0x4001020C` (`COMP4`), `0x40010210` (`COMP5`), `0x40010214` (`COMP6`), `0x40010218` (`COMP7`)
- `OPAMP1..OPAMP6`: `0x40010300` (`OPAMP1`), `0x40010304` (`OPAMP2`), `0x40010308` (`OPAMP3`), `0x4001030C` (`OPAMP4`), `0x40010310` (`OPAMP5`), `0x40010314` (`OPAMP6`)  
**IRQ Numbers (`stm32g474xx.h`):** `COMP1_2_3_IRQn = 64`, `COMP4_5_6_IRQn = 65`, `COMP7_IRQn = 66` (via EXTI lines 21, 22, 29–33)  
**Reference Manual:** RM0440, Ch 23 (*VREFBUF*), Ch 24 (*COMP*), Ch 25 (*OPAMP*) | AN5306 (*OPAMP usage in STM32G4*)  
**Concept IDs:** `PERIPH-OPAMP-001`, `PERIPH-COMP-001`, `PERIPH-VREFBUF-001`  
**Verification Status:** Every base address, struct offset, bitfield, and IRQ verified directly against `stm32g474xx.h`; C code examples Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. OPAMP1–6 Register Map (`OPAMP_TypeDef` — Stride `0x04` for `CSR`, `TCMR` at `+0x18`)

Rail-to-rail 13 MHz GBW op-amps with built-in **PGA gains ×2, ×4, ×8, ×16, ×32, ×64** (non-inverting) and **−1, −3, −7, −15, −31, −63** (inverting), internal direct routing to **ADC1–5 (`OPAMPINTEN = 1`)**, and Timer-Controlled Multiplexing (`TCMR`).

| Register | Offset | Reset Value | Access | Key Bitfields (RM0440 / CMSIS `stm32g474xx.h`) |
|---|---|---|---|---|
| `OPAMPx_CSR` | `+0x000` | `0x00000000` | RW | `OPAEN` / `OPAMP_CSR_OPAMPxEN` (b0), `FORCE_VP` / `FORCEVP` (b1), `VP_SEL` / `VPSEL[3:2]` (non-inv input), `USERTRIM` (b4), `VM_SEL` / `VMSEL[6:5]` (`00`/`01`=Standalone, **`10`=PGA**, `11`=Follower), **`HIGHSPEEDEN` (b7** = 13 MHz mode), **`OPAINTOEN` / `OPAMP_CSR_OPAMPINTEN` (b8** = route output internally to ADC, freeing pin!), `CALON` (b11), `CALSEL[13:12]`, **`PGA_GAIN` / `OPAMP_CSR_PGGAIN[18:14]`** (`00000`=×2, `00001`=×4, `00010`=×8, `00011`=×16, `00100`=×32, `00101`=×64; bit 18 adds external filtering/inverting), `TRIMOFFSETP[23:19]` (PMOS), `TRIMOFFSETN[28:24]` (NMOS), `CALOUT` / `OUTCAL` (b30 RO), `LOCK` (b31 write-once) |
| `OPAMPx_TCMR` | `+0x018` | `0x00000000` | RW | `VMS_SEL` (b0), `VPS_SEL[2:1]`, `T1CM_EN` (b3), `T8CM_EN` (b4), `T20CM_EN` (b5), `LOCK` (b31) — synchronously switches inputs with TIM1/8/20! |

---

### 2. COMP1–7 Register Map (`COMP_TypeDef` — Single `CSR` Register Each)

Ultra-fast **~16.7 ns** rail-to-rail comparators with programmable hysteresis (`0..70 mV`), hardware switching-noise **Blanking** synced to TIM/HRTIM, direct internal threshold from **DAC1–4** or fractional `VREFINT`, and direct hardware Break routing to **TIM1/8/15/16/17/20 `BKIN`** and **HRTIM1 Faults/EEV**.

| Register | Offset | Reset Value | Access | Key Bitfields (CMSIS `stm32g474xx.h`) |
|---|---|---|---|---|
| `COMPx_CSR` | `+0x000` | `0x00000000` | RW | `EN` (b0), `INMSEL[7:4]` (inverting input: `0000`=1/4 VREFINT, `0001`=1/2 VREFINT, `0010`=3/4 VREFINT, `0011`=VREFINT, `0100`/`0101`=DACx internal, `0110`/`0111`=external pin), `INPSEL` (b8 non-inv pin), `POL` (b15 invert), `HYST[18:16]` (`COMP_CSR_HYST`, `000`=0 mV .. `111`=70 mV), **`BLANKING[21:19]`** (`COMP_CSR_BLANKING`, TIM/HRTIM blanking), `BRGEN` (b22 VREFINT resistor bridge), `SCALEN` (b23 VREFINT scaler), `VALUE` (b30 RO output), **`LOCK` (b31** write-once safety lock until reset) |

---

### 3. VREFBUF (Internal Voltage Reference Buffer) — `0x40010030` (`VREFBUF_TypeDef`)

> **CRITICAL NUCLEO-G474RE Hardware Warning (UM2505):** On NUCLEO-G474RE (MB1367), the `VREF+` pin (LQFP64 Pin 13) is shorted to `VDDA` (3.3 V) via solder bridge by default! **NEVER enable `VREFBUF` (`ENVR=1, HIZ=0`) while `VREF+` is shorted to 3.3 V** — open the `VREF+` solder bridge first so the internal 2.048 V / 2.500 V / 2.900 V buffer does not fight the 3.3 V rail!

| Register | Offset | Reset Value | Access | Key Bitfields |
|---|---|---|---|---|
| `VREFBUF_CSR` | `0x000` | `0x00000002` | RW | `ENVR` (b0 enable), `HIZ` (b1 high-impedance; set `0` to drive `VREF+` pin), `VRR` (b3 RO ready flag), **`VRS[5:4]`** (`00`=**2.048 V**, `01`=**2.500 V**, `10`=**2.900 V**, `11`=Reserved) |
| `VREFBUF_CCR` | `0x004` | Factory Trim | RW | `TRIM[5:0]` factory trimming value |

---

### 4. CMSIS Code Examples: `OPAMP1` PGA ×8 → `ADC1` & `COMP1` → `TIM1` Hardware Overcurrent Break

> **Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) / Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

/* 1. Configure OPAMP1 as High-Speed Non-Inverting PGA x8 on PA1, routed internally to ADC1_IN13 */
void OPAMP1_PGA_x8_InternalADC_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; /* Clocks OPAMP1-6, COMP1-7, VREFBUF */
    (void)RCC->APB2ENR;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    GPIOA->MODER |= (3U << (1U * 2U)); /* PA1 Analog (OPAMP1_VINP) */

    /* CMSIS names vs RM0440: PGGAIN (PGA_GAIN), OPAMPINTEN (OPAINTOEN), OPAMPxEN (OPAEN) */
    OPAMP1->CSR = (0U << OPAMP_CSR_VPSEL_Pos)    |
                  (2U << OPAMP_CSR_VMSEL_Pos)    |
                  (2U << OPAMP_CSR_PGGAIN_Pos)   |
                  OPAMP_CSR_HIGHSPEEDEN          |
                  OPAMP_CSR_OPAMPINTEN           |
                  OPAMP_CSR_OPAMPxEN;
}

/* 2. Configure COMP1 (PA1 vs DAC3_CH1) wired directly to TIM1 BKIN (<50 ns zero-CPU fault trip!) */
void COMP1_Overcurrent_To_TIM1_Break_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    /* COMP1: INPSEL=0 (PA1), INMSEL=0100 (DAC3_CH1), HYST=010 (~20 mV), EN=1 */
    COMP1->CSR = (4U << COMP_CSR_INMSEL_Pos) | (2U << COMP_CSR_HYST_Pos) | COMP_CSR_EN;

    /* Route COMP1 output into TIM1 Break 1 (AF1.BKCMP1E = 1) and enable Break in BDTR */
    TIM1->AF1  |= TIM1_AF1_BKCMP1E;
    TIM1->BDTR |= TIM_BDTR_BKE | TIM_BDTR_OSSR | TIM_BDTR_OSSI;
}
```
