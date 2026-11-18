# Stage 16: Production Firmware Engineering, Verification, Optimization, Security & Fault Tolerance

> **Sequential Navigation (Step 17 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [Stage 16 (14) — FreeRTOS `ARM_CM4F` (`16-rtos/`)](../16-rtos/README.md) | **Next:** [Capstone Projects (`19-projects/`)](../19-projects/README.md) & [Stage 05 Mastery Exam](../24-assessments/stage-05-rtos-and-advanced-firmware-exam.md)  
> **Prerequisites:** Stages [`01b`](../01b-digital-electronics-and-hw-integration/README.md)–[`16`](../16-rtos/README.md) (`C-*`, `EMB-*`, `ARM-*`, `TOOL-*`, `RTOS-*`)

> **Directory:** `17-advanced-firmware-engineering/` | **Target Platform:** STM32G474RET6 (NUCLEO-G474RE, MB1367) + Host x86/x64 Verification Pipeline  
> **Code Verification Notice:** All C/linker/CI snippets in this module are **Statically Reviewed against `stm32g474xx.h`, `core_cm4.h`, RM0440, MISRA C:2012/2023, and SEI CERT C (Not Tested on Physical Hardware)**.

---

## 1. Learning Objectives & Concept Scope

| Concept ID | Concept Name | Architectural Scope | Mastery Tier |
|---|---|---|---|
| `PROF-STD-001`..`004` | MISRA C:2012/2023, SEI CERT C, Barr Group Standard, & Formal Deviation Permits | `[General C]` | Level 2–3 (Production) |
| `PROF-SA-001`..`004` / `PROF-DA-001`..`004` | Static Analysis (`cppcheck`, `clang-tidy`, GCC `-fanalyzer`) & Dynamic Analysis (ASan/UBSan, `0xA5A5A5A5` Stack Painting, `FW_ASSERT`, Fault Injection) | `[General C]` / `[ARM Cortex-M4F]` | Level 2 (Production) |
| `PROF-REV-001` / `BOOT-ARCH-001` | Embedded Peer-Review Checklist & Layered Architecture (HAL/BSP/Drivers/App, Vtables, Active Objects) | `[General C]` | Level 2 (Production) |
| `TEST-UNIT-001`..`002` / `TEST-MOCK-001` / `TEST-HIL-001`..`002` | Host Unit Testing (Unity/CMock/Ceedling + Register Fakes) & HIL / Emulation (`pytest`/`pyserial`/OpenOCD/Renode) | `[General C]` / `[STM32G474RE Exact]` | Level 2–3 (Production) |
| `PROF-CI-001` / `PROF-GIT-001`..`002` / `PROF-DOC-001`..`002` | GitHub Actions CI/CD, Git Discipline, Embedded `.fw_info` Version Struct, & Doxygen / ADR / ICD Docs | `[General C]` | Level 2 (Production) |
| `PROF-STK-001`..`002` / `PROF-WCET-001`..`002` | Static Stack Analysis (`-fstack-usage`), Nested ISR+FPU Budgeting, & Cycle-Accurate WCET via **`DWT->CYCCNT`** | `[ARM Cortex-M4F]` | Level 2–3 (Production) |
| `PROF-MEM-001`..`002` / `PROF-PERF-001`..`003` | Footprint Triage (`size`/`nm`/`.map`), Struct Packing, `-Os`/`-flto`, CCM-SRAM, ART, & `CORDIC`/`FMAC` Optimization | `[STM32G474RE Exact]` | Level 2–3 (Production) |
| `SEC-RDP-001` / `SEC-MEM-001` / `SEC-BOOT-001`..`002` / `SEC-TAMP-001` / `SEC-CRYPTO-001` | **STM32G474RE Security:** `RDP` 0/1/2, `WRP`, `PCROP`, **`FLASH_SEC1R`/`SEC2R`**, **Anti-Rollback Monotonic Counters (`SEC-BOOT-002`)**, **`TAMP`** Zeroization, & **`RNG`** (**NO AES on G474; G484-Only!**) | `[STM32G474RE Exact]` | Level 3 (Specialist) |
| `PROF-REL-001`..`002` / `REL-CRASH-001` / `REL-DEGRADE-001` | FMEA, BOR, Flash ECC & SRAM Parity `NMI_Handler`, `SYSCFG->CFGR2` Lockup Break, Watchdog Supervisor, & Naked MSP/PSP `.noinit` Crash Logger | `[STM32G474RE Exact]` | Level 2–3 (Production) |

---

## 2. Prerequisites & Curriculum Positioning

### Prerequisite Concept IDs
- **Stages 01–15 (`02-c-fundamentals/` through `16-rtos/`):** Complete mastery of `C-*`, `EMB-*`, `ARM-*`, `TOOL-*`, `STM32-*`, `PERIPH-*`, `BOOT-*`, `REL-*`, and `RTOS-*`. This capstone engineering stage transforms working bare-metal/RTOS prototypes into **auditable, testable, secure, fault-tolerant production firmware**.

---

## 3. Core Technical Theory & Engineering Principles

### 3.1 Coding Standards & Formal Deviation Management (`PROF-STD-001..004`) `[General C]`
- **MISRA C:2012 / C:2023:** Categorizes requirements into **Directives** (design/process) and **Rules** (statically checkable), graded **Mandatory**, **Required**, or **Advisory**, anchored by the **Essential Type Model** (forbids implicit signed/unsigned/width conversions and non-boolean `if (ptr)` tests). MISRA C:2023 adds C11/C18 coverage (`_Generic`, atomics, concurrency).
- **SEI CERT C:** Security-focused rules targeting exploitable UB: `INT30-C` (unsigned wrap — e.g., Lab 07 underflow!), `INT32-C` (signed overflow), `ARR30-C` (bounds check), `STR31-C` (buffer termination), `EXP33-C` (uninitialized reads).
- **Barr Group Standard:** Fixed-width `<stdint.h>` types, explicit `U` suffixes on bitmasks, mandatory braces on all `if`/`for`/`while`, consistent naming.
- **Formal Deviation Permits (MISRA Compliance:2020):** Bare-metal drivers legitimately violate specific rules (e.g., **Rule 11.4** integer-to-pointer cast for `((GPIO_TypeDef *)0x48000000UL)`). Never silently disable a checker globally — issue a **Deviation Permit** documenting *(1) Rule ID, (2) Risk Assessment, (3) Scope (single header/macro), (4) Verification Method, and (5) Reviewer Sign-Off*, linked 1:1 to inline suppression comments!

### 3.2 Static Analysis, Dynamic Analysis & Peer-Review Checklist
1. **Static Analysis Layers (Zero Execution Needed):**
   - **Hardened GCC Flags:** `-Wall -Wextra -Werror -Wshadow -Wconversion -Wdouble-promotion -Wundef -Wstrict-prototypes -Wswitch-enum -fanalyzer` (`-fanalyzer` runs inter-procedural path analysis catching null dereferences, leaks, and uninitialized reads at compile time!).
   - **`cppcheck`:** `cppcheck --enable=all --std=c11 --platform=arm32-wchar_t4 --addon=misra.json --inline-suppr --error-exitcode=1 Src/`.
   - **`clang-tidy` & `clang-format`:** Enforce `bugprone-*`, `cert-*`, `readability-*` via `compile_commands.json` and zero-diff formatting (`clang-format --dry-run --Werror`).
   - **Commercial / Safety-Qualified Tools Awareness:** Coverity, PC-lint Plus, Polyspace (abstract interpretation sound prover), LDRA for ISO 26262 / IEC 61508 / DO-178C.
2. **Dynamic Analysis Layers:**
   - **Host Sanitizers (`gcc -fsanitize=address,undefined -fno-omit-frame-pointer`):** Catch out-of-bounds reads/writes, use-after-free, signed overflow, and invalid shifts in milliseconds during host unit tests!
   - **Target Stack Painting (`0xA5A5A5A5`):** Fill `[&_ebss, &_estack)` in `Reset_Handler` before `main()` and scan upward from `_ebss` at runtime to measure true peak MSP consumption!
   - **Design-by-Contract (`FW_ASSERT(cond)`):** Captures file/line + `LR` into `.noinit` SRAM, executes `__BKPT(0)` if debugger attached (`CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk`), else transitions to safe state & resets.
3. **8-Point Embedded Code Review Checklist (`PROF-REV-001`):** *(1) `volatile` & memory barriers on shared/DMA state, (2) Atomic register ops (`BSRR`, `rc_w1`/`rc_w0`), (3) Zero unbounded hardware `while()` polls (all have timeouts!), (4) Integer overflow/underflow & promotion safety, (5) ISR duration & `BASEPRI`/`FromISR` priority compliance, (6) Lock ordering (no AB/BA deadlock), (7) Hardware pin/power-mode conflicts (`UCPD_DBDIS`, `PB8-BOOT0`, `MODER=11b`), (8) Stack & Flash/RAM budget check.*

### 3.3 Host Unit Testing, Register Fakes, HIL & CI/CD (`TEST-*`, `PROF-CI-001`)
- **Test-Driven Bare-Metal Drivers (`TEST-UNIT-002`, `TEST-MOCK-001`):** Inject `GPIO_TypeDef *port` or `const i2c_ops_t *bus` pointers instead of hardcoding `GPIOA`/`I2C1` globals! On an x86/x64 host running **Unity / CMock / Ceedling**, allocate a RAM `GPIO_TypeDef fake_gpio = {0};` struct, invoke `gpio_set_output(&fake_gpio, 5U)`, and assert `fake_gpio.MODER == (1U << 10)` in 5 milliseconds with ASan+UBSan and `gcov` coverage!
- **Automated HIL & Emulation (`TEST-HIL-001/002`):**
  - **HIL (`pytest` + `pyserial` + OpenOCD/GDB batch):** Flashes built `.elf`, runs automated protocol/register tests against a real NUCLEO-G474RE over ST-LINK VCP, emits JUnit XML.
  - **Emulation (Renode / QEMU Cortex-M4):** Runs headless firmware ELF regression tests inside GitHub Actions without physical hardware.
- **GitHub Actions Firmware CI (`PROF-CI-001`):** Every PR automatically runs: (1) `arm-none-eabi-gcc` build with `-Werror`, (2) Flash/RAM size-budget gate via `arm-none-eabi-size`, (3) Host Unity tests under `-fsanitize=address,undefined`, (4) `cppcheck` + `clang-format --dry-run --Werror`, (5) Doxygen (`WARN_AS_ERROR = YES`).
- **Git Discipline & Build Provenance (`PROF-GIT-002`, `BOOT-CFG-001`):** Embed a `const fw_info_t` struct (Magic, SemVer `MAJOR.MINOR.PATCH`, Git SHA + dirty flag from `-DFW_GIT_SHA`) into a fixed Flash section via `KEEP(*(.fw_info))` in `stm32g474re.ld`. Document design rationale with **ADRs** and binary wire protocols with **ICDs**.

### 3.4 Static Stack Analysis, `DWT->CYCCNT` WCET & Optimization (`PROF-STK/WCET/MEM/PERF`)
- **Static Stack Analysis (`-fstack-usage -Wstack-usage=256`):** GCC emits `.su` files listing exact per-function stack frames (`static`, `dynamic`, `bounded`). Sum worst-case call chain + nested ISR frames:
  - Non-FPU ISR frame = **32 bytes** (8 words); FPU-active ISR frame = **104 bytes** (26 words: `R0–R3, R12, LR, PC, xPSR, S0–S15, FPSCR, pad`!) + callee-saved registers!
- **Cycle-Accurate WCET via `DWT->CYCCNT` (`PROF-WCET-001`):**
  Enable `CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;`. At 170 MHz, each count = **$5.8824\text{ ns}$** (wraps every 25.26 s — always use unsigned difference `(uint32_t)(end - start)`!).
- **Optimization Toolbox:**
  - *Memory:* Triage top consumers via `arm-none-eabi-nm --size-sort -S -r firmware.elf` & `.map`; reorder struct fields largest-to-smallest (avoiding unaligned `__packed__` penalties); move constant LUTs to `static const` (`.rodata` in Flash); compile with `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections -flto --specs=nano.specs`.
  - *Performance:* Relocate critical ISRs/DSP loops to **0-WS CCM-SRAM (`0x10000000`, `CCMRAM (xrw)` + `_siccmram` copy loop)**; keep ART `PRFTEN | ICEN | DCEN` ON; offload trig/filter math to **`CORDIC`** & **`FMAC`**!

### 3.5 STM32G474RE Hardware Firmware Security (`SEC-*`) `[STM32G474RE Exact]`
> **CRITICAL SILICON FACT (`PERIPH-AES-001`):** **STM32G474RE has NO hardware AES / CRYP / HASH peripheral!** Hardware AES exists ONLY on **STM32G483 / STM32G484** (verified: 0 `AES` definitions in `stm32g474xx.h`, IRQ 85 is reserved). On STM32G474RE, run Ed25519/ECDSA-P256/SHA-256 in software (e.g., Monocypher/TinyCrypt/mbedTLS) backed by STM32G474RE's 5 real hardware security mechanisms:

1. **Readout Protection (`FLASH->OPTR.RDP[7:0]`, `SEC-RDP-001`):**
   - `0xAA` = **Level 0** (open debug & Flash).
   - Any other value (e.g., `0xBB`) = **Level 1** (SWD Flash/SRAM2/`TAMP->BKPxR` reads blocked; regressing L1 $\to$ L0 triggers automatic **full Flash mass-erase**!).
   - **`0xCC` = Level 2 (IRREVERSIBLE PERMANENT LOCKOUT!):** Permanently disables SWD/JTAG and System ROM bootloader forever — **NEVER set `0xCC` on a development NUCLEO board!**
2. **Flash Write Protection (`WRP1AR/1BR/2AR/2BR`, `SEC-MEM-001`):** Protects bootloader pages at 2 KB granularity against runtime erase/write (`FLASH_SR_WRPERR`).
3. **Proprietary Code Readout Protection (`PCROP1SR/ER`, `PCROP2SR/ER`, `SEC-MEM-001`):** Marks IP Flash ranges as **execute-only** (D-bus data reads fault!). Requires compiling protected code with GCC **`-mpure-code`** so literal pools are not placed in code Flash!
4. **Securable Memory Area (`FLASH->SEC1R`/`SEC2R`, `BOOT_LOCK`, `FLASH->CR.SEC_PROT1/2`, `SEC-BOOT-001`):** Execute-once immutable Root of Trust (RoT) holding bootloader verification keys; before jumping to the Application, a RAM trampoline sets `SEC_PROT1 = 1`, rendering the RoT Flash completely invisible (reads return 0 / BusFault) until the next hardware reset!
5. **`TAMP` Active/Passive Tamper Erasure (`0x40002400`, `SEC-TAMP-001`) & Hardware `RNG` (`0x50060800`, `SEC-CRYPTO-001`):**
   - `TAMP` automatically zeroes all 32 `TAMP->BKP0R..31R` registers + SRAM2 upon physical tamper pin (`TAMP_IN1` = `PC13`) or internal voltage/clock/temp attack.
   - Hardware **`RNG`** (`RCC->AHB2ENR.RNGEN` b26 + `HSI48`/`CLK48SEL`) provides NIST SP800-90B 32-bit true random numbers for cryptographic nonces/challenges (check `CECS`/`SECS` health flags!).

#### 3.5.1 Bootloader Anti-Rollback Protection, Monotonic Counters & Security Version Pinning (`SEC-BOOT-002`)

Cryptographic firmware signing (Ed25519 / ECDSA-P256) blocks *forged* images, but **does NOT prevent a Downgrade / Rollback Attack**: an attacker can replay an *older, genuinely signed* production image (e.g., `v1.2.0`) that contains a known CVE or debug backdoor! Per **MCUboot Security Design**, **NIST SP 800-193**, and **ST AN5156**:

1. **Signed Metadata `security_version` Epoch:** Separate the human-facing SemVer (`major.minor.patch`) from an unsigned integer **`security_version`** inside the signed firmware header (`fw_signed_header_t`). Increment `security_version` *only* on security-relevant fixes, and **ALWAYS include `security_version` inside the SHA-256 / Ed25519 signed digest** alongside `magic`, `image_size`, and the firmware payload!
2. **STM32G474RE Non-Volatile Monotonic Counter Backends (Why SRAM / `TAMP->BKPxR` Alone Fails):**
   - SRAM loses state on power cycle and `TAMP->BKPxR` resets if VBAT drops. A true anti-rollback floor must reside in non-volatile memory writable **only by the Root-of-Trust bootloader** (inside `FLASH->SEC1R` before locking `SEC_PROT1` or `WRP1AR`-protected pages):
   - **Backend A — 64-Bit ECC Append-Only Flash Log Page (Up to 256 Increments / 2 KB Page):** Because STM32G4 Flash enforces **64-bit double-word programming with 8-bit SECDED ECC** (you **cannot** clear individual bits inside an already-programmed double-word like legacy NOR Flash — doing so raises `PROGERR` and corrupts ECC!), store each increment as a fresh 64-bit double-word slot `{ uint32_t sec_ver; uint32_t sec_ver_inv; }` (`sec_ver == ~sec_ver_inv` detects power-loss torn writes!) sequentially down a reserved 2 KB bootloader-only Flash page.
   - **Backend B — Hardware 1 KB OTP Area (`0x1FFF7000..0x1FFF73FF`, 128 × 64-Bit Permanent Write-Once Slots):** For physically irreversible, erase-proof security version pinning across product lifetime, program each new security epoch into the next erased (`0xFFFFFFFFFFFFFFFFULL`) 64-bit OTP slot — even a compromised runtime kernel with Flash erase access can **never** erase or decrement OTP!
3. **CRITICAL Trial-Boot vs Counter-Commit Ordering (Avoid Self-Bricking!):**
   - **NEVER increment the monotonic counter *before* the new image boots and passes self-test!**
   - Safe OTA state flow: (1) Verify cryptographic signature over `[header || payload]` FIRST, (2) Enforce `hdr->security_version >= nv_counter`, (3) Boot new image in **Trial Mode** (`IWDG` armed; old bank remains valid because `old_sec_ver == nv_counter`), (4) If the new image crashes $\to$ watchdog rolls back to old bank cleanly, (5) Once the new image passes health self-test and commits $\to$ bootloader advances `nv_counter = hdr->security_version`, permanently blocking downgrade to any prior vulnerable epoch!

### 3.6 Fault Tolerance, FMEA, Memory Integrity & Post-Mortem Crash Logging (`PROF-REL-*`, `REL-*`)
- **FMEA (Failure Mode & Effects Analysis):** Rank failure modes by $\text{RPN} = \text{Severity} \times \text{Occurrence} \times \text{Detection}$ (1–10 each) and assign deterministic hardware mitigations (e.g., `COMP1` $\to$ `TIM1_BKIN` hardware trip, `BOR_LEV >= 3`, fail-safe pull-downs on MOSFET gates).
- **Hardware Memory Integrity & Lockup Safety (`PROF-REL-001/002`):**
  - Enable `SYSCFG->CFGR2 |= SYSCFG_CFGR2_CLL | SYSCFG_CFGR2_SPL | SYSCFG_CFGR2_PVDL | SYSCFG_CFGR2_ECCL;` so Cortex-M4 **Core Lockup**, **SRAM Parity Error**, **PVD**, or **Flash 2-Bit ECC Error** directly asserts hardware Break on `TIM1/8/15/16/17/20/HRTIM1`!
  - Handle `FLASH->ECCR.ECCD` and `SYSCFG->CFGR2.SPF` inside **`NMI_Handler`**.
- **Boot-Loop Containment (`REL-DEGRADE-001`):** Increment a persistent boot/crash counter in `TAMP->BKP4R` / `.noinit` SRAM; if `crash_count >= 3` within 10 s, halt normal actuation and enter **Safe Recovery Mode** (diagnostic CLI + bootloader ready).
- **Naked HardFault MSP/PSP Crash Logger (`REL-CRASH-001`):** Under FreeRTOS, task crashes occur on **`PSP`**, NOT `MSP`! Inspect `EXC_RETURN` bit 2 (`tst lr, #4`) in naked assembly to capture the true faulting frame (`R0–R3, R12, LR, PC, xPSR`) + `SCB->CFSR/HFSR/MMFAR/BFAR` into CRC-protected `.noinit` SRAM before resetting!

---

## 4. Core Terminology & Acronyms

| Term | Engineering Definition |
|---|---|
| **MISRA Deviation Permit** | Formally documented, risk-assessed, scoped justification for an unavoidable rule violation (e.g., Rule 11.4 MMIO cast). |
| **ASan / UBSan** | AddressSanitizer / UndefinedBehaviorSanitizer — host-side runtime detectors for memory corruption and C UB. |
| **WCET (`DWT->CYCCNT`)** | Worst-Case Execution Time measured cycle-accurately via Cortex-M4 Data Watchpoint and Trace cycle counter. |
| **RDP / WRP / PCROP / `SEC1R`** | STM32G474RE Flash security layers: Readout Protection, Write Protection, Execute-Only IP, and Execute-Once Securable Memory. |
| **FMEA / RPN** | Failure Mode and Effects Analysis / Risk Priority Number ($\text{Severity} \times \text{Occurrence} \times \text{Detection}$). |
| **ADR / ICD** | Architecture Decision Record / Interface Control Document specifying wire protocols and subsystem contracts. |

---

## 5. Hardware Register Architecture & Bitfield Reference

Verified directly against `stm32g474xx.h` and `core_cm4.h`:

| Register | Address | Key Bitfields & Engineering Purpose |
|---|---|---|
| **`CoreDebug->DEMCR`** / **`DWT->CTRL`** / **`DWT->CYCCNT`** | `0xE000EDFC` / `0xE0001000` / `0xE0001004` | `TRCENA` (b24) + `CYCCNTENA` (b0) enables free-running 32-bit CPU cycle counter |
| **`SCB->CFSR` / `HFSR` / `MMFAR` / `BFAR`** | `0xE000ED28..38` | Decodes MemManage, BusFault (`BFARVALID`), UsageFault (`DIVBYZERO`, `UNALIGNED`), and Forced HardFault |
| **`SYSCFG->CFGR2`** | `0x4001001C` | `CLL` (b0 Core Lockup), `SPL` (b1 SRAM Parity Lockup), `PVDL` (b2), `ECCL` (b3) $\to$ Timer Break; `SPF` (b8 parity flag) |
| **`FLASH->OPTR` / `WRP1AR` / `PCROP1SR` / `SEC1R`** | `0x40022020..74` | `RDP[7:0]`, `SRAM_PE` (b24), Write-Protect pages, Execute-Only range, `SEC_SIZE1` & `BOOT_LOCK` |
| **`RNG` (`0x50060800`)** | `CR@0x00, SR@0x04, DR@0x08` | `RNGEN` (b2), `DRDY` (b0), `CECS`/`SECS`/`CEIS`/`SEIS` health monitors (requires 48 MHz `CLK48SEL`!) |
| **`TAMP` (`0x40002400`)** | `CR1@0x00, SR@0x30, BKP0R..31R@0x100..17C` | Tamper pin/internal detectors auto-zeroing 128 B backup registers + SRAM2 |

---

## 6. Engineering Mathematics & Worked Formulas

### 6.1 Worst-Case Nested Interrupt Stack Budget (Cortex-M4F)
$$S_{\text{MSP,max}} = S_{\text{main,peak}} + \sum_{p \in \text{nesting levels}} \left( S_{\text{hw,frame},p} + S_{\text{ISR,local},p} \right)$$
where $S_{\text{hw,frame}} = 32\text{ B}$ (non-FPU) or **$104\text{ B}$** (FPU extended frame!). Always verify $S_{\text{MSP,max}} \times 1.25 \le S_{\text{stack budget}}$ (`_Min_Stack_Size`).

### 6.2 `DWT->CYCCNT` Time Conversion at 170 MHz
$$t_{\mu\text{s}} = \frac{\left(t_{\text{end}} - t_{\text{start}}\right) \bmod 2^{32}}{170.0}$$
Max single measurable interval before 32-bit wrap: $2^{32} / 170\times 10^6 = \mathbf{25.264\text{ s}}$.

---

## 7. Official Documentation & Standards References

| Document | Scope |
|---|---|
| **MISRA C:2012 (Amd 1–4) / MISRA C:2023 / MISRA Compliance:2020** | Safety-critical C guidelines & deviation management |
| **SEI CERT C Coding Standard / Barr Group Embedded C Standard** | Secure & bug-resistant C coding rules |
| **ST RM0440** (§2.4 SRAM Parity, §3.5 Security/RDP/WRP/PCROP/Securable Area, Ch. 33 RNG, Ch. 38 TAMP) & **DS12288 Table 2** (G474 vs G484 crypto matrix) | `[STM32G474RE Exact]` |
| **ST AN5281 / AN4758 / AN4230 / AN5156** | Securable Memory Area, PCROP, STM32 Hardware RNG Validation, & STM32 Security Overview |
| **MCUboot Security Design & NIST SP 800-193** | Platform Firmware Resiliency, Signed Metadata Headers & Anti-Rollback Monotonic Counters (`SEC-BOOT-002`) |
| **James Grenning — *TDD for Embedded C* / Elecia White — *Making Embedded Systems*** | Host testing, fakes/mocks, and embedded architecture |

---

## 8. Production-Grade Bare-Metal C Code Examples

> **Verification Status:** Statically Reviewed against `stm32g474xx.h` & `core_cm4.h` (Not Tested on Physical Hardware).

### 8.1 Cycle-Accurate `DWT->CYCCNT` Profiler & Hardware `RNG` Reader
```c
#include "stm32g474xx.h"
#include <stdint.h>
#include <stdbool.h>

void DWT_CycleCounter_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t DWT_GetCycles(void)
{
    return DWT->CYCCNT;
}

bool HW_RNG_Read32(uint32_t *out_rand)
{
    /* Assumes HSI48 ON + RCC->CCIPR.CLK48SEL = 00b + RCC->AHB2ENR.RNGEN = 1 + RNG->CR.RNGEN = 1 */
    uint32_t timeout = 100000U;
    while (timeout-- > 0U) {
        uint32_t sr = RNG->SR;

        /* Clock Error (CECS/CEIS): clear rc_w0 directly (NEVER '&=') and abort for clock fix */
        if ((sr & (RNG_SR_CECS | RNG_SR_CEIS)) != 0U) {
            RNG->SR = ~RNG_SR_CEIS;
            return false;
        }

        /* Seed Error (SECS/SEIS): RM0440 Ch. 33 recovery — direct rc_w0 clear,
         * disable/re-enable RNGEN, and re-poll until SECS clears and DRDY asserts */
        if ((sr & (RNG_SR_SECS | RNG_SR_SEIS)) != 0U) {
            RNG->SR  = ~RNG_SR_SEIS;
            RNG->CR &= ~RNG_CR_RNGEN;
            RNG->CR |=  RNG_CR_RNGEN;
            continue;
        }

        if ((sr & RNG_SR_DRDY) != 0U) {
            uint32_t val = RNG->DR;
            if (val != 0U) { /* RM0440: discard zero word */
                *out_rand = val;
                return true;
            }
        }
    }
    return false;
}
```

### 8.2 Naked Cortex-M4 MSP/PSP HardFault Capture into `.noinit` SRAM
```c
typedef struct {
    uint32_t magic;
    uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
    uint32_t cfsr, hfsr, mmfar, bfar;
    uint32_t crash_count;
} crash_record_t;

__attribute__((section(".noinit")))
static volatile crash_record_t g_crash_log;

void HardFault_Capture_C(const uint32_t *frame)
{
    g_crash_log.magic = 0xDEADC0DEUL;
    g_crash_log.r0    = frame[0];
    g_crash_log.r1    = frame[1];
    g_crash_log.r2    = frame[2];
    g_crash_log.r3    = frame[3];
    g_crash_log.r12   = frame[4];
    g_crash_log.lr    = frame[5];
    g_crash_log.pc    = frame[6]; /* Resolve exact source line via arm-none-eabi-addr2line! */
    g_crash_log.xpsr  = frame[7];
    g_crash_log.cfsr  = SCB->CFSR;
    g_crash_log.hfsr  = SCB->HFSR;
    g_crash_log.mmfar = SCB->MMFAR;
    g_crash_log.bfar  = SCB->BFAR;
    g_crash_log.crash_count++;
    __DSB();
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U) {
        __BKPT(0);
    }
    NVIC_SystemReset();
}

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4                \n" /* EXC_RETURN bit 2: 0 = MSP, 1 = PSP (FreeRTOS task!) */
        "ite eq                    \n"
        "mrseq r0, msp             \n"
        "mrsne r0, psp             \n"
        "b HardFault_Capture_C     \n"
    );
}
```

### 8.3 Bootloader Anti-Rollback Verifier & 64-Bit ECC Monotonic Counter Reader (`SEC-BOOT-002`)

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `stm32g474xx.h` (Not Tested on Physical Hardware). Works identically on a `FLASH->SEC1R`-protected 2 KB Flash page or STM32G474RE 1 KB OTP (`0x1FFF7000`, 128 × 64-bit slots).

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FW_HDR_MAGIC 0x47344657UL /* "G4FW" */

typedef struct {
    uint32_t magic;
    uint32_t security_version; /* Strictly monotonic anti-rollback epoch */
    uint32_t image_size;
    uint32_t crc32;
    uint8_t  signature[64];    /* Ed25519 signature covering [magic..crc32 || payload] */
} fw_signed_header_t;

typedef struct {
    uint32_t sec_ver;
    uint32_t sec_ver_inv;      /* Bitwise complement (~sec_ver) catches torn power-fail writes */
} nv_counter_slot_t;

_Static_assert(sizeof(nv_counter_slot_t) == 8U, "STM32G4 Flash/OTP ECC requires 64-bit double-word slots");

uint32_t nv_counter_read_current(uint32_t page_base_addr, uint32_t num_slots)
{
    const volatile nv_counter_slot_t *slots = (const volatile nv_counter_slot_t *)page_base_addr;
    uint32_t highest = 0UL;

    for (uint32_t i = 0UL; i < num_slots; ++i) {
        uint32_t v   = slots[i].sec_ver;
        uint32_t inv = slots[i].sec_ver_inv;
        if ((v == 0xFFFFFFFFUL) && (inv == 0xFFFFFFFFUL)) {
            break; /* Reached blank tail of append-only Flash/OTP log */
        }
        if ((v == ~inv) && (v > highest)) {
            highest = v;
        }
    }
    return highest;
}

typedef enum {
    BOOT_AUTH_OK = 0,
    BOOT_AUTH_ERR_MAGIC,
    BOOT_AUTH_ERR_ROLLBACK,
    BOOT_AUTH_ERR_SIGNATURE
} boot_auth_status_t;

boot_auth_status_t bootloader_verify_anti_rollback(
    const fw_signed_header_t *hdr,
    uint32_t nv_min_security_ver,
    bool (*verify_ed25519_fn)(const fw_signed_header_t *h))
{
    if ((hdr == NULL) || (hdr->magic != FW_HDR_MAGIC)) {
        return BOOT_AUTH_ERR_MAGIC;
    }
    /* Authenticate cryptographic signature FIRST so unsigned headers cannot probe state */
    if ((verify_ed25519_fn == NULL) || !verify_ed25519_fn(hdr)) {
        return BOOT_AUTH_ERR_SIGNATURE;
    }
    /* Reject any replay/downgrade whose signed security_version < pinned NV floor */
    if (hdr->security_version < nv_min_security_ver) {
        return BOOT_AUTH_ERR_ROLLBACK;
    }
    return BOOT_AUTH_OK;
}
```

---

## 9. Practical Laboratory Experiments & Debugging Checkpoints

- **Host Unit Test & Sanitizer Checkpoint:**
  ```bash
  gcc -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
      tests/test_ringbuf.c Src/ringbuf.c -o build/test_ringbuf && ./build/test_ringbuf
  ```
- **Static Stack & Symbol Footprint Checkpoint:**
  ```bash
  arm-none-eabi-gcc -fstack-usage -Wstack-usage=256 ...
  arm-none-eabi-nm --size-sort -S -r build/firmware.elf | head -n 20
  arm-none-eabi-addr2line -e build/firmware.elf -f -C 0x08001234
  ```
- **Security Verification Checkpoint:** Inspect `FLASH->OPTR & 0xFF` (`0xAA` = RDP Level 0); test `TAMP_IN1` (`PC13`) auto-zeroing `TAMP->BKP0R..3R`; **NEVER write `0xCC` (RDP Level 2) on a lab Nucleo board!**

---

## 10. Common Engineering Mistakes, Hardware Traps & Silicon Errata (ES0430)

1. **Assuming STM32G474RE Has Hardware AES:** AES/CRYP exists ONLY on **STM32G483/G484**! Attempting to locate `AES_BASE` in `stm32g474xx.h` fails because the silicon does not include it.
2. **Bricking a Dev Board with RDP Level 2 (`0xCC`):** Level 2 permanently fuses SWD/JTAG and bootloader off forever! Only ever test Level 1 (`0xBB` $\to$ `0xAA` regression) on development hardware.
3. **Enabling `PCROP` Without GCC `-mpure-code`:** Literal-pool loads inside execute-only Flash trigger immediate BusFaults!
4. **Enabling `FLASH->OPTR.SRAM_PE = 0` Without Zeroing ALL SRAM in `Reset_Handler`:** Reading uninitialized stack/heap words triggers phantom parity NMIs!
5. **Reading Only `MSP` Inside `HardFault_Handler` Under FreeRTOS:** Always inspect `EXC_RETURN` bit 2 (`tst lr, #4`) to capture the faulting task's `PSP` frame!
6. **Hardcoding Windows Tool Paths in `Makefile`:** Breaks Linux/macOS/GitHub Actions CI runners; parameterize `OPENOCD_SCRIPTS` and compiler prefixes.

---

## 11. Progressive Coding Exercises & Design Challenges

1. **[Level 2]** Author a formal MISRA Deviation Permit for Rule 11.4 (CMSIS MMIO casts) and pass `cppcheck --enable=all --error-exitcode=1` + GCC `-Wall -Wextra -Werror -fanalyzer` with 0 warnings across a bare-metal driver.
2. **[Level 2]** Refactor `gpio_driver.c` and `ringbuf.c` to accept injected pointers/fake structs, write 15+ Unity unit tests compiled on host with `-fsanitize=address,undefined`, and achieve $\ge 90\%$ branch coverage (`gcov`).
3. **[Level 2]** Add `-fstack-usage`, stack painting (`0xA5A5A5A5`), and `.noinit` naked `HardFault_Handler`; trigger an intentional `DIV_0_TRP` UsageFault inside a FreeRTOS task and resolve the logged `PC` with `arm-none-eabi-addr2line`.
4. **[Level 3]** Configure `SYSCFG->CFGR2 |= SYSCFG_CFGR2_CLL` + `TIM1_BKIN` and `TAMP` external tamper on `PC13` erasing `TAMP->BKP0R..3R`, plus read 10,000 hardware `RNG->DR` words verifying `CECS == SECS == 0`.

---

## 12. Self-Assessment Mastery Questions

1. **Q:** Why can hardware `CRC-32` verify firmware integrity against accidental Flash bit-flips, yet NEVER authenticate firmware against an attacker?  
   *Answer Check:* CRC-32 is an unkeyed linear polynomial — an attacker modifying firmware can trivially recompute a matching CRC-32. Cryptographic authentication requires asymmetric signatures (e.g., Ed25519 / ECDSA-P256).
2. **Q:** What happens when regressing STM32G474RE from RDP Level 1 back to RDP Level 0 vs setting RDP Level 2?  
   *Answer Check:* L1 $\to$ L0 triggers an automatic full hardware mass-erase of Flash, SRAM2, and `TAMP->BKPxR` (protecting secrets while keeping the chip reusable). L2 (`0xCC`) permanently bricks SWD/JTAG/bootloader forever.
3. **Q:** Why does `HardFault_Handler` use `__attribute__((naked))` and test `lr & 4`?  
   *Answer Check:* A non-naked C function pushes registers onto `MSP`, corrupting stack offsets before capture; `EXC_RETURN` bit 2 indicates whether the exception frame was stacked onto `MSP` (`0`) or `PSP` (`1`).
4. **Q:** (`SEC-BOOT-002`) Why can you NOT implement a bit-clearing monotonic counter inside a single 64-bit word of STM32G474RE Flash, and why must the monotonic counter only advance *after* a newly booted OTA image passes self-test?  
   *Answer Check:* STM32G4 Flash stores 8-bit SECDED ECC per 64-bit double-word — overwriting a programmed double-word triggers `PROGERR` and ECC faults, requiring append-only 64-bit slots (or 1 KB OTP `0x1FFF7000`). Advancing the counter before self-test would block automatic rollback to the known-good previous bank if the new image crashes!

---

## 13. Measurable Mastery & Completion Criteria

| Concept ID | Pass / Fail Verification Criterion | Measurement Tool |
|---|---|---|
| `PROF-STD-001..004` / `PROF-SA-001..003` | Zero errors under `-Wall -Wextra -Werror -fanalyzer`, `cppcheck --error-exitcode=1`, `clang-format --dry-run --Werror`, + documented Rule 11.4 Deviation Permit | GCC / `cppcheck` / CI |
| `TEST-UNIT-001..002` / `PROF-DA-002` | $\ge 15$ host Unity tests (using fake `GPIO_TypeDef`/`TIM_TypeDef` structs) passing under `-fsanitize=address,undefined` with $\ge 90\%$ branch coverage | Host GCC + `gcov` |
| `PROF-STK-001` / `PROF-WCET-001` | `.su` worst-case call-graph + nested FPU frame $\le 75\%$ stack budget; `DWT->CYCCNT` min/avg/max WCET profiled on target | GCC `.su` + GDB |
| `SEC-TAMP-001` / `SEC-CRYPTO-001` / `SEC-BOOT-002` | `TAMP` pin zeroes `BKP0R..3R`; hardware `RNG` generates 10,000 words with `CECS==SECS==0`; `bootloader_verify_anti_rollback()` rejects downgraded `security_version` while permitting equal/higher signed epochs | GDB + Host Unit Test |
| `REL-CRASH-001` / `PROF-REL-001` | Naked `HardFault_Handler` persists faulting `PSP` `PC`/`CFSR` in `.noinit` matching `addr2line`; 6-row FMEA + `SYSCFG->CFGR2` lockup break verified | GDB + Oscilloscope |

---

> **Sequential Navigation (Step 17 of 17):**  
> **Prev:** [Stage 16 (14) — FreeRTOS `ARM_CM4F` (`16-rtos/`)](../16-rtos/README.md) | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Capstone Projects (`19-projects/`)](../19-projects/README.md) & [Stage 05 Mastery Exam](../24-assessments/stage-05-rtos-and-advanced-firmware-exam.md)
