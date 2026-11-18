# CORDIC, FMAC, CRC & RNG (Math & Security Accelerators) Register Map

**Target MCU:** STM32G474RE (LQFP64 — **Note: STM32G474RE has NO hardware AES/CRYP!** AES exists only on STM32G483/G484.)  
**Base Addresses:**
- `CORDIC`: `0x40020C00` (AHB1, `RCC_AHB1ENR_CORDICEN` b3, `CORDIC_IRQn = 100`, DMAMUX RD=`112`/WR=`113`)
- `FMAC`: `0x40021400` (AHB1, `RCC_AHB1ENR_FMACEN` b4, `FMAC_IRQn = 101`, DMAMUX RD=`110`/WR=`111`)
- `CRC`: `0x40023000` (AHB1, `RCC_AHB1ENR_CRCEN` b12)
- `RNG`: `0x50060800` (AHB2, `RCC_AHB2ENR_RNGEN` b26 + 48 MHz `CLK48SEL`, `RNG_IRQn = 90`)  
**Reference Manual:** RM0440, Ch 16 (*CRC*), Ch 17 (*CORDIC*), Ch 18 (*FMAC*), Ch 33 (*RNG*) | AN5325 & AN5305  
**Concept IDs:** `PERIPH-CORDIC-001`, `PERIPH-FMAC-001`, `PERIPH-CRC-001`, `PERIPH-RNG-001`, `PROF-PERF-003`, `SEC-CRYPTO-001`  
**Verification Status:** Every base address, struct offset, and IRQ verified directly against `stm32g474xx.h`; C code examples Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. CORDIC (Trigonometric & Hyperbolic Coprocessor) — `0x40020C00` (`CORDIC_TypeDef`)

Computes `cos`, `sin`, `atan2`, `hypot`, `atan`, `cosh`, `sinh`, `atanh`, `ln`, and `sqrt` in fixed-point **`q1.31` / `q1.15`** in **~3–6 HCLK cycles** (zero-wait-state AHB stall on `RDATA` eliminates polling!).

| Register | Offset | Reset Value | Access | Key Bitfields |
|---|---|---|---|---|
| `CORDIC_CSR` | `0x000` | `0x00000050` | RW | `FUNC[3:0]` (`0`=Cos, `1`=Sin, `2`=Phase, `3`=Modulus, `4`=Atan, `5`=Cosh, `6`=Sinh, `7`=Atanh, `8`=Ln, `9`=Sqrt), `PRECISION[7:4]` (`1..15` = 4..60 iter; `6` = 24 iter / 6 cycles), `SCALE[10:8]`, `IEN` b16, `DMAREN` b17, `DMAWEN` b18, `NRES` b19 (`0`=1, `1`=2), `NARGS` b20 (`0`=1, `1`=2), `RESSIZE` b21 (`0`=32b `q1.31`, `1`=16b `q1.15`), `ARGSIZE` b22, `RRDY` b31 (RO) |
| `CORDIC_WDATA` | `0x004` | `0x00000000` | WO | Argument write register (writing last argument starts calculation) |
| `CORDIC_RDATA` | `0x008` | `0x00000000` | RO | Result read register (AHB bus stalls automatically until ready!) |

---

### 2. FMAC (Filter Math Accelerator — 170 MMAC/s) — `0x40021400` (`FMAC_TypeDef`)

Dedicated `q1.15` DSP engine with **256 × 16-bit internal SRAM** partitioned into 3 circular buffers (`X1` input, `X2` coefficients, `Y` output/state) for FIR and IIR (e.g., 3p3z digital power compensator).

| Register | Offset | Reset Value | Access | Key Bitfields |
|---|---|---|---|---|
| `FMAC_X1BUFCFG` | `0x000` | `0x00000000` | RW | `X1_BASE[7:0]`, `X1_BUF_SIZE[15:8]`, `FULL_WM[25:24]` |
| `FMAC_X2BUFCFG` | `0x004` | `0x00000000` | RW | `X2_BASE[7:0]`, `X2_BUF_SIZE[15:8]` |
| `FMAC_YBUFCFG` | `0x008` | `0x00000000` | RW | `Y_BASE[7:0]`, `Y_BUF_SIZE[15:8]`, `EMPTY_WM[25:24]` |
| `FMAC_PARAM` | `0x00C` | `0x00000000` | RW | `P[7:0]` (feed-forward taps), `Q[15:8]` (feedback taps), `R[23:16]` (post-shift), `FUNC[30:24]` (`1`=Load X1, `2`=Load X2, `3`=Load Y, `8`=FIR, `9`=IIR), `START` b31 |
| `FMAC_CR` | `0x010` | `0x00000000` | RW | `RIEN`, `WIEN`, `OVFLIEN`, `UNFLIEN`, `SATIEN`, `DMAREN`, `DMAWEN`, `CLIPEN` b15, `RESET` b16 |
| `FMAC_SR` | `0x014` | `0x00000001` | RO | `YEMPTY` b0, `X1FULL` b1, `OVFL` b8, `UNFL` b9, `SAT` b10 |
| `FMAC_WDATA` | `0x018` | `0x00000000` | WO | 16-bit `q1.15` input sample/coefficient write register |
| `FMAC_RDATA` | `0x01C` | `0x00000000` | RO | 16-bit `q1.15` filtered output sample |

---

### 3. CRC (Programmable 7/8/16/32-Bit Hardware CRC) — `0x40023000` (`CRC_TypeDef`)

