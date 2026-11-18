# PWR & FLASH Register Maps (Boost Mode, Latency & ECC)

**Target MCU:** STM32G474RE (LQFP64, 512 KB Dual-Bank Flash, NUCLEO-G474RE MB1367)  
**PWR Base Address:** `0x40007000` (`APB1PERIPH_BASE + 0x7000UL`)  
**FLASH Interface Base:** `0x40022000` (`AHB1PERIPH_BASE + 0x2000UL`)  
**Reference Manual:** RM0440, Chapter 3 (*Embedded Flash memory*) & Chapter 6 (*Power control (PWR)*)  
**Concept IDs:** `STM32-PWR-001`, `STM32-PWR-002`, `STM32-PWR-003`, `STM32-FLASH-001`, `STM32-FLASH-002`, `MEM-OB-001`  
**Verification Status:** Every register offset and bit position verified directly against `stm32g474xx.h`; C code snippets Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. PWR (Power Control) Register Map (`PWR_TypeDef` @ `0x40007000`)

Operating the STM32G474RE above 150 MHz (up to 170 MHz) requires enabling **Range 1 Boost Mode** (`PWR_CR5.R1MODE = 0`), verifying `PWR_SR2.VOSF == 0`, setting **4 Flash Wait States**, and stepping `HPRE = /2` for ≥ 1 µs per **RM0440 §6.1.5**.

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `PWR_CR1` | `0x000` | `0x00000200` | RW | Power control 1 (`LPMS[2:0]`, `DBP` bit 8, `VOS[10:9]`, `LPR` bit 14) |
| `PWR_CR2` | `0x004` | `0x00000000` | RW | Power control 2 (`PVDE`, `PLS[3:1]`, `PVME1`/`PVME3`/`PVME4`) |
| `PWR_CR3` | `0x008` | `0x00008000` | RW | Power control 3 (`EWUP1..5`, `RRS` bit 8, `UCPD_STDBY` bit 13, **`UCPD_DBDIS` bit 14**, `EIWUL` bit 15) |
| `PWR_CR4` | `0x00C` | `0x00000000` | RW | Power control 4 (`WP1..5` wakeup polarity, `VBE`/`VBRS` VBAT battery charging) |
| `PWR_SR1` | `0x010` | `0x00000000` | RO | Power status register 1 (`WUF1..5`, `SBF` Standby flag bit 8, `WUFI` bit 15) |
| `PWR_SR2` | `0x014` | `0x00000000` | RO | Power status register 2 (`REGLPS` bit 8, `REGLPF` bit 9, **`VOSF` bit 10**, `PVDO` bit 11, `PVMO1/3/4`) |
| `PWR_SCR` | `0x018` | `0x00000000` | WO | Power status clear register (`CWUF1..5`, `CSBF` bit 8) |
| `PWR_PUCRA..G` | `0x020..0x050` | `0x00000000` | RW | Standby/Shutdown pull-up configuration registers (ports A–G) |
| `PWR_PDCRA..G` | `0x024..0x054` | `0x00000000` | RW | Standby/Shutdown pull-down configuration registers (ports A–G) |
| **`PWR_CR5`** | **`0x080`** | `0x00000100` | RW | Power control 5 (**`R1MODE` bit 8**: `0` = Range 1 Boost Mode, `1` = Range 1 Normal) |

#### Critical PWR Bitfields
- **`PWR_CR1` Bits [2:0] (`LPMS`):** Low-power mode selection when `SCB->SCR.SLEEPDEEP = 1`:
  - `000`: Stop 0 | `001`: Stop 1 | `011`: Standby | `1xx`: Shutdown *(Note: **Stop 2 does NOT exist on STM32G4**).*
- **`PWR_CR1` Bit 8 (`DBP`):** Disable backup domain write protection (`1` = allows writes to `RTC`, `TAMP`, `RCC_BDCR`).
- **`PWR_CR1` Bits [10:9] (`VOS`):** Voltage scaling range (`01` = Range 1 [≤150 MHz normal / ≤170 MHz boost], `10` = Range 2 [≤26 MHz]).
- **`PWR_CR3` Bit 8 (`RRS`):** SRAM2 retention in Standby mode (`1` = 16 KB SRAM2 powered in Standby).
- **`PWR_CR3` Bit 14 (`UCPD_DBDIS`):** USB Type-C Dead-Battery pull-down disable. **CRITICAL GOTCHA:** At reset `UCPD_DBDIS = 0`, enabling internal 5.1 kΩ pull-downs on `PA9`/`PA10` (`UCPD1_DBCC1`/`DBCC2`) and `PB6`/`PB4`! Set `PWR->CR3 |= PWR_CR3_UCPD_DBDIS;` whenever using those pins for GPIO, USART1, HRTIM_CHA2, or I2C/SPI!
- **`PWR_SR2` Bit 10 (`VOSF`):** Voltage scaling flag (RO, located in **`PWR_SR2` at offset `0x14`**, NOT `PWR_SR1`!). `0` = regulator ready at selected VOS range; `1` = transition in progress.
- **`PWR_CR5` Bit 8 (`R1MODE`):** Offset **`0x080`** (NOT `0x10`!). `0` = Range 1 Boost Mode (up to 170 MHz); `1` = Range 1 Normal Mode (reset default, max 150 MHz).

