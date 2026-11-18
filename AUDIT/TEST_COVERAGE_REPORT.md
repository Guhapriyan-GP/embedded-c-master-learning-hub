# Automated Testing, Code Coverage, Static Analysis & CI Report

**Repository:** `embedded-c-master-learning-hub`  
**Target MCU:** STM32G474RET6 (NUCLEO-G474RE, MB1367 Rev C — ARM Cortex-M4F, 170 MHz Boost / 160 MHz Boost)  
**Concept IDs:** `TOOL-QA-001`, `TOOL-CI-001`, `TOOL-STATIC-001`, `C-DATA-001`, `C-BIT-001`, `EMB-MMIO-001`, `EMB-CONC-001`, `PERIPH-GPIO-001`, `PERIPH-EXTI-001`, `PERIPH-TIM-001`, `PERIPH-UART-001`, `PERIPH-SPI-001`, `PERIPH-I2C-001`, `PERIPH-ADC-001`, `PERIPH-DMA-001`, `PERIPH-WDG-001`, `REL-FAULT-001`  
**Hardware Verification Disclaimer:** All firmware builds are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`)** and all unit tests/coverage metrics are **Host-Executed (`gcc 15.2.0` + `gcov 15.2.0`)**. No code in this report has been executed on physical NUCLEO-G474RE silicon; physical hardware-in-the-loop (HIL) testing remains strictly separate from host testing and cloud CI.

---

## 1. Executive Summary

| Verification Dimension | Baseline (Before) | Current (Verified) | Status |
|---|---|---|---|
| Host Unit Test Suites (`tests/`) | 2 suites | **4 suites** | PASS (0 failures) |
| Host Assertions Executed (`make -C tests test`) | 113 assertions | **594 assertions** (+426%) | PASS (594/594) |
| Real Drivers Linked on Host | 1 (`ring_buffer.c`) | **10 drivers** (`ring_buffer`, `gpio_g4`, + 8 `-DDRIVER_HOST_TEST` drivers) | PASS |
| Real `gcov -b` Aggregate Line Coverage (10 drivers) | Not measured | **96.04%** (267 / 278 lines; **9/10 drivers at 100.00%**) | MEASURED |
| Host Static Analysis (`gcc 15.2.0 -fanalyzer`) | Not configured | **0 warnings / 0 findings** (`-Wall -Wextra -Werror`) | PASS |
| ARM Static Analysis (`arm-none-eabi-gcc 14.3.1 -fanalyzer`) | Not configured | **0 warnings / 0 findings** (all 12 drivers + starter template) | PASS |
| ARM Cross-Compilation (`templates` + `freertos`) | Template only | **Template (debug/release) + FreeRTOS V11.1.0 + Capstone CI discovery** | PASS |
| Static Analysis & Doc Configs | `.clang-format` only | **`.clang-format`, `.clang-tidy`, `Doxyfile`, upgraded CI workflow** | CONFIGURED |
| Repository Validator (`scripts/validate_repo.py`) | 12 PASS | **12 PASS, 0 WARN, 0 FAIL** | PASS |

---

## 2. Host Unit Test Suite Breakdown (`make -C tests clean test`)

All 4 host suites compile cleanly with zero external dependencies under strict ISO C11 diagnostics:
`-std=c11 -O1 -g -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`

| # | Test Suite | Linked Production Source(s) | Assertions | Failed | Key Coverage Scope |
|---|---|---|---:|---:|---|
| 1 | [test_c_foundations.c](../tests/test_c_foundations.c) | Standalone C11 | **116** | 0 | Fixed-width limits, two's complement, `~uint8_t` integer promotions, signed/unsigned conversions, bitfields, BE/LE packing, `memcpy` strict-aliasing float/unaligned punning, struct padding & `offsetof`, tagged unions, function-pointer jump tables, Q15/Q31 saturating math, wraparound-safe 32-bit tick subtraction, non-blocking 4-state button debounce FSM |
| 2 | [test_ring_buffer.c](../tests/unit/test_ring_buffer.c) | `ring_buffer.c` | **104** | 0 | Power-of-2 init validation, 100% capacity utilization, FIFO ordering, peek semantics, wraparound bulk `rb_write`/`rb_read`, 32-bit `uint32_t` rollover (`0xFFFFFFFE -> 0x00000002`), NULL/zero-length defensive guards, overflow clamping |
| 3 | [test_drivers_math.c](../tests/unit/test_drivers_math.c) | `exti_g4.c`, `tim_g4.c`, `usart_g4.c`, `spi_g4.c`, `i2c_g4.c`, `adc_g4.c`, `dma_g4.c`, `iwdg_g4.c` (`-DDRIVER_HOST_TEST`) | **292** | 0 | Exhaustive EXTI pins 0–15 + invalid pin rejection; TIM PSC/ARR & PWM CCR at 170 MHz / 160 MHz / 16 MHz + `0xFFFF` & `0xFFFFFFFF` wraparound guards; USART & LPUART BRR at 170 / 160 / 16 MHz & 32768 Hz LSE; SPI `/2../256` exact ceiling boundaries at 170 & 160 MHz; I2C `TIMINGR` selection + NXP UM10204 `t_LOW`/`t_HIGH`/`t_SCLDEL` & max FM frequency checks; ADC mV, VREFINT VDDA, two-point factory `TS_CAL1`/`TS_CAL2` die temp interpolation + divide-by-zero guards; DMA1/2 ch 1–8 -> DMAMUX1 0–15 + RM0440 Table 91 Request IDs; IWDG prescaler/reload across 10 ms..32768 ms + DS12288 LSI +/-5% tolerance |
| 4 | [test_gpio_mock.c](../tests/unit/test_gpio_mock.c) | `gpio_g4.c` + fake CMSIS [stm32g474xx.h](../tests/mocks/stm32g474xx.h) | **82** | 0 | STM32G4 Analog reset defaults (`GPIOA=0xABFFFFFF`, `GPIOB=0xFFFFFEBF`, `GPIOC=0xFFFFFFFF`), `GPIO_EnableClock` ports A–G + NULL ignore, `MODER`/`OTYPER`/`OSPEEDR`/`PUPDR`/`AFRL`/`AFRH` bitfield isolation (SWD PA13/PA14 preservation), atomic `BSRR` set/reset/write/toggle hardware ODR model, `LCKR` 4-step sequence + 2nd-pin rejection on locked port, HardFault 8-word exception frame SRAM1+SRAM2 & CCM-SRAM bounds validator + Cortex-M4 SCB `CFSR`/`HFSR` flag decoding |
| | **Total** | **10 real driver `.c` modules** | **594** | **0** | **100% pass rate (exit code 0)** |

---

## 3. Real Code Coverage (`gcov -b`, `make -C tests clean coverage`)

Measured directly with MSYS2 `gcc 15.2.0` (`-O0 -g --coverage -fprofile-arcs -ftest-coverage`) and `gcov 15.2.0 -b` over the real driver source files in `23-code-library/Drivers/`:

| Driver Module | Scope Compiled on Host | Lines Executed | Branches Executed | Branches Taken >= 1x | Calls Executed |
|---|---|---:|---:|---:|---:|
| `ring_buffer.c` | Full driver | **100.00%** (93/93) | **100.00%** (68/68) | 98.53% (67/68) | 100.00% (24/24) |
| `gpio_g4.c` | Full driver (fake CMSIS regs) | **100.00%** (63/63) | **100.00%** (32/32) | 96.88% (31/32) | 100.00% (4/4) |
| `exti_g4.c` | `-DDRIVER_HOST_TEST` helper | **100.00%** (6/6) | **100.00%** (6/6) | 100.00% (6/6) | No calls |
| `tim_g4.c` | `-DDRIVER_HOST_TEST` helpers | **70.27%** (26/37) | **88.89%** (32/36) | 72.22% (26/36) | No calls |
| `usart_g4.c` | `-DDRIVER_HOST_TEST` helpers | **100.00%** (15/15) | **100.00%** (16/16) | 100.00% (16/16) | No calls |
| `spi_g4.c` | `-DDRIVER_HOST_TEST` helper | **100.00%** (8/8) | **100.00%** (8/8) | 100.00% (8/8) | No calls |
| `i2c_g4.c` | `-DDRIVER_HOST_TEST` helper | **100.00%** (16/16) | **100.00%** (16/16) | 100.00% (16/16) | No calls |
| `adc_g4.c` | `-DDRIVER_HOST_TEST` helpers | **100.00%** (17/17) | **100.00%** (6/6) | 100.00% (6/6) | No calls |
| `dma_g4.c` | `-DDRIVER_HOST_TEST` helper | **100.00%** (8/8) | **100.00%** (8/8) | 100.00% (8/8) | No calls |
| `iwdg_g4.c` | `-DDRIVER_HOST_TEST` helper | **100.00%** (15/15) | **100.00%** (14/14) | 100.00% (14/14) | No calls |
| **Aggregate (10 Drivers)** | **Host-testable driver code** | **96.04% (267/278)** | **98.10% (206/210)** | **93.33% (196/210)** | **100.00% (28/28)** |

### Coverage Gap Root-Cause Analysis (`tim_g4.c`: 11 Uncovered Lines)

Inspection of `tests/build/cov/tim_g4.c.gcov` confirms that all 11 uncovered lines in `tim_g4.c` are **mathematically unreachable defensive arithmetic guards** for valid `uint32_t` inputs (`1 <= target_hz <= clk_hz <= UINT32_MAX`):

1. `tim_calc_psc_arr()` (10 lines — lines 19, 25, 28, 33, 36–42):
   - Since `target_hz <= clk_hz <= 4,294,967,295`, `total_ticks = (clk_hz + target_hz/2) / target_hz` is strictly in `[1 .. 4,294,967,295]`.
   - Consequently, `psc_div = (total_ticks + 65535) / 65536` is strictly in `[1 .. 65536]`, and `arr_ticks = (total_ticks + psc_div/2) / psc_div` is strictly in `[1 .. 65536]`.
   - Thus the defensive clamps (`total_ticks == 0`, `psc_div == 0`, `psc_div > 65536`, `arr_ticks == 0`, `arr_ticks > 65536`) can never trigger without widening input types beyond `uint32_t`.
2. `tim_calc_pwm_ccr()` (1 line — line 63):
   - For `duty_permille <= 999` and `arr == 0xFFFF`, `((65536 * 999) + 500) / 1000 = 65470 <= 0xFFFF`, so the post-rounding `ccr > 0xFFFF` defensive clamp never triggers.

Excluding these 11 mathematically unreachable defensive guards, **100.00% (267/267) of reachable host-compiled driver lines are covered**.

> **Note on Hardware-Only Driver Code:** MMIO peripheral initialization and ISR register loops inside `#if !defined(DRIVER_HOST_TEST)` (as well as `clock_g4.c` PLL/PWR sequencing and `fault_handler.c` SCB/CoreDebug traps) cannot execute on x86_64 host without full silicon emulation; they are verified via strict `arm-none-eabi-gcc -Werror` cross-compilation and `arm-none-eabi-gcc -fanalyzer` static analysis.