| Register | Offset | Reset Value | Access | Key Bitfields |
|---|---|---|---|---|
| `CRC_DR` | `0x000` | `0xFFFFFFFF` | RW | Data register (supports 8-, 16-, and 32-bit writes; reads current CRC) |
| `CRC_IDR` | `0x004` | `0x00000000` | RW | Independent 32-bit scratch register (unaffected by `CR.RESET`) |
| `CRC_CR` | `0x008` | `0x00000000` | RW | `RESET` b0 (reloads `INIT`), `POLYSIZE[4:3]` (`00`=32b, `01`=16b, `10`=8b, `11`=7b), `REV_IN[6:5]` (`01`=byte, `10`=halfword, `11`=word), `REV_OUT` b7 |
| `CRC_INIT` | `0x010` | `0xFFFFFFFF` | RW | Programmable initial CRC value |
| `CRC_POL` | `0x014` | `0x04C11DB7` | RW | Programmable generator polynomial (default = IEEE 802.3 Ethernet CRC-32) |

---

### 4. RNG (True Random Number Generator) — `0x50060800` (`RNG_TypeDef`)

Requires a **48 MHz kernel clock** via `RCC->CCIPR.CLK48SEL` (`00` = `HSI48`, `10` = `PLL48Q`) with $f_{\text{RNG\_CLK}} \ge f_{\text{HCLK}} / 16$ (48 MHz easily satisfies 170 MHz / 16 = 10.625 MHz).

| Register | Offset | Reset Value | Access | Key Bitfields |
|---|---|---|---|---|
| `RNG_CR` | `0x000` | `0x00000000` | RW | `RNGEN` b2 (enable), `IE` b3 (`RNG_IRQn = 90`), `CED` b5 (`0` = clock error detector enabled) |
| `RNG_SR` | `0x004` | `0x00000000` | RO/rc_w0 | `DRDY` b0 (32-bit word ready), `CECS` b1 (clock err status), `SECS` b2 (seed err status), `CEIS` b5 (`rc_w0`), `SEIS` b6 (`rc_w0`) |
| `RNG_DR` | `0x008` | `0x00000000` | RO | 32-bit NIST SP800-90B true random output (discard if `0` or `SEIS/CEIS` asserted) |

---

### 5. CMSIS Code Examples: CORDIC `cos/sin`, IEEE 802.3 CRC-32 & Hardware TRNG

> **Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) / Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

/* 1. CORDIC: Compute both cos(angle) and sin(angle) in ~6 CPU cycles (angle_q31 = rad / pi * 2^31) */
void CORDIC_CosSin_Q31(int32_t angle_q31, int32_t *cos_out, int32_t *sin_out) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;
    (void)RCC->AHB1ENR; /* Mandatory RCC clock-enable readback */
    /* FUNC=0 (Cos+Sin), PRECISION=6 (24 iter = 6 cycles), NRES=1 (2 results), q1.31 */
    CORDIC->CSR   = (0U << CORDIC_CSR_FUNC_Pos) | (6U << CORDIC_CSR_PRECISION_Pos) |
                    CORDIC_CSR_NRES;
    CORDIC->WDATA = (uint32_t)angle_q31;
    *cos_out      = (int32_t)CORDIC->RDATA; /* Zero-overhead AHB wait */
    *sin_out      = (int32_t)CORDIC->RDATA;
}

/* 2. CRC-32 IEEE 802.3 (Standard check value for "123456789" == 0xCBF43926) */
uint32_t CRC32_Compute(const uint8_t *data, uint32_t len) {
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    (void)RCC->AHB1ENR; /* Mandatory RCC clock-enable readback */
    CRC->POL  = 0x04C11DB7UL;
    CRC->INIT = 0xFFFFFFFFUL;
    CRC->CR   = CRC_CR_REV_IN_0 | CRC_CR_REV_OUT | CRC_CR_RESET; /* Byte bit-reverse + out reverse */
    for (uint32_t i = 0U; i < len; i++) {
        *(__IO uint8_t *)&CRC->DR = data[i];
    }
    return CRC->DR ^ 0xFFFFFFFFUL;
}

/* 3. RNG: Read one health-checked 32-bit true random word using HSI48 (RM0440 Ch. 33) */
int RNG_ReadWord(uint32_t *out, uint32_t timeout) {
    RCC->CRRCR   |= RCC_CRRCR_HSI48ON;
    while (!(RCC->CRRCR & RCC_CRRCR_HSI48RDY)) { }
    RCC->CCIPR   &= ~RCC_CCIPR_CLK48SEL_Msk; /* 00b = HSI48 */
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    (void)RCC->AHB2ENR; /* Mandatory RCC clock-enable readback */
    RNG->CR       = RNG_CR_RNGEN;

    while (timeout-- > 0U) {
        uint32_t sr = RNG->SR;
        if ((sr & (RNG_SR_CECS | RNG_SR_CEIS)) != 0U) {
            RNG->SR = ~RNG_SR_CEIS; /* Direct rc_w0 write (NEVER '&=') */
            return -1;              /* Clock configuration fault */
        }
        if ((sr & (RNG_SR_SECS | RNG_SR_SEIS)) != 0U) {
            /* RM0440 Ch. 33 seed-error recovery: clear SEIS via rc_w0 & cycle RNGEN */
            RNG->SR  = ~RNG_SR_SEIS;
            RNG->CR &= ~RNG_CR_RNGEN;
            RNG->CR |=  RNG_CR_RNGEN;
            continue;
        }
        if ((sr & RNG_SR_DRDY) != 0U) {
            uint32_t val = RNG->DR;
            if (val != 0U) { /* RM0440: discard zero word */
                *out = val;
                return 0;
            }
        }
    }
    return -2; /* Timed out */
}
```