---

### 2. FLASH Interface Register Map (`FLASH_TypeDef` @ `0x40022000`)

STM32G474RE contains **512 KB Dual-Bank Flash** (`DBANK=1`: 2×256 KB, **2 KB pages**, Read-While-Write) or Single-Bank (`DBANK=0`: 512 KB, **4 KB pages**). All Flash programming writes **64-bit aligned double-words** protected by **8-bit SECDED ECC**.

| Register | Offset | Reset Value | Access | Description |
|---|---|---|---|---|
| `FLASH_ACR` | `0x000` | `0x00040600` | RW | Access control (`LATENCY[3:0]`, `PRFTEN`, `ICEN`, `DCEN`, `ICRST`, `DCRST`, `DBG_SWEN`) |
| `FLASH_PDKEYR` | `0x004` | `0x00000000` | WO | Power-down key register |
| `FLASH_KEYR` | `0x008` | `0x00000000` | WO | Flash key register (`0x45670123`, `0xCDEF89AB` to unlock `FLASH_CR`) |
| `FLASH_OPTKEYR` | `0x00C` | `0x00000000` | WO | Option byte key register (`0x08192A3B`, `0x4C5D6E7F` to unlock `OPTLOCK`) |
| `FLASH_SR` | `0x010` | `0x00000000` | RW/rc_w1 | Status register (`EOP`, `OPERR`, `PROGERR`, `WRPERR`, `PGAERR`, `SIZERR`, `PGSERR`, `MISERR`, `FASTERR`, `RDERR`, `OPTVERR`, `BSY` bit 16) |
| `FLASH_CR` | `0x014` | `0xC0000000` | RW | Control register (`PG`, `PER`, `MER1`, `PNB[9:3]`, `BKER` bit 11, `MER2`, `STRT`, `OPTSTRT`, `EOPIE`, `ERRIE`, `OBL_LAUNCH`, `SEC_PROT1/2`, `OPTLOCK`, `LOCK`) |
| `FLASH_ECCR` | `0x018` | `0x00000000` | RW | ECC register (`ADDR_ECC[18:0]`, `BK_ECC` bit 21, `SYSF_ECC` bit 22, `ECCIE` bit 24, `ECCC` bit 30 [`rc_w1`], `ECCD` bit 31 [`rc_w1`]) |
| `FLASH_OPTR` | `0x020` | Option Byte | RW | Option register (`RDP[7:0]`, `BOR_LEV[10:8]`, `BFB2` bit 20, `DBANK` bit 22, `nBOOT1` bit 23, `nSWBOOT0` bit 26, `nBOOT0` bit 27, `NRST_MODE[29:28]`) |
| `FLASH_PCROP1SR/ER` | `0x024/0x028` | Option Byte | RW | Bank 1 PCROP Start/End address registers |
| `FLASH_WRP1AR/BR` | `0x02C/0x030` | Option Byte | RW | Bank 1 Write Protection Area A/B registers |
| `FLASH_PCROP2SR/ER` | `0x044/0x048` | Option Byte | RW | Bank 2 PCROP Start/End address registers |
| `FLASH_WRP2AR/BR` | `0x04C/0x050` | Option Byte | RW | Bank 2 Write Protection Area A/B registers |
| `FLASH_SEC1R` | `0x070` | Option Byte | RW | Securable Memory Area Bank 1 (`SEC_SIZE1[7:0]`, `BOOT_LOCK` bit 16) |
| `FLASH_SEC2R` | `0x074` | Option Byte | RW | Securable Memory Area Bank 2 (`SEC_SIZE2[7:0]`) |

#### Wait State Matrix (RM0440 Table 9 — Verified)