---

## 4. Static Analysis Results (`gcc -fanalyzer`)

### 4.1 Host Static Analyzer (`make -C tests analyze`)

- **Tool:** `gcc.exe (Rev8, Built by MSYS2 project) 15.2.0`
- **Flags:** `-std=c11 -O1 -fanalyzer -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -fsyntax-only`
- **Files Analyzed:** All 4 host unit test suites (`test_c_foundations.c`, `test_ring_buffer.c`, `test_drivers_math.c`, `test_gpio_mock.c`) + all 10 host-compiled drivers (`ring_buffer.c`, `gpio_g4.c`, `exti_g4.c`, `tim_g4.c`, `usart_g4.c`, `spi_g4.c`, `i2c_g4.c`, `adc_g4.c`, `dma_g4.c`, `iwdg_g4.c`).
- **Result:** **PASS — 0 warnings, 0 findings (exit code 0).**

### 4.2 ARM Cortex-M4F Cross-Compiler Static Analyzer

- **Tool:** `arm-none-eabi-gcc.exe (GNU Tools for STM32 14.3.rel1) 14.3.1`
- **Flags:** `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -O1 -fanalyzer -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -DSTM32G474xx -fsyntax-only`
- **Files Analyzed:** All **12** STM32G474RE bare-metal drivers (`adc_g4.c`, `clock_g4.c`, `dma_g4.c`, `exti_g4.c`, `fault_handler.c`, `gpio_g4.c`, `i2c_g4.c`, `iwdg_g4.c`, `ring_buffer.c`, `spi_g4.c`, `tim_g4.c`, `usart_g4.c`) + `23-code-library/templates/Src/main.c` + `23-code-library/templates/Src/startup_stm32g474re.c`.
- **Result:** **PASS — 0 warnings, 0 findings (exit code 0).**

