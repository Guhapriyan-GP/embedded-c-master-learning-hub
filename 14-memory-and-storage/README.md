# Stage 13: Embedded Memory Subsystems, Dual-Bank Flash, ECC & Bootloaders

> **Directory:** `14-memory-and-storage/` | **Target Platform:** STM32G474RET6 (512 KB Dual-Bank ECC Flash, 80 KB SRAM1 + 16 KB SRAM2 + 32 KB CCM-SRAM)  
> **Code Verification Notice:** All bare-metal C snippets in this module are **Statically Reviewed against `stm32g474xx.h`, `core_cm4.h`, RM0440, AN2606, and ES0430 (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `BOOT-FLASH-001` (`STM32-FLASH-002`) | 512 KB Flash Dual-Bank (`DBANK=1`, 2 KB Pages, RWW) vs Single-Bank (`DBANK=0`, 4 KB Pages), **Mandatory 64-Bit Double-Word Programming + 8-Bit ECC (`FLASH->ECCR`)** | `[STM32G474RE Exact]` | Level 2 (Production) |
| `STM32-FLASH-001` (`PROF-PERF-002`) | ART Accelerator (`PRFTEN`, 1 KB `ICEN`, 256 B `DCEN`) & Post-Write Cache Flush (`DCRST`/`ICRST`, ES0430 §2.2.2) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `STM32-SRAM-001` | SRAM1 (80 KB), SRAM2 (16 KB Parity + Standby `RRS`), & CCM-SRAM (32 KB Parity + `SYSCFG->SWPR` 1 KB Write-Protect) | `[STM32G474RE Exact]` | Level 2 (Production) |
| `BOOT-JUMP-001` | Bootloader Partitioning, `SCB->VTOR` 512-Byte Alignment, & **9-Step Safe Application Jump Sequence** | `[ARM Cortex-M4F]` / `[STM32G474RE Exact]` | Level 2 (Production) |
| `BOOT-VERIF-001` / `BOOT-UPD-001` | Option Bytes (`OPTR`), Dual-Bank A/B Swap (`BFB2`/`FB_MODE`), Trial-Boot Rollback, Hardware CRC-32 & Ed25519 Verification, ST ROM Bootloader (`AN2606`) | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `BOOT-CFG-001` / `REL-CRASH-001` | `.noinit (NOLOAD)` SRAM & `TAMP->BKP0R..31R` Persistent Storage Across Reset/Standby | `[STM32G474RE Exact]` | Level 2 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs (Corrected — Zero Off-By-One Errors!)
- **Stage 04 (`05-toolchain-and-debugging/`):** `TOOL-LD-001`, `TOOL-START-001` (Linker scripts `MEMORY`/`SECTIONS`, VMA vs LMA, vector table layout, `Reset_Handler`).
- **Stage 05 (`06-stm32-fundamentals/`):** `STM32-ARCH-001` (Full STM32G474RE memory map: Flash, System ROM, OTP, Option Bytes, SRAM1/2/CCM).
- **Stage 06 (`07-rcc-and-clocks/`):** `STM32-RCC-001`, `STM32-FLASH-001` (`HSI16` mandatory during Flash write/erase; wait states).
- **Stage 08 (`10-interrupts/`):** `RT-IRQ-001` (NVIC `ICER`/`ICPR`, `SysTick`, `__disable_irq()`).
- **Stage 10 (`11-communication/`):** `PERIPH-UART-001`, `PERIPH-CRC-001` (for IAP transport & hardware CRC-32 verification).

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 512 KB Embedded Flash: Dual-Bank vs Single-Bank, 64-Bit ECC & RWW `[STM32G474RE Exact]`
STM32G474RE (Category 3 device) implements **512 KB Flash (`0x08000000..0x0807FFFF`)** rated for 10,000 erase/program cycles per page:
- **Dual-Bank Mode (`FLASH->OPTR.DBANK = 1`, Factory Default):**
  - **Bank 1:** `0x08000000..0x0803FFFF` (256 KB, **128 × 2 KB pages**).
  - **Bank 2:** `0x08040000..0x0807FFFF` (256 KB, **128 × 2 KB pages**).
  - **Read-While-Write (RWW):** CPU can execute code or service zero-latency ISRs from **Bank 1** while simultaneously erasing or programming **Bank 2** with **zero CPU stalls**!
- **Single-Bank Mode (`FLASH->OPTR.DBANK = 0`):**
  - 1 contiguous 512 KB bank of **128 × 4 KB pages** (no RWW; CPU stalls if fetching from Flash during erase/program).
- **CRITICAL STM32G4 Programming Rules:**
  1. **Mandatory 64-Bit Double-Word Programming:** You CANNOT write 8-bit, 16-bit, or isolated 32-bit words! Every write (`FLASH->CR.PG = 1`) MUST write **two consecutive 32-bit words at an 8-byte aligned address** (generating a 72-bit word: 64 data bits + **8-bit SECDED ECC**!). Misalignment triggers `PGAERR`/`SIZERR`/`PGSERR`.
  2. **8-Bit SECDED ECC (`FLASH->ECCR`):** Automatically corrects any 1-bit error (`ECCC`, optional `FLASH_IRQn = 4` via `ECCIE`) and detects 2-bit errors (`ECCD` $\to$ triggers **NMI**!).
  3. **`HSI16` Must Be ON:** The Flash erase/program state machine requires `RCC->CR.HSION = 1` (`HSIRDY = 1`) even when `SYSCLK` runs on HSE+PLL!
  4. **ART Cache Invalidation (`ES0430 §2.2.2`):** After erasing/programming Flash, flush DCache/ICache (`DCRST`/`ICRST` while `DCEN=0`/`ICEN=0`) before reading back modified addresses so stale cached lines are not returned!

### 3.2 SRAM Subsystem: SRAM1, SRAM2 & CCM-SRAM Integrity Features `[STM32G474RE Exact]`
| Region | Address Range | Size | Hardware Features |
|---|---|---|---|
| **SRAM1** | `0x20000000..0x20013FFF` | 80 KB | Main S-Bus RAM, DMA1/DMA2 accessible; first 32 KB supports parity check (`FLASH->OPTR.SRAM_PE`) |
| **SRAM2** | `0x20014000..0x20017FFF` | 16 KB | Contiguous after SRAM1, **hardware byte parity check**, optional **Standby Mode retention (`PWR->CR3.RRS = 1`)** |
| **CCM-SRAM** | `0x10000000..0x10007FFF` (Native I/D) / `0x20018000..0x2001FFFF` (S-Bus Alias) | 32 KB | **0-wait-state execution & data** at 170 MHz, **hardware parity (`SYSCFG->CFGR2.SPF`)**, **1 KB granularity write-protection (`SYSCFG->SWPR` — 32 bits, locked until reset!)**, hardware secure erase (`SYSCFG->SCSR.CCMER`) |

> **Parity Warning:** If `FLASH->OPTR.SRAM_PE = 0` (hardware parity enabled), `Reset_Handler` MUST initialize **every word** of parity-protected SRAM before reading (including unused stack/heap!) to generate valid parity bits; otherwise reading uninitialized SRAM fires a spurious parity NMI!

### 3.3 Option Bytes (`FLASH->OPTR`) & ST Factory Bootloader (`AN2606`) `[STM32G474RE Exact]`
- Stored at `0x1FFF7800` (unlocked via `FLASH->OPTKEYR`: `0x08192A3B`, then `0x4C5D6E7F`, committed via `OPTSTRT` and loaded via `OBL_LAUNCH`).
- Key bits: `RDP[7:0]` (Readout Protection), `BOR_LEV[2:0]` (Brown-Out levels 0–4), `BFB2` (bit 20: Dual-Bank Boot Bank 2 swap), `DBANK` (bit 22), `nBOOT1` (b23), `SRAM_PE` (b24), `nSWBOOT0` (b26: `1`=use `PB8-BOOT0` pin, `0`=use `nBOOT0` b27), `NRST_MODE[29:28]`.
- **ST System ROM Bootloader (`0x1FFF0000`, 28 KB):** Activated via `PB8-BOOT0 = 1` (`nBOOT1 = 1`) OR automatically on blank Flash (`FLASH->ACR.EMPTY = 1`), supporting flashing over USART1/2/3, I2C2/3/4, SPI1/2/3, **FDCAN1**, and **USB DFU**.

### 3.4 Mandatory 9-Step Safe Application Jump Sequence (`BOOT-JUMP-001`) `[ARM Cortex-M4F]`
A naive 3-line bootloader jump (`VTOR`, `__set_MSP`, `app_reset()`) crashes real firmware because pending SysTick/UART IRQs, active PLL/DMA, or non-zero `CONTROL` (PSP) immediately fault inside the new Application! Always execute the complete **9-step ARMv7-M handoff**:
1. **Validate App Vector Table:** Verify `app_msp = *(uint32_t *)app_addr` lies strictly inside valid SRAM (`0x20000000..0x20020000`, 8-byte aligned) AND `app_reset = *(uint32_t *)(app_addr + 4)` has **Thumb bit 0 == `1`** and lies within valid App Flash bounds!
2. **Disable Global Interrupts:** `__disable_irq();`
3. **Halt SysTick:** `SysTick->CTRL = 0U; SysTick->LOAD = 0U; SysTick->VAL = 0U;`
4. **Disable & Clear ALL NVIC Interrupts:** Loop `i = 0..3`: `NVIC->ICER[i] = 0xFFFFFFFFUL; NVIC->ICPR[i] = 0xFFFFFFFFUL;`
5. **De-Initialize Bootloader Peripherals & Reset Clocks:** Disable DMA/UART/TIM used by bootloader; optionally restore `SYSCLK` to HSI16 (or document clock handoff contract in ICD).
6. **Reset Core `CONTROL` Register (if Bootloader used PSP/Unprivileged):** `__set_CONTROL(0U);`
7. **Relocate Vector Table (`SCB->VTOR`):** Must be **512-byte aligned** on STM32G474RE (118 vectors × 4 B = 472 B $\implies$ next power of 2 = **`0x200` = 512 bytes**!). Write `SCB->VTOR = app_addr;`.
8. **Synchronize Barriers:** `__DSB(); __ISB();`
9. **Load App Main Stack Pointer & Branch:** `__set_MSP(app_msp); ((void (*)(void))app_reset)();` (never returns; avoid accessing bootloader local stack variables after `__set_MSP()`!).

### 3.5 A/B OTA Rollback, Cryptographic Verification & Persistent Storage
- **Dual-Bank A/B Swap (`BFB2` + `SYSCFG->MEMRMP.FB_MODE`):** Stage incoming OTA image into inactive Bank 2 while running Bank 1. Verify header magic, image length, **Hardware `CRC-32`** (detects accidental corruption), and **Ed25519 / ECDSA-P256 signature** (prevents malicious tampering — remember CRC is integrity-only, NOT security!).
- **Automatic Rollback:** Arm `IWDG` before booting a newly updated bank in *trial* state; if the new firmware fails self-test and reboots before setting a `CONFIRMED` flag, the bootloader detects the unconfirmed trial counter in `TAMP->BKPxR` or Flash trailer and rolls back to the known-good bank!
- **Persistent Storage Across Reset:**
  - **`.noinit (NOLOAD)` SRAM Section:** Preserves crash logs/boot counters across warm/watchdog resets (lost on power loss).
  - **`TAMP->BKP0R..BKP31R` (`0x40002500..0x4000257C`, 128 bytes):** Retained across Reset, Standby, Shutdown, and VBAT (requires `PWR->CR1.DBP = 1` to write).

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **RWW (Read-While-Write)** | Dual-bank Flash capability allowing CPU code execution from Bank 1 while erasing/programming Bank 2 without stalls. |
| **SECDED ECC (`FLASH->ECCR`)** | Single-Error Correction, Double-Error Detection (8 ECC bits per 64-bit Flash double-word). |
| **`SCB->VTOR`** | Vector Table Offset Register relocating Cortex-M4 exception vectors to a 512-byte aligned Application address. |
| **IAP / OTA** | In-Application Programming / Over-The-Air firmware update staged into inactive Flash bank. |
| **`.noinit (NOLOAD)`** | Linker section in SRAM intentionally excluded from `Reset_Handler` `.bss` zero-initialization. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `stm32g474xx.h`:

| Register (`FLASH @ 0x40022000`) | Offset | Key Bitfields |
|---|---|---|
| **`FLASH->ACR`** | `0x00` | `LATENCY[3:0]`, `PRFTEN` (b8), `ICEN` (b9), `DCEN` (b10), `ICRST` (b11), `DCRST` (b12), `EMPTY` (b16) |
| **`FLASH->KEYR` / `OPTKEYR`** | `0x08` / `0x0C` | Flash unlock: `0x45670123`, `0xCDEF89AB`; Option unlock: `0x08192A3B`, `0x4C5D6E7F` |
| **`FLASH->SR`** | `0x10` | `EOP` (b0), `OPERR`, `PROGERR`, `WRPERR`, `PGAERR`, `SIZERR`, `PGSERR`, `MISERR`, `FASTERR`, `RDERR`, `OPTVERR`, `BSY` (b16) — errors are `rc_w1`! |
| **`FLASH->CR`** | `0x14` | `PG` (b0), `PER` (b1), `MER1` (b2), `PNB[9:3]` (pages `0..127`), `BKER` (b11: `0`=Bank1, `1`=Bank2), `MER2` (b15), `STRT` (b16), `OPTSTRT` (b17), `OBL_LAUNCH` (b27), `LOCK` (b31) |
| **`FLASH->ECCR`** | `0x18` | `ADDR_ECC[18:0]`, `BK_ECC` (b21), `SYSF_ECC` (b22), `ECCIE` (b24), `ECCC` (b30), `ECCD` (b31) |
| **`FLASH->OPTR`** | `0x20` | `RDP[7:0]`, `BOR_LEV[10:8]`, **`BFB2` (b20)**, **`DBANK` (b22)**, `nBOOT1` (b23), `SRAM_PE` (b24), `nSWBOOT0` (b26), `nBOOT0` (b27) |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 `SCB->VTOR` Alignment Requirement on STM32G474RE
STM32G474RE defines $16\text{ core} + 102\text{ external} = 118\text{ vectors}$:
$$\text{Table Size} = 118 \times 4\text{ bytes} = 472\text{ bytes} \implies \text{Alignment} = 2^{\lceil \log_2(472) \rceil} = 2^9 = \mathbf{512\text{ bytes}\ (\texttt{0x200})}$$
Placing an Application at `0x08008000` (32 KB bootloader reserved) satisfies both 512-byte `VTOR` alignment and 2 KB/4 KB Flash page alignment!

### 6.2 Wear-Leveling Flash Endurance Calculation (EEPROM Emulation)
With page endurance $E = 10{,}000$ cycles, 2 KB page size ($2048\text{ B}$), and 16-byte log records (two 64-bit double-words per write) rotating across $P = 4$ pages:
$$\text{Total Writes} = P \times \frac{2048}{16} \times 10{,}000 = 4 \times 128 \times 10{,}000 = \mathbf{5.12\text{ million writes}}$$

---

## 7. Official Documentation & Standards References

| Document | Exact Sections |
|---|---|
| **ST RM0440** | Ch. 2 (*Memory Map & SRAM Parity*), Ch. 3 (*Embedded Flash Memory, Dual-Bank, ECC, Option Bytes*), Ch. 10 (*SYSCFG `SWPR`/`SCSR`*) |
| **ST AN2606 / AN4894** | AN2606 (*STM32 System Memory Boot Mode — STM32G4*), AN4894 (*EEPROM Emulation Techniques*) |
| **ST PM0214 / ES0430** | PM0214 §4.4.4 (`SCB->VTOR`); ES0430 §2.2.2–2.2.3 (*Flash DCache RWW & ECCR*) |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h`, `core_cm4.h`, RM0440, and ES0430 §2.2.2 (Not Tested on Physical Hardware).

### 8.1 STM32G474RE Bank 2 Page Erase & 64-Bit Double-Word Program (with Cache Flush)
```c
#include "stm32g474xx.h"
#include <stdint.h>
#include <stdbool.h>

#define FLASH_KEY1 0x45670123UL
#define FLASH_KEY2 0xCDEF89ABUL

static void flash_flush_caches(void)
{
    /* ES0430 §2.2.2: Flush DCache/ICache after programming to avoid stale readback */
    uint32_t acr = FLASH->ACR;
    FLASH->ACR &= ~(FLASH_ACR_DCEN | FLASH_ACR_ICEN);
    FLASH->ACR |=  (FLASH_ACR_DCRST | FLASH_ACR_ICRST);
    FLASH->ACR &= ~(FLASH_ACR_DCRST | FLASH_ACR_ICRST);
    FLASH->ACR  = acr;
}

bool Flash_EraseBank2Page(uint8_t page_0_to_127)
{
    if ((page_0_to_127 > 127U) || ((RCC->CR & RCC_CR_HSIRDY) == 0U)) { return false; }
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}

    /* Clear all sticky error flags (rc_w1) */
    FLASH->SR = FLASH_SR_OPERR | FLASH_SR_PROGERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                FLASH_SR_SIZERR | FLASH_SR_PGSERR | FLASH_SR_MISERR | FLASH_SR_FASTERR;

    if ((FLASH->CR & FLASH_CR_LOCK) != 0U) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }

    /* Bank 2 (BKER = 1), 2 KB page erase (DBANK = 1) */
    FLASH->CR = (FLASH->CR & ~(FLASH_CR_PNB_Msk | FLASH_CR_PG))
              | FLASH_CR_PER
              | FLASH_CR_BKER
              | ((uint32_t)page_0_to_127 << FLASH_CR_PNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;

    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    FLASH->CR &= ~FLASH_CR_PER;
    FLASH->CR |=  FLASH_CR_LOCK;
    flash_flush_caches();
    return (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_WRPERR | FLASH_SR_PGSERR)) == 0U;
}