| `FLASH_ACR.LATENCY` | Range 1 Boost (`R1MODE=0`, ≤ 170 MHz) | Range 1 Normal (`R1MODE=1`, ≤ 150 MHz) | Range 2 (`VOS=10`, ≤ 26 MHz) |
|---|---|---|---|
| `0000` (0 WS) | $\le 34\ \text{MHz}$ | $\le 30\ \text{MHz}$ | $\le 12\ \text{MHz}$ |
| `0001` (1 WS) | $\le 68\ \text{MHz}$ | $\le 60\ \text{MHz}$ | $\le 24\ \text{MHz}$ |
| `0010` (2 WS) | $\le 102\ \text{MHz}$ | $\le 90\ \text{MHz}$ | $\le 26\ \text{MHz}$ |
| `0011` (3 WS) | $\le 136\ \text{MHz}$ | $\le 120\ \text{MHz}$ | — |
| **`0100` (4 WS)** | **$\le 170\text{ MHz}$ (Mandatory for BOTH 160 MHz & 170 MHz!)** | $\le 150\text{ MHz}$ | — |

#### `FLASH_ACR` ART Accelerator, `FLASH_ECCR` & Option-Byte `OBL_LAUNCH` Mechanics (`MEM-OB-001`)
- **`FLASH_ACR`:** `PRFTEN` (bit 8, Prefetch), `ICEN` (bit 9, 1 KB ICache), `DCEN` (bit 10, 256 B DCache), `ICRST` (bit 11, reset ICache while `ICEN=0`), `DCRST` (bit 12, reset DCache while `DCEN=0` — mandatory after Flash write per **ES0430 §2.2.2**).
- **`FLASH_ECCR`:** `ECCC` (bit 30, 1-bit corrected error, triggers `FLASH_IRQn = 4` if `ECCIE=1`), `ECCD` (bit 31, 2-bit uncorrectable double-error, triggers **NMI**), `ADDR_ECC[18:0]` (offset of faulting 64-bit word), `BK_ECC` (bit 21: `0`=Bank1, `1`=Bank2). Clear `ECCC`/`ECCD` by writing `1` directly.
- **Option-Byte Commit & Cold-Reset Mechanics (`FLASH_CR` & `FLASH_OPTR`, `MEM-OB-001`):**
  1. Unlock BOTH `FLASH->KEYR` (`LOCK` b31) AND `FLASH->OPTKEYR` (`OPTLOCK` b30); require `HSI16` ON!
  2. **`OPTSTRT` (bit 17):** Programs modified `FLASH->OPTR` into Option Flash (`BSY` poll) without reloading active hardware latches.
  3. **`OBL_LAUNCH` (bit 27):** Immediately triggers an **Option-Byte Loader Cold System Reset** (`RCC->CSR.OBLRSTF` b25) — never returns! Always compare desired vs current `FLASH->OPTR` first (idempotency guard!) to avoid infinite reset loops.
  4. **`BFB2` (b20) / `DBANK` (b22) Brick Traps:** Validate Bank 2 `MSP` (SRAM), `Reset_Handler` (Thumb), and CRC-32 *before* setting `BFB2=1`; never flip `DBANK` while `BFB2=1`. Recover via `STM32_Programmer_CLI -c port=SWD mode=UR -ob BFB2=0 DBANK=1 nSWBOOT0=1`.

---

### 3. CMSIS Code Example: Pre-Clock Boost Setup & RM0440 §6.1.5 Safe Transition

> **Status:** Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

void PWR_FLASH_Prepare_170MHz_Boost(void) {
    /* 1. Enable PWR clock on APB1 and read back for bus sync */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;

    /* 2. Disable UCPD dead-battery 5.1 kOhm pull-downs on PA9/PA10/PB6/PB4 */
    PWR->CR3 |= PWR_CR3_UCPD_DBDIS;

    /* 3. Ensure Voltage Scaling is Range 1 (VOS = 01b) and poll PWR_SR2.VOSF == 0 */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS_Msk) | PWR_CR1_VOS_0;
    while (PWR->SR2 & PWR_SR2_VOSF) { }

    /* 4. Enable Range 1 Boost Mode in PWR_CR5 (Offset 0x80, clear R1MODE bit 8) */
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 5. Configure Flash Latency to 4 Wait States + ART Prefetch/I-Cache/D-Cache */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) |
                 FLASH_ACR_LATENCY_4WS |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_4WS) { }

    /* 6. Follow with RM0440 §6.1.5 safe SYSCLK switch:
     *    HPRE = /2 (85 MHz) -> SW = PLL -> wait SWS == PLL -> wait >= 1 us -> HPRE = /1 (170 MHz) */
}
```