---

## 5. ARM Firmware Cross-Compilation Verification (`arm-none-eabi-gcc 14.3.1`)

| Firmware Project | Profile | Result | `.text` (B) | `.data` (B) | `.bss` (B) | Vector Table / Notes |
|---|---|---|---:|---:|---:|---|
| `23-code-library/templates` (starter + all 12 drivers) | `BUILD_TYPE=debug` | **PASS** (0 warnings) | — | — | — | Compiled cleanly with `-Og -g3 -Werror` |
| `23-code-library/templates` (starter + all 12 drivers) | `BUILD_TYPE=release` | **PASS** (0 warnings) | 2,620 | 12 | 1,700 | `.isr_vector` at `0x08000000`, size `0x1D8` (**472 B** = 118 entries), **512 B aligned** |
| `23-code-library/freertos` (FreeRTOS V11.1.0 + CMSIS) | `BUILD_TYPE=release` | **PASS** (0 warnings on app) | 21,596 | 96 | 35,640 | FLASH 4.14% (21,692 B / 512 KB), RAM 36.34% (35,736 B / 96 KB) |

---

## 6. Local Execution vs. GitHub Actions CI Tool Matrix

To maintain strict engineering honesty, the table below explicitly separates tools executed locally on this Windows 11 workstation from tools configured in [.github/workflows/firmware-ci.yml](../.github/workflows/firmware-ci.yml) for Ubuntu CI runners:

