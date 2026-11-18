# Curriculum Mastery Assessments — Worked Answer Keys & Grading Rubrics

This document provides **complete, verified solutions** and **objective point-by-point pass/fail rubrics** for all five examinations in `24-assessments/` (500 points total; **pass threshold = $\ge 85 / 100$ per exam**).

**Concept ID:** `RES-ASSESS-001`  
**Verification Status:** All register addresses, offsets, and bit positions verified directly against `stm32g474xx.h`, `core_cm4.h`, RM0440, DS12288, PM0214, and UM2505. Code snippets Statically Reviewed (Not Tested on Physical Hardware).

---

## Stage 01 Answer Key & Rubric

| Q# | Max Pts | Full-Credit Verification Criteria | Partial / Fail Deductions |
|---:|---:|---|---|
| Q1 | 10 | Correct binary `1101 0110` (`0xD6`) with 2's-comp steps (6 pts) + `[-128, +127]` range (4 pts) | −4 pts if missing `+1` step; −2 pts if wrong range |
| Q2 | 10 | `0x20000010` (6 pts) + `16` bytes scaling by `sizeof(uint32_t)` (4 pts) | 0 pts if answering `0x20000004` |
| Q3 | 10 | Original `12` bytes with byte offsets `0, 4..7, 8..9` + 5 pad bytes (6 pts) + reordered `8` bytes without `packed` (4 pts) | −4 pts if forgetting 2-byte trailing struct alignment pad |
| Q4 | 10 | `reg = (reg & ~(3UL << 10U)) | (1UL << 10U);` (6 pts) + signed left-shift into sign bit = UB (4 pts) | −3 pts if missing `U`/`UL` suffix |
| Q5 | 10 | Identifies `49U - 50U = 0xFFFFFFFF` wrap / `INT30-C` infinite loop (5 pts) + safe rewrite (5 pts) | 0 pts if missing unsigned underflow |
| Q6 | 10 | Local `static` persistence vs file-scope internal linkage (5 pts) + exact `typedef` & `const` array (5 pts) | −2 pts for invalid function-pointer syntax |
| Q7 | 25 | Exact casts & masks in `pack_telemetry()` (15 pts) + 3 parenthesized unsigned unpack macros/functions (10 pts) | −5 pts for missing `uint32_t` cast before `<< 24U` |
| Q8 | 15 | $R = 200\ \Omega$ (std $220\ \Omega$) (7 pts), verifies $\le 20\ \text{mA}$ DS12288 limit (4 pts), $P = 7.2\ \text{mW}$ (4 pts) | −4 pts if omitting $V_f$ subtraction |