bool Flash_ProgramDoubleWord(uint32_t addr_8b_aligned, uint64_t data)
{
    if ((addr_8b_aligned & 0x7UL) != 0U) { return false; } /* Mandatory 64-bit alignment! */
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}

    if ((FLASH->CR & FLASH_CR_LOCK) != 0U) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }

    FLASH->CR |= FLASH_CR_PG;
    *(__IO uint32_t *)(addr_8b_aligned)      = (uint32_t)(data & 0xFFFFFFFFULL);
    *(__IO uint32_t *)(addr_8b_aligned + 4U) = (uint32_t)(data >> 32U);
    __DSB();

    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    FLASH->CR &= ~FLASH_CR_PG;
    FLASH->CR |=  FLASH_CR_LOCK;
    flash_flush_caches();
    return (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_PGAERR | FLASH_SR_SIZERR | FLASH_SR_PGSERR)) == 0U;
}
```

### 8.2 Safe 9-Step ARMv7-M Bootloader-to-Application Jump
```c
#define APP_BASE_ADDR   0x08008000UL
#define APP_END_ADDR    0x0807FFFFUL
#define SRAM_START_ADDR 0x20000000UL
#define SRAM_END_ADDR   0x20020000UL /* 80K SRAM1 + 16K SRAM2 + 32K CCM alias */