| Tool / Check | Local Windows 11 Status | GitHub Actions CI (`ubuntu-latest`) | Configuration Artifact |
|---|---|---|---|
| `gcc 15.2.0` (Host Unit Tests) | **EXECUTED LOCALLY — PASS (594/594)** | Configured (`host-unit-tests-and-coverage`) | [tests/Makefile](../tests/Makefile) |
| `gcov 15.2.0` (`gcov -b` Coverage) | **EXECUTED LOCALLY — PASS (96.04% lines)** | Configured (`host-unit-tests-and-coverage`) | [tests/Makefile](../tests/Makefile) |
| `gcc 15.2.0 -fanalyzer` (Host) | **EXECUTED LOCALLY — PASS (0 warnings)** | Configured (`host-unit-tests-and-coverage`) | [tests/Makefile](../tests/Makefile) |
| `arm-none-eabi-gcc 14.3.1` + `-fanalyzer` | **EXECUTED LOCALLY — PASS (0 warnings)** | Configured (`firmware-cross-compile`) | [23-code-library/templates/Makefile](../23-code-library/templates/Makefile) |
| `python scripts/validate_repo.py` | **EXECUTED LOCALLY — PASS (12/12)** | Configured (`repo-validation`) | [scripts/validate_repo.py](../scripts/validate_repo.py) |
| `lcov` (HTML/info coverage report) | Not installed locally on Windows PATH | Configured (`apt-get install lcov`, LCOV 2.0 `--rc branch_coverage=1 --ignore-errors inconsistent,gcov`) | [.github/workflows/firmware-ci.yml](../.github/workflows/firmware-ci.yml) |
| `cppcheck` (C11 static analyzer) | Not installed locally on Windows PATH | Configured (`--enable=warning,performance,portability --error-exitcode=1`) | [.github/workflows/firmware-ci.yml](../.github/workflows/firmware-ci.yml) |
| `clang-tidy` (MISRA/CERT/bugprone) | Not installed locally on Windows PATH | Configured (`static-analysis-and-format`) | [.clang-tidy](../.clang-tidy) |
| `clang-format --dry-run` (advisory diff report) | Not installed locally on Windows PATH | Configured (`static-analysis-and-format`, `continue-on-error: true`) | [.clang-format](../.clang-format) |
| `doxygen` (`WARN_AS_ERROR = YES`) | Not installed locally on Windows PATH | Configured (`doxygen-docs`) | [Doxyfile](../Doxyfile) |
| Physical NUCLEO-G474RE HIL Bench | **Not performed (no hardware attached)** | **Excluded from cloud CI by design** | Lab bench checklists in `18-practical-laboratory/` |

---

## 7. CI Workflow Architecture ([firmware-ci.yml](../.github/workflows/firmware-ci.yml))

The upgraded GitHub Actions pipeline runs 5 parallel jobs on pushes (`main`, `master`, `feature/*`), pull requests, and manual `workflow_dispatch`:

1. **`host-unit-tests-and-coverage`**: Runs `make -C tests clean test`, `make -C tests analyze`, `make -C tests clean coverage`, captures `lcov` branch coverage (`coverage.info`), and uploads `.gcov`/`coverage.info` artifacts.
2. **`firmware-cross-compile`**: Cross-compiles `23-code-library/templates` (debug + release, all 12 drivers), validates `.isr_vector` (`0x08000000`, 472 B, 512-byte aligned), runs `arm-none-eabi-gcc -fanalyzer`, builds `23-code-library/freertos` (debug + `nm-check` + release), and dynamically discovers & builds all Capstone Makefiles under `19-projects/`.
3. **`static-analysis-and-format`**: Executes `cppcheck` (`--enable=warning,performance,portability --error-exitcode=1`), `clang-tidy` using root [.clang-tidy](../.clang-tidy), and advisory `clang-format --dry-run` (`continue-on-error: true`) using root [.clang-format](../.clang-format).
4. **`doxygen-docs`**: Runs `doxygen Doxyfile` over `23-code-library/Drivers/` and `23-code-library/templates/Src/` (excluding vendor `CMSIS/` and `system_stm32g4xx.c`) with `WARN_AS_ERROR = YES`, uploading HTML docs as an artifact.
5. **`repo-validation`**: Runs `python3 scripts/validate_repo.py --skip-builds` to enforce directory structure, control-character cleanliness, internal links, Mermaid syntax, Source-of-Truth concepts, STM32G474RE register invariants, curriculum completeness, and video database integrity.