### Stage 01 Worked Solutions
1. **`-42` (8-bit 2's complement):** $+42 = \texttt{0010\_1010}_2 \xrightarrow{\text{invert}} \texttt{1101\_0101}_2 \xrightarrow{+1} \mathbf{\texttt{1101\_0110}_2 = \texttt{0xD6}}$. Check: $-128+64+16+4+2 = -42$. Range of `int8_t`: **$-128$ to $+127$**.
2. **Pointer arithmetic:** `ptr + 4` advances by $4 \times \texttt{sizeof(uint32\_t)} = 16\ (\texttt{0x10})$ bytes → **`0x20000010`**. Byte difference = **`16`**.
3. **Struct alignment:**
   - Original: `id` `@0` (1 B) → pad `@1..3` (3 B) → `timestamp` `@4..7` (4 B) → `raw_value` `@8..9` (2 B) → tail pad `@10..11` (2 B for 4-byte struct alignment) → **`sizeof = 12` bytes**.
   - Reordered (largest-to-smallest): `uint32_t timestamp` (`@0..3`), `uint16_t raw_value` (`@4..5`), `uint8_t id` (`@6`), 1 B tail pad (`@7`) → **`sizeof = 8` bytes**.
4. **Bitmask:** `reg = (reg & ~(3UL << 10U)) | (1UL << 10U);`. `(1 << 31)` shifts a 32-bit signed `int` into/past its sign bit (**ISO C Undefined Behavior**); `(1UL << 31U)` operates on unsigned `uint32_t`/`unsigned long`.
5. **Unsigned Underflow (`INT30-C`):** `6799 % 50 == 49`. When `duty == 49U`, `duty -= 50U` wraps to **`0xFFFFFFFF`**, looping forever. Safe rewrite:
   ```c
   for (uint32_t duty = 6799U; ; duty -= 50U) {
       set_pwm(duty);
       if (duty < 50U) { break; }
   }
   ```
6. **Storage classes & Function pointer:**
   - Function-local `static`: persists in `.data`/`.bss` across calls, block visibility. File-scope `static`: internal linkage (private to translation unit).
   - `typedef int32_t (*event_handler_t)(uint32_t event_id, void *ctx);`
   - `static const event_handler_t handlers[4] = { h0, h1, h2, h3 };`
7. **Bit-Packing:**
   ```c
   uint32_t pack_telemetry(uint8_t status, uint16_t voltage_mv, uint8_t checksum) {
       return ((uint32_t)status << 24U) | ((uint32_t)voltage_mv << 8U) | (uint32_t)checksum;
   }
   #define UNPACK_STATUS(w)   ((uint8_t)(((uint32_t)(w) >> 24U) & 0xFFU))
   #define UNPACK_VOLTAGE(w)  ((uint16_t)(((uint32_t)(w) >> 8U) & 0xFFFFU))
   #define UNPACK_CHECKSUM(w) ((uint8_t)((uint32_t)(w) & 0xFFU))
   ```
8. **LED Resistor Math:** $R = \frac{V_{\text{DD}} - V_f}{I_f} = \frac{3.3 - 2.1}{0.006} = \mathbf{200\ \Omega}$ (nearest standard 5%: $220\ \Omega \rightarrow 5.45\ \text{mA}$, well within DS12288 Table 15 $\pm 20\ \text{mA}$ limit). Power: $P = I_f^2 R = (0.006)^2 \times 200 = \mathbf{7.2\ \text{mW}}$.

---

## Stage 02 Answer Key & Rubric

| Q# | Max Pts | Full-Credit Verification Criteria |
|---:|---:|---|
| Q1 | 20 | Explains cached-register infinite loop vs repeated `LDR` on `volatile` (12 pts) + explains `volatile` is NOT atomic and does not emit `DMB`/`DSB` (8 pts) |
| Q2 | 15 | Shows `LDR`/`ORR`/`STR` (7 pts) + exact interleaving where ISR sets bit 6 and main's stale `STR` overwrites bit 6 back to 0 (8 pts) |
| Q3 | 15 | Explains single `STR` to `BSRR`/`BRR` with 0-bits ignored (8 pts) + explains `0x48000000` is outside `0x40000000–0x400FFFFF` 1 MB bit-band window (7 pts) |
| Q4 | 20 | 5 pts per exact fix (`= EXTI_PR1_PIF13`, `= ~TIM_SR_UIF`, `= TIM_EGR_UG`, `= ADC_ISR_ADRDY`) with hardware hazard explanation |
| Q5 | 15 | Exact `#define` chain (`PERIPH_BASE`, `AHB2PERIPH_BASE`, `GPIOA_BASE`, `GPIOC_BASE`, `(GPIO_TypeDef *)`) (10 pts) + `__IO` rationale (5 pts) |
| Q6 | 15 | Exact save/restore `PRIMASK` functions with `__DMB()` (10 pts) + nested-exit premature-enable bug explanation (5 pts) |

### Stage 02 Worked Solutions
1. **Volatile:** Without `volatile`, `-O2` hoists `*status_reg` into a register (`LDR` once) and spins forever if initially 0. With `volatile`, every iteration emits `LDR` + `TST` + `BEQ`. However, `volatile` still permits multi-instruction RMW tearing and does not drain hardware write buffers (requires `BSRR`/`LDREX` + `DMB`/`DSB`).
2. **RMW Race:** `main()` runs `LDR R0, [ODR]` (reads `0x00`) → `ORR R0, #(1<<5)` (`0x20`) → *ISR preempts and sets bit 6 (`ODR = 0x40`)* → `main()` executes `STR R0, [ODR]` writing `0x20`, **silently erasing bit 6!**
3. **`BSRR`/`BRR` & Bit-Banding:** (a) Single `STR` latches set/reset flip-flops directly; hardware gates only `1` bits so `0` bits leave other pins untouched. (b) Cortex-M4 peripheral bit-banding covers only `0x40000000–0x400FFFFF` (1 MB); STM32G4 `GPIOA–G` reside on AHB2 at `0x48000000+` (outside the window).
4. **Flag Clearing Fixes:**
   - (a) `EXTI->PR1 = EXTI_PR1_PIF13;` (`|=` reads all other pending flags and writes `1` back to them, accidentally clearing unrelated lines!).
   - (b) `TIM2->SR = ~TIM_SR_UIF;` (`&=` RMW can clear another flag that asserted between read and write).
   - (c) `TIM2->EGR = TIM_EGR_UG;` (`EGR` is write-only; reading it is meaningless).
   - (d) `ADC1->ISR = ADC_ISR_ADRDY;` (`rc_w1` — same multi-flag clear hazard as `EXTI->PR1`).
5. **CMSIS Chain:**
   ```c
   #define PERIPH_BASE       0x40000000UL
   #define AHB2PERIPH_BASE   (PERIPH_BASE + 0x08000000UL)
   #define GPIOA_BASE        (AHB2PERIPH_BASE + 0x0000UL)
   #define GPIOC_BASE        (AHB2PERIPH_BASE + 0x0800UL)
   #define GPIOA             ((GPIO_TypeDef *)GPIOA_BASE)
   #define GPIOC             ((GPIO_TypeDef *)GPIOC_BASE)
   ```
6. **Reentrant Critical Section:**
   ```c
   static inline uint32_t crit_enter(void) {
       uint32_t primask = __get_PRIMASK();
       __disable_irq();
       __DMB();
       return primask;
   }
   static inline void crit_exit(uint32_t primask) {
       __DMB();
       __set_PRIMASK(primask);
   }
   ```
   Blindly calling `__enable_irq()` inside an inner helper would prematurely unmask interrupts while an outer critical section is still active.

---

## Stage 03 Answer Key & Rubric

| Q# | Max Pts | Full-Credit Verification Criteria |
|---:|---:|---|
| Q1 | 15 | ARMv7E-M DSP/SIMD (4 pts), `PC[0]==0` & `PC+4` & `EPSR.T` bit 24 (6 pts), `MSP` reset/Handler vs `PSP` Thread via `CONTROL.SPSEL` (5 pts) |
| Q2 | 15 | Exact 8-word order `[R0,R1,R2,R3,R12,LR,PC,xPSR]` (6 pts), 26-word / 104 B FPU frame + lazy `LSPEN` + `0xFFFFFFFD` vs `0xFFFFFFED` (9 pts) |
| Q3 | 20 | `SCB->SHCSR` bits 16–18 (5 pts), exact 4-line naked-asm `TST LR, #4` (7 pts), `0x00008200` (`PRECISERR`+`BFARVALID` @ `0x60000000`) & `0x02000000` (`DIVBYZERO`) (8 pts) |
| Q4 | 15 | Valid `MPU->RNR/RBAR/RASR` (`SIZE=4`, `AP=0`, `XN=1`, `PRIVDEFENA=1`) (11 pts) + `__DSB(); __ISB();` (4 pts) |
| Q5 | 20 | VMA/LMA + `CCMRAM (xrw)` + `_siccmram` copy loop (8 pts), 512-byte (`0x200`) `VTOR` alignment (6 pts), `0x10000000` CPU vs `0x20018000` DMA (6 pts) |
| Q6 | 15 | `LDREX`/`STREX` monitor retry loop (5 pts), `DEMCR.TRCENA` + `DWT->CTRL.CYCCNTENA` + 5.882 ns / 25.26 s wrap (5 pts), `DAC1–4` on **AHB2** & `GPIOE`/`SPI4`/`FMC` unbonded + no `AES` (5 pts) |

### Stage 03 Worked Solutions
1. **(a)** ARMv7E-M adds single-cycle MAC, DSP/SIMD (`SMLAD`, `QADD16`), and saturating math (`SSAT`/`USAT`) over Cortex-M3's ARMv7-M. **(b)** Reading `PC` always yields `bit[0] = 0` at `current_instruction + 4` (3-stage pipeline); Thumb state resides in `EPSR.T` (bit 24). **(c)** `MSP` is active at reset and always in Handler mode; `PSP` is selected in Thread mode via `CONTROL.SPSEL = 1` for RTOS tasks.
2. **(a)** `SP+0: R0, +4: R1, +8: R2, +12: R3, +16: R12, +20: LR, +24: ReturnAddr(PC), +28: xPSR` (32 bytes). **(b)** FPU active expands frame to **26 words (104 bytes)** adding `S0–S15`, `FPSCR`, and align pad. With `FPCCR.LSPEN = 1`, hardware reserves the 72 extra bytes without pushing `S0–S15` until the ISR executes an FPU instruction. `0xFFFFFFFD` = return to Thread/PSP basic frame; `0xFFFFFFED` = return to Thread/PSP extended FPU frame (`bit[4] = 0`).
3. **(a)** `SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk;` (bits 16–18).
   **(b)** Naked prologue:
   ```c
   __attribute__((naked)) void HardFault_Handler(void) {
       __asm volatile (
           "tst lr, #4       \n"
           "ite eq           \n"
           "mrseq r0, msp    \n"
           "mrsne r0, psp    \n"
           "b HardFault_Decode_C \n"
       );
   }
   ```
   **(c)** `CFSR = 0x00008200` → `BFSR = 0x82`: `BFARVALID` (bit 15) + `PRECISERR` (bit 9) → **Precise BusFault** accessing invalid address `0x60000000`. `CFSR = 0x02000000` → `UFSR = 0x0200` (`DIVBYZERO`, bit 25) → **Integer divide-by-zero UsageFault**.
4. **MPU 32-Byte Stack Guard:** `MPU->CTRL = 0; MPU->RNR = 0; MPU->RBAR = guard_addr & MPU_RBAR_ADDR_Msk; MPU->RASR = MPU_RASR_XN_Msk | (0U << MPU_RASR_AP_Pos) | (1U << MPU_RASR_TEX_Pos) | (4U << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk; MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk; __DSB(); __ISB();`.
5. **(a)** `LMA` = Flash storage address, `VMA` = SRAM runtime address. Without `(xrw)` linker rejects executable code in CCMRAM; without `_siccmram = LOADADDR(.ccmram)` and a `Reset_Handler` copy loop, CCMRAM holds uninitialized garbage! **(b)** Next power-of-2 $\ge 118 \times 4 = 472\ \text{B}$ is **512 bytes (`0x200`)**. **(c)** CPU executes zero-wait-state code from **`0x10000000`** (I/D-Bus); DMA1/DMA2 MUST target the S-Bus alias **`0x20018000`**.
6. **(a)** `LDREX` loads value and tags local exclusive monitor; `STREX` succeeds (returns `0`) only if no exception/clear occurred since `LDREX`, else retries. **(b)** `CoreDebug->DEMCR |= TRCENA` + `DWT->CTRL |= CYCCNTENA`; $1/170\ \text{MHz} = \mathbf{5.882\ \text{ns}}$, wrap = $2^{32}/170\times 10^6 = \mathbf{25.26\ \text{s}}$. **(c)** `DAC1–4` live on **AHB2** (`0x50000800`); LQFP64 lacks bonded `GPIOE`, `SPI4`, `FMC`, and G474 silicon lacks `AES/CRYP`.

---

## Stage 04 Answer Key & Rubric

| Q# | Max Pts | Full-Credit Verification Criteria |
|---:|---:|---|
| Q1 | 15 | Valid `M=6, N=85, R=2, P=8` (4/340/170/42.5 MHz) (7 pts) + exact 5 steps (`PWR_CR5 @ 0x80`, `PWR_SR2.VOSF`, 4 WS, `HPRE=/2` + ≥1 µs, `HPRE=/1`) (8 pts) |
| Q2 | 10 | `11` Analog reset default for leakage prevention (5 pts) + Active-HIGH `B1`, `PUPDR=00`, `RTSR1`, direct `PR1 = (1U<<13)` (5 pts) |
| Q3 | 15 | `ARR = 5312` (or `5311`, 5313/5312 steps) (6 pts), `DTG = 85` (`0x55`) (5 pts), `MOE=0` silences output + `COMP1`→`BKIN` <50 ns trip (4 pts) |
| Q4 | 10 | `BRR = 184` (`+0.25%` error) (6 pts) + `LPUART1` 256× formula (4 pts) |
| Q5 | 15 | `FRXTH=1` (8-bit RXNE threshold) + 8-bit `DR` packing trap (7 pts) + `AUTOEND=0→TC→AUTOEND=1` + `PB8-BOOT0` 10 kΩ R36 divider (~0.66 V) (8 pts) |
| Q6 | 10 | Valid 500 kbps `NBTP` (`NBRP=3, NTSEG1=8, NTSEG2=1` for 83.33% or `NBRP=2, NTSEG1=11, NTSEG2=2` for 81.25%) (6 pts) + `0x4000A400` / 848 B (`0x350`) (4 pts) |
| Q7 | 15 | `CKMODE=11` (`42.5 MHz <= 60 MHz`) (5 pts), $t_S \ge 534\ \text{ns} \rightarrow 24.5$ cycles (`011b`) (6 pts), `DACx->MCR.HFSEL = 10b` (4 pts) |
| Q8 | 10 | `DMAMUX1EN` + ID `5` (`ADC1`) + circular 16-bit config (5 pts), ES0430 clear individual `CTCIF/CHTIF/CTEIF` (never `CGIFx`) + `USART2_RX=26`, `TX=27` (5 pts) |

### Stage 04 Worked Solutions
1. **170 MHz HSE X3:** `M = 6` (`PLLM=5` → $4\ \text{MHz}$), `N = 85` ($f_{\text{VCO}} = 340\ \text{MHz} \in [96, 344]$), `R = 2` (`00b` → $170\ \text{MHz}$), `P = 8` (`PLLPDIV=8` → $42.5\ \text{MHz} \le 60\ \text{MHz}$). Mandatory steps: (1) Enable `PWREN`, set `VOS=01`, poll `PWR->SR2.VOSF == 0`; (2) Clear `PWR->CR5.R1MODE = 0` (offset `0x80`); (3) Set `FLASH->ACR` 4 WS + ART & read back; (4) Set `RCC->CFGR.HPRE = /2` (85 MHz), switch `SW = 11` (PLL), wait `SWS == 11`; (5) Wait $\ge 1\ \mu\text{s}$, then set `HPRE = /1` (170 MHz).
2. **(a)** Reset default is **`11` (Analog)** to disable digital Schmitt triggers and eliminate floating-pin leakage. **(b)** `B1` (`PC13`) is **Active-HIGH** (external pull-down on MB1367): `PUPDR = 00` (no pull), enable `EXTI->RTSR1 |= (1U << 13U)`, clear with `EXTI->PR1 = (1U << 13U);`.
3. **(a)** $170{,}000{,}000 / 32{,}000 = 5312.5 \rightarrow \mathbf{\texttt{ARR} = 5312}$ ($31.997\ \text{kHz}$, **5313 steps**) or `5311` ($32.003\ \text{kHz}$, **5312 steps**). **(b)** $500\ \text{ns} / 5.882\ \text{ns} = \mathbf{85\ (\texttt{0x55})}$ in `BDTR.DTG[7:0]`. If `MOE = 0`, outputs stay inactive. `COMP1` wired to `TIM1->AF1.BKCMP1E` clears `MOE = 0` in hardware (< 50 ns) on overcurrent.
4. **(a)** $\text{BRR} = \text{round}(170{,}000{,}000 / 921{,}600) = \text{round}(184.46) = \mathbf{184\ (\texttt{0x00B8})}$; actual $= 923{,}913\ \text{bps}$, error $= \mathbf{+0.25\%}$. **(b)** `LPUART1` uses a 20-bit fractional formula: $\text{BRR}_{\text{LPUART}} = \frac{256 \times f_{\text{CK}}}{\text{Baud}}$.
5. **(a)** With `FRXTH = 0`, `RXNE` triggers only at 16 bits (1/2 of 32-bit FIFO), so a single 8-bit byte never sets `RXNE`. Writing 16 bits to `SPI1->DR` packs 2 bytes into the FIFO; `*(__IO uint8_t *)&SPI1->DR` pushes exactly 1 byte. **(b)** Phase 1 writes reg addr with `AUTOEND=0` → waits `ISR.TC` (SCL held low) → Phase 2 writes `RD_WRN=1, NBYTES=len, AUTOEND=1, START=1`. On NUCLEO-G474RE, `PB8-BOOT0` has external 10 kΩ pull-down `R36`; an internal ~40 kΩ pull-up yields $3.3 \times \frac{10}{50} \approx 0.66\ \text{V}$ (never reaches logic HIGH!).
6. **FDCAN1:** For exact **83.33%**: `NBRP = 3` (`/4` → 6 MHz TQ), 12 TQ/bit → `1 + 9 + 2 = 12` → **`NTSEG1 = 8`, `NTSEG2 = 1`**, sample point $= 10/12 = 83.33\%$ (also accept `NBRP=2, NTSEG1=11, NTSEG2=2` → 16 TQ, 81.25%). `FDCAN1` Message RAM starts at **`0x4000A400`**, **848 bytes (`0x350`, 212 words)** per instance.
7. **(a)** `CKMODE=01` runs ADC at 170 MHz (2.83× over the 60 MHz spec!). Use **`CKMODE = 11` (`HCLK/4 = 42.5 MHz`)**. **(b)** $t_S \ge (10\text{k} + 1\text{k})\,\Omega \times 5\ \text{pF} \times 9.704 \approx \mathbf{534\ \text{ns}}$; at 42.5 MHz ($23.53\ \text{ns/clk}$), $534 / 23.53 = 22.7\ \text{cycles} \rightarrow$ select **`SMPRx = 011b` (24.5 cycles = 576 ns)** (or `100b` = 47.5 cycles). **(c)** **`DAC1->MCR.HFSEL = 10b`** (bits `[15:14]`, mandatory > 160 MHz).
8. **DMA1 + DMAMUX1:** Enable `RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN`; set `DMAMUX1_Channel0->CCR = 5U;` (`ADC1` = ID **5**); configure `CPAR = (uint32_t)&ADC1->DR`, `CMAR`, `CNDTR`, `CCR = CIRC | MINC | PSIZE_16 | MSIZE_16 | HTIE | TCIE | EN`. **ES0430 §2.3.1:** Clear `DMA1->IFCR = DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;` individually — **NEVER `CGIF1`**! `USART2_RX = 26 (0x1A)`, `USART2_TX = 27 (0x1B)`.

---

## Stage 05 Answer Key & Rubric

| Q# | Max Pts | Full-Credit Verification Criteria |
|---:|---:|---|
| Q1 | 20 | `SVC` first launch, `SysTick` tick, `PendSV` deferred lowest-prio switch + `TST LR, #0x10` S16–S31 (10 pts) + `PRIGROUP=0`, prios `5..15` kernel-safe vs `0..4` zero-latency, `BASEPRI=0` disables masking (10 pts) |
| Q2 | 15 | Distinctly contrasts L/M/H inversion (unbounded blocking) vs AB/BA circular-wait deadlock (8 pts) + `FromISR`, `portYIELD_FROM_ISR`, Task Notifications zero-object RAM / ~45% faster (7 pts) |
| Q3 | 15 | `PR = 100b = 4`, `RLR = 499`, `DBGMCU_APB1FZR1_DBG_IWDG_STOP` (6 pts), WWDG `CNT>W` & `CNT<0x40` + `EWI` crash log (4 pts), Sleep/LPRun/LPSleep/Stop0/Stop1/Standby/Shutdown (**NO Stop 2**), **`JP6`**, 3 wakeup steps (5 pts) |
| Q4 | 20 | Complete safe jump sequence (`__disable_irq`, SysTick=0, ICER/ICPR, MSP bounds, `VTOR` 512 B aligned, `CONTROL=0`, `DSB`/`ISB`) (10 pts) + `RDP` 0/1/2 (irreversible warning), `WRP`, `PCROP`, `SEC1R`, `TAMP`, **NO hardware AES on G474RE** (10 pts) |
| Q5 | 15 | 4 violations (bare `int`, missing `volatile`, Rule 11.4 cast, signed shift `1<<5`, RMW on `ODR`) + compliant `BSRR` rewrite (10 pts) + Deviation Permit fields (5 pts) |
| Q6 | 15 | Hardcoded physical MMIO segfaults on x86/x64 (5 pts) + concrete pointer-injection fake struct & `const i2c_ops_t *` vtable with Unity/CMock + ASan/UBSan (10 pts) |

### Stage 05 Worked Solutions
1. **(a)** `SVC 0` launches the first task (`vPortStartFirstTask`); `SysTick` increments ticks and sets `SCB->ICSR.PENDSVSET`; `PendSV` executes at lowest priority (15) so it never preempts active hardware ISRs. `PendSV_Handler` tests `EXC_RETURN` bit 4 (`tst r14, #0x10`); if `0` (FPU active), it executes `vstmdbeq`/`vldmiaeq` for `S16–S31`. **(b)** `NVIC_SetPriorityGrouping(0)` allocates all 4 G474RE bits to preemption (0 subpriority). With `configMAX_SYSCALL_INTERRUPT_PRIORITY = (5 << 4)`: numeric priorities **`5..15`** may safely call `FromISR` APIs; priorities **`0..4`** are never masked by `BASEPRI` (zero-latency motor/HRTIM ISRs, forbidden from calling RTOS APIs). `BASEPRI = 0` means **masking disabled**, so `configMAX_SYSCALL_INTERRUPT_PRIORITY` must NEVER be `0`.
2. **(a)** Priority inversion is unbounded blocking (M preempts L while H waits on L's resource) — NOT a deadlock. Priority Inheritance elevates L to H's priority, bounding blocking time, **but does NOT prevent AB/BA circular-wait deadlock** (requires strict global lock ordering). **(b)** ISRs have no TCB and cannot block. `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` pends `PendSV` so the unblocked task runs immediately on ISR exit instead of waiting for the next SysTick. Direct-to-Task Notifications embed a 32-bit value inside the TCB (no extra RAM object, ~45% lower latency).
3. **(a)** `IWDG->PR = 4U` (`100b` = `/64`), `IWDG->RLR = 499U` (`0x01F3`); freeze via `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;` (bit 12). **(b)** Refreshing WWDG while `CNT > W` resets immediately; letting `CNT` drop `0x40 → 0x3F` resets; `WWDG_IRQn (0)` (`EWI` at `0x40`) logs emergency state to `TAMP->BKPxR` before reset. **(c)** Actual STM32G474RE modes: **Sleep, Low-Power Run, Low-Power Sleep, Stop 0, Stop 1, Standby (optional SRAM2 `RRS`), Shutdown** (**Stop 2 does NOT exist!**). Measure $I_{DD}$ across **`JP6`** (NOT `JP5`!). Stop wakeup steps: (1) Clear `EXTI->PR1` in ISR, (2) Clear `SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk`, (3) Re-enable HSE/PLL to restore 170 MHz (Stop reverts SYSCLK to HSI16).
4. **(a)** Safe jump: (1) Validate App MSP $\in [\texttt{0x20000000}, \texttt{0x20020000}]$ and Reset PC bit 0 == 1 in App Flash; (2) `__disable_irq()`; (3) Stop `SysTick->CTRL = 0`, clear all `NVIC->ICER`/`ICPR`; (4) Reset used RCC peripherals/clocks; (5) Set `SCB->VTOR = 0x08008000` (512-byte aligned); (6) Reset `CONTROL = 0`, `__DSB(); __ISB();`, `__set_MSP(app_msp)`, branch to `app_reset()`. **(b)** `RDP`: L0 (`0xAA`) open, L1 debug/Flash/TAMP blocked (regression mass-erases), L2 (`0xCC`) **permanently irreversible bricking of SWD!** `WRP` locks Flash page ranges; `PCROP` is execute-only (`-mpure-code`); `SEC1R` hides RoT bootloader after `SEC_PROT1`; `TAMP` auto-zeroes `BKP0R..31R` + SRAM2 on tamper. **STM32G474RE has NO hardware AES/CRYP** (only STM32G483/G484).
5. **MISRA/CERT C Violations:** (1) Bare `int` instead of `uint32_t` (Barr/MISRA Dir 4.6); (2) Missing `volatile` qualifier; (3) Unjustified integer-to-pointer cast (Rule 11.4); (4) Signed literal shift `1 << 5` (Rule 10.1); (5) Non-atomic RMW on `ODR` (`0x48000014`) instead of atomic `BSRR` (`0x48000018`). Compliant rewrite: `GPIOA->BSRR = GPIO_BSRR_BS5;`. Deviation Permit for Rule 11.4 documents Deviation ID, Rule, Scope (CMSIS/MMIO header only), Technical Rationale, Risk Mitigation, and Reviewer Sign-Off.
6. **Host Unit Testing:** Dereferencing `0x40005400` causes `SIGSEGV` on x86/x64. Pattern A (Fake Struct Injection): `int i2c_write(I2C_TypeDef *i2c, ...)` accepts a host RAM `I2C_TypeDef fake_i2c = {0};` so Unity asserts exact register bitmasks written. Pattern B (Vtable Dependency Injection): `typedef struct { int (*write)(uint8_t addr, const uint8_t *buf, size_t n); } i2c_ops_t;` allows CMock/spy mocks to inject NACK/timeout error paths under `gcc -fsanitize=address,undefined`.
