# Known Limitations & Honest Phase 5 Continuation Roadmap

> [!NOTE]
> **Historical Audit Snapshot Notice:** This document records an earlier audit milestone (`v2.1.0` / commit `585dc4f`, 2026-11-18). Its findings and metric counts reflect that historical state and are preserved verbatim for traceability. For the authoritative, canonical release metrics and active status of the current repository (`v4.0.0`), see [`AUDIT/release_metrics.json`](release_metrics.json) and [`AUDIT/RELEASE_INTEGRITY_REPORT.md`](RELEASE_INTEGRITY_REPORT.md).

This document enumerates every unresolved limitation in `<repo-root>/` as of the **Post-Red-Team Recalibrated Release (`v2.1.0`)**. In strict accordance with engineering honesty standards, **no limitation is hidden or inflated as "complete"**.

---

## 1. Physical Hardware Verification Limitation (Highest-Priority Gap)

- **Limitation:** **Zero STM32 target binaries (`23-code-library/templates/` and `18-practical-laboratory/Labs 01–19`) have been flashed to or measured on physical NUCLEO-G474RE (`MB1367 Rev C`) silicon.**
- **What IS Verified:** All bare-metal C templates, 4 remediated drivers, and Labs `01–14` & `16–19` are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`)** against official ST CMSIS `stm32g474xx.h`, with ELF `.isr_vector` (472 B, 512 B align), `.ccmram` (`AX`), and `.noinit` (`NOBITS`) verified via `arm-none-eabi-readelf`, and **113/113** host unit assertions passing under `gcc 15.2.0`.
- **Concepts Explicitly Flagged `NEEDS_VERIFICATION` (`5` items in [`AUDIT/concepts_sot.json`](concepts_sot.json)):**
  1. `HWSW-PWR-001` — Real `JP6` (`IDD`) current measurement across Run (170 MHz), Stop 0, Stop 1, and Standby on NUCLEO-G474RE.
  2. `RT-PWR-002` — Measured microamp leakage delta on `JP6` before vs. after parking floating GPIOs to Analog Mode (`MODER=11b`).
  3. `ANA-VREF-001` — Physical `VREF+` solder-bridge removal, `VREFBUF` 2.500 V accuracy, and real ADC noise-floor measurement.
  4. `G4-HRTIM-001` — Real `HRTIM1` 184 ps DLL jitter and sub-2 ns duty-step measurement on a high-bandwidth oscilloscope.
  5. `DIG-ELEC-006` — Empirical `OSPEEDR` 10 MHz MCO overshoot/ringing & 33 Ω source-termination oscilloscope verification.

---

## 2. Twelve Newly Discovered `MISSING` Concepts (Agent 10 Red-Team Backlog)

Independent adversarial auditing ([`AUDIT/INDEPENDENT_RED_TEAM_REPORT.md`](INDEPENDENT_RED_TEAM_REPORT.md), `[HIGH-06]`) identified **12 important embedded C / STM32G474RE production concepts** with `0` coverage in the Part 4 baseline. All 12 are now tracked honestly as **`MISSING`** in [`AUDIT/concepts_sot.json`](concepts_sot.json) (`350` total concepts) and [`AUDIT/MISSING_CONCEPTS_AND_GAPS.md`](MISSING_CONCEPTS_AND_GAPS.md) with concrete Phase 5 deliverables:

1. **`C-LIB-001`** — Newlib / Newlib-Nano Thread-Safety (`_reent`, `__malloc_lock` & `configUSE_NEWLIB_REENTRANT`) under FreeRTOS.
2. **`GPIO-FMP-001`** — `SYSCFG->CFGR1` I2C Fast-Mode Plus 20 mA Drive (`SYSCFG_CFGR1_I2C_PBx_FMP` / `I2C1_FMP`).
3. **`TIM-DMA-001`** — Timer DMA Burst Mode (`TIMx->DCR` @ `0x3DC` & `TIMx->DMAR` @ `0x3E0` atomic `ARR`+`CCR1..4` updates).
4. **`COMM-LP-001`** — `LPUART1` / `USART` Autonomous Wakeup from Stop Mode (`USART_CR1_UESM`, `USART_CR3_WUS`, `RCC_CR_HSIKERON`).
5. **`COMM-CAN-002`** — `FDCAN` Transmitter Delay Compensation (`TDCR`/`TDCV`), TX Event FIFO (`TXEFS` @ `0x0E4`) & `PSR.BO` Bus-Off Recovery.
6. **`SEC-BOOT-002`** — Bootloader Anti-Rollback Protection, Monotonic Version Counters & Security Version Pinning.
7. **`TOOL-LD-004`** — GNU `ld` `--wrap=symbol` Link-Time Mocking & `.ccmram` (`0x10000000`) ↔ `FLASH` (`0x08000000`) Long-Branch Veneers (`__veneer`).
8. **`COMM-USB-002`** — STM32G4 USB FS 16-Bit Halfword PMA (`USB_PMAADDR = 0x40006000`) Alignment Quirk & Endpoint Double-Buffering.
9. **`TOOL-DBG-004`** — `SWO` Wire Encoding (`TPIU->SPPR` NRZ vs Manchester) & `TPIU->ACPR` Baud Prescaler Math at 170 MHz.
10. **`MEM-OB-001`** — Flash Option-Byte `FLASH_CR_OBL_LAUNCH` Cold-Reset Mechanics & `BFB2` / `DBANK` Dual-Bank Swap Brick Traps.
11. **`FW-DRV-003`** — Zero-Overhead Static HAL Dispatch (X-Macros & `static inline` Board Policies) vs Runtime Vtables.
12. **`HWSW-ANA-001`** — Analog Power-Rail Integrity (`VDDA`/`VREF+` Ferrite Bead, MLCC ESR/ESL & Ground Plane Return Paths).

---

## 3. Unvendored Stacks, Untested C/Tooling Concepts & Deferred Labs (`137` Concepts Honestly Marked `PARTIAL`)

Following strict Red-Team de-inflation (`[HIGH-05]`), **`137` applicable concepts** (`39.8%`) with complete theory/reference coverage but lacking dedicated compiled/executed code, vendored kernels, or physical bench proof remain honestly classified as **`PARTIAL`**:

1. **FreeRTOS Kernel Sources Not Vendored (All `20` Applicable Domain 19 `RTOS-*` Concepts Honestly `PARTIAL`):**
   - `Lab 15` is **Statically Reviewed** rather than compiled in CI because FreeRTOS kernel `.c`/`.h` sources (`portable/GCC/ARM_CM4F/port.c`) are not vendored into `23-code-library/`.
   - **Next Task:** Add a pinned Git submodule for `FreeRTOS-Kernel` (e.g., `V11.1.0`) and a compile-verified `23-code-library/templates-freertos/` target including `LPTIM1` Tickless Idle (`configUSE_TICKLESS_IDLE=2`) and Newlib `__malloc_lock` hooks (`C-LIB-001`).
2. **Untested C11 & Embedded C Concurrency Concepts (`21` C concepts + `3` Embedded C atomics concepts):**
   - `tests/test_c_foundations.c` currently executes 35 assertions covering core integer, bitfield, endianness, pointer, struct padding, and `static`/`const`/`volatile` behavior; dedicated unit/disassembly tests for `C-ALIGN-002` (`packed`), `C-ARR-003..004`, `C-FPTR-002..003`, `C-INT-004`, `C-PREP-003`, `C-PTR-004`, `C-TYPE-002..003`, `C-UB-001/003`, `EMB-ATOM-001..003` (`PRIMASK`/`BASEPRI`/`LDREX`/`STREX`), and `ARM-SYNC-001` remain Phase 5 test expansions.
3. **Complex Peripheral Stacks Without Standalone Compiled Labs:**
   - `PERIPH-USB-001` & `CLK-CRS-001` (bare-metal USB FS Device HID/CDC enumeration stack),
   - `G4-UCPD-001` (USB Type-C Power Delivery BMC/4b5b protocol stack),
   - `PERIPH-SAI-001` (SAI1 external I2S audio codec streaming),
   - `PERIPH-QSPI-001` (QUADSPI external NOR Flash XIP at `0x90000000`),
   - `PERIPH-LPTIM-001` & `PERIPH-RTC-001` (standalone Stop 1/Standby wakeup binaries),
   - `PERIPH-RNG-001` (standalone TRNG statistical verification lab),
   - `PERIPH-ENC-001`, `PERIPH-IC-001..002`, `PERIPH-OC-001`, `PERIPH-SYNC-001`, `PERIPH-GPIO-005` (`LCKR`), `PERIPH-EXTI-003` (`EMR1`), `PERIPH-ADC-004`, `PERIPH-SPI-002..003`, `PERIPH-UART-003..004`, `RT-WDG-002` (WWDG `EWI`), `REL-BOR-001`, `STM32-PWR-003`.
4. **Dual-Image Bootloader & Hardware Option-Byte Security Labs (`BOOT-*`, `SEC-*`, `MEM-ECC-001`, `STM32-FLASH-002`):**
   - Documented with exact STM32G474RE register sequences, 64-bit Flash programming, dual-bank `BFB2` swap, and explicit anti-bricking warnings (never burn `RDP` Level 2 `0xCC` on dev boards), but no dual-linker-script compiled bootloader+app OTA binary pair exists in-repo yet.
5. **Capstone Projects 01–03 (`19-projects/`, `APP-*`):**
   - All hardware pin conflicts, button polarity errors, and power-electronics safety defects were fixed in Part 4, but the projects remain architectural specifications without standalone multi-file `.c`/`.h` reference builds.
6. **CMSIS-SVD File Not Bundled & Hardware Debug Sessions Untested (`TOOL-PROBE-001..002`, `TOOL-DBG-001..002`, `TOOL-GDB-001..002`, `TOOL-FLASH-001`, `EMB-CMSIS-003`):**
   - `.vscode/launch.json` points to `23-code-library/Drivers/SVD/STM32G474.svd`; due to ST redistribution licensing, learners must download `STM32G474.svd` from ST or `cmsis-svd-data` (instructions in [`23-code-library/README.md`](../23-code-library/README.md)).
7. **Non-Fatal Linker Warnings in Template Build:**
   - Linking with `--specs=nano.specs --specs=nosys.specs` produces benign GNU `ld` informational warnings (`_write is not implemented` when `printf` is unused, and `LOAD segment with RWX permissions` due to `CCMRAM (xrw)`). Adding `-Wl,--no-warn-rwx-segments` and a minimal `_write()` stub in a follow-up cleanup will silence both.

---

## 4. Educational Video Library Coverage Limitations (`RES-VID-002`)

- While **50/50 YouTube URLs** in [`22-video-library/video-database.csv`](../22-video-library/video-database.csv) were live-verified (HTTP 200 oEmbed + exact metadata match) and **50/50** are mapped in [`topic-coverage-matrix.md`](../22-video-library/topic-coverage-matrix.md):
  - **24% (12/50)** are DigiKey's ESP32 (Xtensa) FreeRTOS series (Video #50 Multicore SMP is `NOT_APPLICABLE` to single-core STM32G474RE).
  - **12% (6/50)** are HAL/STM32CubeIDE STM32G4 videos rather than bare-metal CMSIS register programming.
  - **15 core topics** (Linker scripts, HardFault unwinding, MPU, FPU lazy stacking, FDCAN, HRTIM, CORDIC/FMAC, bare-metal DMAMUX, Dual-Bank Flash/Bootloaders, Low-Power/Watchdogs, MISRA/Unit Testing, Cortex-M4 FreeRTOS port, FSM/HSM, ITM/SWO/RTT, OPAMP/COMP) still have **0 bare-metal videos** and rely on written compensation guides.

---

## 5. Tooling & CI Automation Limitations (`TEST-HIL-001..002`, `PROF-DOC-001`, `PROF-SA-001..003`, `PROF-WCET-002`)

- **No Live HIL or Emulator Execution in CI:** Automated `pytest` + `pyserial` + OpenOCD HIL tests and headless Renode/QEMU emulation scripts are documented in `17-advanced-firmware-engineering/README.md`, but not yet implemented as executable CI jobs.
- **No `Doxyfile`, `.clang-tidy`, `cppcheck` CI, `-fanalyzer`, ASan/UBSan Linux CI, or `gcov`/`lcov` Coverage Gate Yet:** `.clang-format` and `.github/workflows/firmware-ci.yml` are active; adding `Doxyfile` (`WARN_AS_ERROR = YES`), `.clang-tidy`, `cppcheck`, `CMake+Ninja` CI execution (`TOOL-BUILD-002`), and `lcov` host branch coverage reporting remains a Phase 5 enhancement.
- **ARM Cortex-M4 TRM Revision Suffix:** `21-reference-library/arm-architecture-references.md` should confirm the exact Arm Developer revision letter (`DDI 0439B`/`D` for Cortex-M4 r0p1 vs `DDI 0439F`) during the next documentation maintenance pass.
- *(Note: The earlier Mermaid `.mmd` fence limitation has been resolved — `scripts/validate_repo.py` verifies **12/12 `.mmd` diagrams valid and free of HTML tags**.)*

---

## 6. Prioritized Phase 5 Continuation Tasks

| Priority | Continuation Deliverable | Target Concept IDs Closed |
|---|---|---|
| **P1** | Physical NUCLEO-G474RE (`MB1367`) bench sign-off: flash Labs `01–19`, measure `JP6` currents, `PA8` MCO 10.625 MHz, `COMP1 -> TIM1_BKIN` <50 ns trip, `HRTIM1` DLL jitter, `OSPEEDR` ringing, `VREFBUF` noise floor, and oscilloscope WCET toggles | `HWSW-PWR-001`, `RT-PWR-002`, `G4-HRTIM-001`, `ANA-VREF-001`, `DIG-ELEC-006`, `PROF-WCET-002`, `HWSW-LAB-001..003` |
| **P1** | Vendor `FreeRTOS-Kernel` submodule + compile-verified `templates-freertos/` (covering `Lab 15`, `LPTIM1` Tickless Idle, and Newlib `_reent`/`__malloc_lock`) | All `20` applicable `RTOS-*` concepts + `C-LIB-001` |
| **P1** | Author theory, verified register maps, and compile-verified code for the remaining **11 Agent 10 `MISSING` concepts** (especially `.ccmram` ↔ `FLASH` long-branch veneers, `SYSCFG` Fm+, `TIMx->DCR/DMAR`, `LPUART` Stop wakeup, `FDCAN` `TDCR`/Bus-Off, anti-rollback, `OBL_LAUNCH`/`BFB2`, `TPIU->ACPR`) | `GPIO-FMP-001`, `TIM-DMA-001`, `COMM-LP-001`, `COMM-CAN-002`, `SEC-BOOT-002`, `TOOL-LD-004`, `COMM-USB-002`, `TOOL-DBG-004`, `MEM-OB-001`, `FW-DRV-003`, `HWSW-ANA-001` |
| **P2** | Expand `tests/test_c_foundations.c` + add Linux ASan/UBSan CI job, `Doxyfile`, `.clang-tidy`, `cppcheck`, `CMake+Ninja` CI build, and `lcov` coverage report | `21` `PARTIAL` C concepts, `EMB-ATOM-001..003`, `TOOL-BUILD-002`, `PROF-DOC-001`, `PROF-SA-001..003`, `PROF-DA-002`, `TEST-COV-001` |
| **P2** | Build standalone multi-file compile-verified C codebases for Capstone Projects `01–03` and Dual-Bank A/B OTA Bootloader + executable Renode/HIL CI harness | `APP-INST-001`, `APP-LOG-001`, `APP-PWR-001`, `BOOT-FLASH/JUMP/VERIF/UPD-001`, `TEST-HIL-001..002` |
| **P3** | Curate verified bare-metal videos for the 15 zero-bare-metal-video topics and add standalone labs for `USB FS + CRS`, `UCPD1`, `SAI1`, and `QUADSPI` | `RES-VID-002`, `PERIPH-USB-001`, `G4-UCPD-001`, `PERIPH-SAI-001`, `PERIPH-QSPI-001` |