void Bootloader_JumpToApp(uint32_t app_base)
{
    uint32_t app_msp   = *(__IO uint32_t *)(app_base);
    uint32_t app_reset = *(__IO uint32_t *)(app_base + 4U);

    /* 1. Validate alignment, SRAM MSP bounds, and Thumb-bit Flash Reset_Handler */
    if ((app_base & 0x1FFUL) != 0U) { return; } /* Must be 512-byte aligned for VTOR */
    if ((app_msp < SRAM_START_ADDR) || (app_msp > SRAM_END_ADDR) || ((app_msp & 0x7UL) != 0U)) { return; }
    if ((app_reset < app_base) || (app_reset > APP_END_ADDR) || ((app_reset & 1UL) == 0U)) { return; }

    /* 2. Disable all interrupts */
    __disable_irq();

    /* 3. Halt SysTick */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    /* 4. Disable & clear all NVIC IRQs */
    for (uint32_t i = 0U; i < 4U; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    /* 5. Ensure Thread mode uses MSP & Privileged state */
    __set_CONTROL(0U);

    /* 6. Relocate Vector Table & synchronize pipeline */
    SCB->VTOR = app_base;
    __DSB();
    __ISB();

    /* 7. Switch MSP and branch directly into Application Reset_Handler */
    __set_MSP(app_msp);
    ((void (*)(void))app_reset)();
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **GDB Verification Checkpoint (Dual-Bank RWW & Bootloader):**
  ```text
  (gdb) x/1wx 0x40022020     # Inspect FLASH->OPTR (verify bit 22 DBANK == 1)
  (gdb) x/2wx 0x0807F800     # Verify 64-bit programmed sentinel in Bank 2 Page 127
  (gdb) x/1wx 0x40022018     # Confirm FLASH->ECCR ECCC (b30) & ECCD (b31) == 0
  (gdb) x/1wx 0xE000ED08     # Verify SCB->VTOR == 0x08008000 inside Application
  ```

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Attempting 32-Bit Flash Writes:** Triggers `SIZERR`/`PGSERR` immediately — STM32G4 **requires 64-bit double-word writes** due to 72-bit ECC.
2. **Turning Off `HSI16` Before Erasing/Programming Flash:** Silently hangs `FLASH->SR.BSY`!
3. **Forgetting Cache Flush (`DCRST`/`ICRST`) After Flash Write (`ES0430 §2.2.2`):** CPU reads stale `0xFFFFFFFF` from ART DCache instead of freshly programmed words.
4. **Jumping to Application Without Clearing `SysTick`/`NVIC->ICER`/`CONTROL`:** First SysTick firing inside the App vectors through stale state and immediately HardFaults.
5. **Writing `CCMRAM` in Linker Script Without `_siccmram` Copy Loop:** Functions placed in `.ccmram` execute uninitialized zeros!

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 2]** Erase Bank 2 Page 127 (`0x0807F800`) while running from Bank 1, write `0xDEADBEEFCAFEBABEULL`, verify via GDB and confirm `FLASH->SR == 0` & `FLASH->ECCR == 0`.
2. **[Level 2]** Lock CCM-SRAM Page 0 (`0x10000000..0x100003FF`) via `SYSCFG->SWPR |= 1U;` and verify an intentional write triggers a BusFault/HardFault until reset.
3. **[Level 2]** Build a 2-image Bootloader (`0x08000000`) + Application (`0x08008000`) using dual linker scripts and verify clean handoff even when the bootloader used `SysTick` and `USART2`.
4. **[Level 3]** Add a fixed `.fw_header` struct (`magic`, `version`, `length`, `crc32`) checked via Hardware `CRC` before jumping; corrupt 1 byte and verify the bootloader refuses execution.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** What is the Flash page size on STM32G474RE when `DBANK = 1` vs `DBANK = 0`?  
   *Answer Check:* **2 KB** per page in Dual-Bank mode (`DBANK = 1`, 2×256 KB); **4 KB** per page in Single-Bank mode (`DBANK = 0`, 1×512 KB).
2. **Q:** Why can an application NOT be placed at `0x08008100` on STM32G474RE?  
   *Answer Check:* `SCB->VTOR` requires **512-byte (`0x200`) alignment** for 118 vectors (and Flash pages require 2 KB / `0x800` alignment).
3. **Q:** Why does `Bootloader_JumpToApp()` store `app_msp` and `app_reset` in local variables *before* calling `__set_MSP(app_msp)`?  
   *Answer Check:* Once `MSP` switches to the Application's stack pointer, any stack-allocated local variable belonging to the bootloader is invalid!

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `BOOT-FLASH-001` | Erase Bank 2 page during Bank 1 RWW, write 256 B as 32 aligned 64-bit double-words, flush caches, verify `FLASH->SR == 0` & `ECCR == 0` | GDB (`x/64wx`) |
| `BOOT-JUMP-001` | Bootloader using `SysTick` + `USART2` executes full 9-step teardown, validates MSP/Reset PC, sets `VTOR = 0x08008000`, and boots App cleanly | GDB + UART log |
| `BOOT-VERIF-001` | Corrupt 1 byte in Bank 2 image $\to$ bootloader rejects via Hardware `CRC`; unconfirmed trial boot auto-rolls back via IWDG | Hardware Test / GDB |
| `STM32-SRAM-001` | Verify `SYSCFG->SWPR` write-protect fault and `.noinit` / `TAMP->BKP0R` persistence across `NVIC_SystemReset()` | GDB |
