# Audit Round 2 — Exact Validation Commands & Tool Outputs

> **Date:** 2026-11-18  
> **Branch:** `feature/complete-remediation`  
> **Repository:** `D:\General_ant\embedded-c-master-learning-hub`  
> **Validator Version:** [`scripts/validate_repo.py`](../scripts/validate_repo.py) **`v4.2.0`**  
> **Companion Reports:** [`AUDIT/ROUND_2_FINDINGS.md`](ROUND_2_FINDINGS.md) | [`AUDIT/ROUND_2_REMEDIATION_REPORT.md`](ROUND_2_REMEDIATION_REPORT.md) | [`AUDIT/MASTER_REMEDIATION_TRACKER.md`](MASTER_REMEDIATION_TRACKER.md)  
> **Verification Honesty Boundary:** All Round 2 builds and tests below were executed live on host (`arm-none-eabi-gcc 14.3.1`, `gcc 15.2.0`, `Python 3.14.6`, `GNU Make 4.4.1`). Firmware artifacts are **Compile-Verified (Not Tested on Physical Hardware in Round 2)**; STM32CubeG4 LL/HAL implementations are **Statically Reviewed (STM32CubeG4 HAL/LL package not installed on host)**; Mermaid visual rendering is **`UNVERIFIED` (`mmdc` unavailable on host)**.

---

## 1. Validator v4.2.0 Summary Matrix (All 3 Modes)

| Mode | Exact Command | `INFO/PASS` | `WARNING` | `UNVERIFIED` | `ERROR` | Exit Code |
|---|---|---:|---:|---:|---:|---:|
| **Mode 1 (`--skip-builds`)** | `python scripts/validate_repo.py --skip-builds` | `23` | `1` | `11` | **`0`** | `0` |
| **Mode 2 (Default)** | `python scripts/validate_repo.py` | `32` | `1` | `2` | **`0`** | `0` |
| **Mode 3 (Live External URLs)** | `python scripts/validate_repo.py --check-external-urls` | `33` | `1` | `1` | **`0`** | `0` |

---

## 2. Mode 1 Exact Output: `python scripts/validate_repo.py --skip-builds`

```text
=== Embedded C & STM32G474RE Master Learning Hub Validator v4.2.0 ===
Repository: D:\General_ant\embedded-c-master-learning-hub
[INFO/PASS] [structure] All 34 required directories present.
[INFO/PASS] [structure] All 14 onboarding/governance/tooling files present.
[INFO/PASS] [structure] All 29 mandatory AUDIT files present.
[INFO/PASS] [structure] Repository hygiene clean: no stale scratch/ directory.
[INFO/PASS] [control_chars] Zero raw ASCII control characters across 145 Markdown/Mermaid files.
[INFO/PASS] [cmsis_registers] Parsed CMSIS headers (stm32g474xx.h + core_cm4.h): 171 base macros, 57 typedef structs, 4082 _Pos / 3916 _Msk bitfields, 110 IRQns (max FMAC_IRQn=101).
[INFO/PASS] [cmsis_registers] CMSIS ground-truth cross-check passed (16/16 maps cite RM0440 or PM0214/ARMv7-M): verified 87 bases, 344 struct offsets, 114 bit positions across 16 maps — scope limited to parsed table entries, bullet bit positions & CMSIS C snippets, not every RM0440 reset value or unparsed prose field (0 mismatches).
[INFO/PASS] [sot_dag] Source-of-Truth v2.1.0 DAG verified: 350 unique IDs, 27/27 domains, 0 cycles, 0 unknown prereqs, 0 inverted-level edges.
[INFO/PASS] [sot_evidence] All 265 COMPLETE concepts verified with substantive repository evidence and metadata fields (74 PARTIAL concepts preserved untouched; never auto-promoted).
[WARNING] [sot_manual_review] WARNING (MANUAL REVIEW RECOMMENDED): 88 COMPLETE concept(s) flagged for manual review (18 hardware_status in EXTERNAL_HARDWARE_PENDING/NEEDS_BENCH_INSTRUMENT, 72 lacking an explicit lab/driver/test path in practical_implementation; 2 flagged for both).
    - CS-ALGO-001: practical_implementation lacks explicit lab/driver/test path
    - C-ARR-003: practical_implementation lacks explicit lab/driver/test path
    - C-ARR-004: practical_implementation lacks explicit lab/driver/test path
    - C-FPTR-002: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-001: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-002: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-003: practical_implementation lacks explicit lab/driver/test path
    - C-MEM-002: practical_implementation lacks explicit lab/driver/test path
    - C-MEM-003: practical_implementation lacks explicit lab/driver/test path
    - C-OPT-002: practical_implementation lacks explicit lab/driver/test path
    - C-PREP-003: practical_implementation lacks explicit lab/driver/test path
    - C-PTR-004: practical_implementation lacks explicit lab/driver/test path
    ... (+76 more)
[INFO/PASS] [labs] All 20/20 practical labs verified with all 11 core sections + Clock Profile Declaration + Standardized Verification Record.
[INFO/PASS] [labs_prerequisites] All 20/20 practical labs include an explicit Prerequisites heading or bold field.
[INFO/PASS] [links] All 1192 internal inline links verified across 133 .md files.
[INFO/PASS] [anchors] All 12 internal Markdown #anchor fragments verified against GFM headings (0 broken).
[INFO/PASS] [images] All 0 local image references verified (0 broken).
[INFO/PASS] [ref_links] Reference-style links verified: 0 uses, 0 definitions (0 broken).
[INFO/PASS] [mathjax] Expanded GitHub MathJax safety passed across all 133 .md files (0 violations).
[UNVERIFIED] [external_urls] 70 unique external URLs UNVERIFIED (pass --check-external-urls to probe live).
[INFO/PASS] [mermaid_syntax] All 12 Mermaid (.mmd) diagrams passed static structural syntax validation.
[UNVERIFIED] [mermaid_render] Mermaid visual rendering NOT TESTED — 'mmdc' (@mermaid-js/mermaid-cli) tooling unavailable on host.
[INFO/PASS] [curriculum] All 17/17 curriculum stage modules verified (13/13 sections + authoritative primary references).
[INFO/PASS] [curriculum_nav] All 17/17 curriculum stages include Prev/Next navigation links.
[INFO/PASS] [drivers] All 12/12 bare-metal drivers (.h/.c) verified in 23-code-library/Drivers/.
[INFO/PASS] [freertos] Pinned FreeRTOS-Kernel V11.1.0 project structure verified.
[INFO/PASS] [capstones] All 3/3 Capstone firmware projects verified in 19-projects/.
[INFO/PASS] [video_db] Video database CSV schema verified: 70 entries (>=70), 12/12 exact columns, 70 unique IDs, 70 valid unique HTTPS URLs, 0 duplicate YouTube IDs.
[UNVERIFIED] [register_snippets] BUILD_SKIPPED: Register-map C snippet compilation skipped via --skip-builds.
[UNVERIFIED] [firmware_build_template_debug] BUILD_SKIPPED: Starter template + 12 drivers (BUILD_TYPE=debug) skipped via --skip-builds.
[UNVERIFIED] [firmware_build_template_release] BUILD_SKIPPED: Starter template + 12 drivers (BUILD_TYPE=release) skipped via --skip-builds.
[UNVERIFIED] [firmware_build_freertos] BUILD_SKIPPED: FreeRTOS V11.1.0 firmware (23-code-library/freertos/) skipped via --skip-builds.
[UNVERIFIED] [firmware_build_capstone_01] BUILD_SKIPPED: Capstone 01 (19-projects/01-beginner-smart-instrument-panel) skipped via --skip-builds.
[UNVERIFIED] [firmware_build_capstone_02] BUILD_SKIPPED: Capstone 02 (19-projects/02-intermediate-dma-data-logger) skipped via --skip-builds.
[UNVERIFIED] [firmware_build_capstone_03] BUILD_SKIPPED: Capstone 03 (19-projects/03-advanced-freertos-digital-power-controller) skipped via --skip-builds.
[UNVERIFIED] [firmware_elf] ELF_INSPECTION_UNAVAILABLE: ELF .isr_vector inspection skipped via --skip-builds.
[UNVERIFIED] [host_tests] HOST_TESTS_SKIPPED: Host C unit tests skipped via --skip-builds.
=== Validation Summary: 23 INFO/PASS | 1 WARNING | 11 UNVERIFIED | 0 ERROR ===
```

---

## 3. Mode 2 Exact Output: `python scripts/validate_repo.py`

```text
=== Embedded C & STM32G474RE Master Learning Hub Validator v4.2.0 ===
Repository: D:\General_ant\embedded-c-master-learning-hub
[INFO/PASS] [structure] All 34 required directories present.
[INFO/PASS] [structure] All 14 onboarding/governance/tooling files present.
[INFO/PASS] [structure] All 29 mandatory AUDIT files present.
[INFO/PASS] [structure] Repository hygiene clean: no stale scratch/ directory.
[INFO/PASS] [control_chars] Zero raw ASCII control characters across 145 Markdown/Mermaid files.
[INFO/PASS] [cmsis_registers] Parsed CMSIS headers (stm32g474xx.h + core_cm4.h): 171 base macros, 57 typedef structs, 4082 _Pos / 3916 _Msk bitfields, 110 IRQns (max FMAC_IRQn=101).
[INFO/PASS] [cmsis_registers] CMSIS ground-truth cross-check passed (16/16 maps cite RM0440 or PM0214/ARMv7-M): verified 87 bases, 344 struct offsets, 114 bit positions across 16 maps — scope limited to parsed table entries, bullet bit positions & CMSIS C snippets, not every RM0440 reset value or unparsed prose field (0 mismatches).
[INFO/PASS] [sot_dag] Source-of-Truth v2.1.0 DAG verified: 350 unique IDs, 27/27 domains, 0 cycles, 0 unknown prereqs, 0 inverted-level edges.
[INFO/PASS] [sot_evidence] All 265 COMPLETE concepts verified with substantive repository evidence and metadata fields (74 PARTIAL concepts preserved untouched; never auto-promoted).
[WARNING] [sot_manual_review] WARNING (MANUAL REVIEW RECOMMENDED): 88 COMPLETE concept(s) flagged for manual review (18 hardware_status in EXTERNAL_HARDWARE_PENDING/NEEDS_BENCH_INSTRUMENT, 72 lacking an explicit lab/driver/test path in practical_implementation; 2 flagged for both).
    - CS-ALGO-001: practical_implementation lacks explicit lab/driver/test path
    - C-ARR-003: practical_implementation lacks explicit lab/driver/test path
    - C-ARR-004: practical_implementation lacks explicit lab/driver/test path
    - C-FPTR-002: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-001: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-002: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-003: practical_implementation lacks explicit lab/driver/test path
    - C-MEM-002: practical_implementation lacks explicit lab/driver/test path
    - C-MEM-003: practical_implementation lacks explicit lab/driver/test path
    - C-OPT-002: practical_implementation lacks explicit lab/driver/test path
    - C-PREP-003: practical_implementation lacks explicit lab/driver/test path
    - C-PTR-004: practical_implementation lacks explicit lab/driver/test path
    ... (+76 more)
[INFO/PASS] [labs] All 20/20 practical labs verified with all 11 core sections + Clock Profile Declaration + Standardized Verification Record.
[INFO/PASS] [labs_prerequisites] All 20/20 practical labs include an explicit Prerequisites heading or bold field.
[INFO/PASS] [links] All 1192 internal inline links verified across 133 .md files.
[INFO/PASS] [anchors] All 12 internal Markdown #anchor fragments verified against GFM headings (0 broken).
[INFO/PASS] [images] All 0 local image references verified (0 broken).
[INFO/PASS] [ref_links] Reference-style links verified: 0 uses, 0 definitions (0 broken).
[INFO/PASS] [mathjax] Expanded GitHub MathJax safety passed across all 133 .md files (0 violations).
[UNVERIFIED] [external_urls] 70 unique external URLs UNVERIFIED (pass --check-external-urls to probe live).
[INFO/PASS] [mermaid_syntax] All 12 Mermaid (.mmd) diagrams passed static structural syntax validation.
[UNVERIFIED] [mermaid_render] Mermaid visual rendering NOT TESTED — 'mmdc' (@mermaid-js/mermaid-cli) tooling unavailable on host.
[INFO/PASS] [curriculum] All 17/17 curriculum stage modules verified (13/13 sections + authoritative primary references).
[INFO/PASS] [curriculum_nav] All 17/17 curriculum stages include Prev/Next navigation links.
[INFO/PASS] [drivers] All 12/12 bare-metal drivers (.h/.c) verified in 23-code-library/Drivers/.
[INFO/PASS] [freertos] Pinned FreeRTOS-Kernel V11.1.0 project structure verified.
[INFO/PASS] [capstones] All 3/3 Capstone firmware projects verified in 19-projects/.
[INFO/PASS] [video_db] Video database CSV schema verified: 70 entries (>=70), 12/12 exact columns, 70 unique IDs, 70 valid unique HTTPS URLs, 0 duplicate YouTube IDs.
[INFO/PASS] [register_snippets] BUILD_PASSED: All 16/16 standalone CMSIS C snippets in 20-technical-diagrams/registers/*.md syntax-compiled cleanly (-fsyntax-only -Wall -Wextra -Werror; 3 prose fragment(s) skipped).
[INFO/PASS] [firmware_build_template_debug] BUILD_PASSED: Starter template + 12 drivers (BUILD_TYPE=debug) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_template_release] BUILD_PASSED: Starter template + 12 drivers (BUILD_TYPE=release) compiled cleanly (exit 0).
[INFO/PASS] [firmware_elf] ELF_INSPECTION_PASSED: ELF .isr_vector verified: addr=0x08000000, size=472 B (118 vectors), align=512 B.
[INFO/PASS] [firmware_build_freertos] BUILD_PASSED: FreeRTOS V11.1.0 firmware (23-code-library/freertos/) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_capstone_01] BUILD_PASSED: Capstone 01 (19-projects/01-beginner-smart-instrument-panel) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_capstone_02] BUILD_PASSED: Capstone 02 (19-projects/02-intermediate-dma-data-logger) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_capstone_03] BUILD_PASSED: Capstone 03 (19-projects/03-advanced-freertos-digital-power-controller) compiled cleanly (exit 0).
[INFO/PASS] [host_tests] HOST_TESTS_PASSED: 4 suites, 594 assertions executed, 0 failed.
=== Validation Summary: 32 INFO/PASS | 1 WARNING | 2 UNVERIFIED | 0 ERROR ===
```

---

## 4. Mode 3 Exact Output: `python scripts/validate_repo.py --check-external-urls`

```text
=== Embedded C & STM32G474RE Master Learning Hub Validator v4.2.0 ===
Repository: D:\General_ant\embedded-c-master-learning-hub
[INFO/PASS] [structure] All 34 required directories present.
[INFO/PASS] [structure] All 14 onboarding/governance/tooling files present.
[INFO/PASS] [structure] All 29 mandatory AUDIT files present.
[INFO/PASS] [structure] Repository hygiene clean: no stale scratch/ directory.
[INFO/PASS] [control_chars] Zero raw ASCII control characters across 145 Markdown/Mermaid files.
[INFO/PASS] [cmsis_registers] Parsed CMSIS headers (stm32g474xx.h + core_cm4.h): 171 base macros, 57 typedef structs, 4082 _Pos / 3916 _Msk bitfields, 110 IRQns (max FMAC_IRQn=101).
[INFO/PASS] [cmsis_registers] CMSIS ground-truth cross-check passed (16/16 maps cite RM0440 or PM0214/ARMv7-M): verified 87 bases, 344 struct offsets, 114 bit positions across 16 maps — scope limited to parsed table entries, bullet bit positions & CMSIS C snippets, not every RM0440 reset value or unparsed prose field (0 mismatches).
[INFO/PASS] [sot_dag] Source-of-Truth v2.1.0 DAG verified: 350 unique IDs, 27/27 domains, 0 cycles, 0 unknown prereqs, 0 inverted-level edges.
[INFO/PASS] [sot_evidence] All 265 COMPLETE concepts verified with substantive repository evidence and metadata fields (74 PARTIAL concepts preserved untouched; never auto-promoted).
[WARNING] [sot_manual_review] WARNING (MANUAL REVIEW RECOMMENDED): 88 COMPLETE concept(s) flagged for manual review (18 hardware_status in EXTERNAL_HARDWARE_PENDING/NEEDS_BENCH_INSTRUMENT, 72 lacking an explicit lab/driver/test path in practical_implementation; 2 flagged for both).
    - CS-ALGO-001: practical_implementation lacks explicit lab/driver/test path
    - C-ARR-003: practical_implementation lacks explicit lab/driver/test path
    - C-ARR-004: practical_implementation lacks explicit lab/driver/test path
    - C-FPTR-002: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-001: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-002: practical_implementation lacks explicit lab/driver/test path
    - C-FUNC-003: practical_implementation lacks explicit lab/driver/test path
    - C-MEM-002: practical_implementation lacks explicit lab/driver/test path
    - C-MEM-003: practical_implementation lacks explicit lab/driver/test path
    - C-OPT-002: practical_implementation lacks explicit lab/driver/test path
    - C-PREP-003: practical_implementation lacks explicit lab/driver/test path
    - C-PTR-004: practical_implementation lacks explicit lab/driver/test path
    ... (+76 more)
[INFO/PASS] [labs] All 20/20 practical labs verified with all 11 core sections + Clock Profile Declaration + Standardized Verification Record.
[INFO/PASS] [labs_prerequisites] All 20/20 practical labs include an explicit Prerequisites heading or bold field.
[INFO/PASS] [links] All 1192 internal inline links verified across 133 .md files.
[INFO/PASS] [anchors] All 12 internal Markdown #anchor fragments verified against GFM headings (0 broken).
[INFO/PASS] [images] All 0 local image references verified (0 broken).
[INFO/PASS] [ref_links] Reference-style links verified: 0 uses, 0 definitions (0 broken).
[INFO/PASS] [mathjax] Expanded GitHub MathJax safety passed across all 133 .md files (0 violations).
[INFO/PASS] [external_urls] Live external URL probe (70 URLs): 70 accessible (2xx), 0 redirected (3xx -> final URL), 0 bot-blocked (403/429), 0 broken (404/410), 0 network/DNS/timeout failures.
[INFO/PASS] [mermaid_syntax] All 12 Mermaid (.mmd) diagrams passed static structural syntax validation.
[UNVERIFIED] [mermaid_render] Mermaid visual rendering NOT TESTED — 'mmdc' (@mermaid-js/mermaid-cli) tooling unavailable on host.
[INFO/PASS] [curriculum] All 17/17 curriculum stage modules verified (13/13 sections + authoritative primary references).
[INFO/PASS] [curriculum_nav] All 17/17 curriculum stages include Prev/Next navigation links.
[INFO/PASS] [drivers] All 12/12 bare-metal drivers (.h/.c) verified in 23-code-library/Drivers/.
[INFO/PASS] [freertos] Pinned FreeRTOS-Kernel V11.1.0 project structure verified.
[INFO/PASS] [capstones] All 3/3 Capstone firmware projects verified in 19-projects/.
[INFO/PASS] [video_db] Video database CSV schema verified: 70 entries (>=70), 12/12 exact columns, 70 unique IDs, 70 valid unique HTTPS URLs, 0 duplicate YouTube IDs.
[INFO/PASS] [register_snippets] BUILD_PASSED: All 16/16 standalone CMSIS C snippets in 20-technical-diagrams/registers/*.md syntax-compiled cleanly (-fsyntax-only -Wall -Wextra -Werror; 3 prose fragment(s) skipped).
[INFO/PASS] [firmware_build_template_debug] BUILD_PASSED: Starter template + 12 drivers (BUILD_TYPE=debug) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_template_release] BUILD_PASSED: Starter template + 12 drivers (BUILD_TYPE=release) compiled cleanly (exit 0).
[INFO/PASS] [firmware_elf] ELF_INSPECTION_PASSED: ELF .isr_vector verified: addr=0x08000000, size=472 B (118 vectors), align=512 B.
[INFO/PASS] [firmware_build_freertos] BUILD_PASSED: FreeRTOS V11.1.0 firmware (23-code-library/freertos/) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_capstone_01] BUILD_PASSED: Capstone 01 (19-projects/01-beginner-smart-instrument-panel) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_capstone_02] BUILD_PASSED: Capstone 02 (19-projects/02-intermediate-dma-data-logger) compiled cleanly (exit 0).
[INFO/PASS] [firmware_build_capstone_03] BUILD_PASSED: Capstone 03 (19-projects/03-advanced-freertos-digital-power-controller) compiled cleanly (exit 0).
[INFO/PASS] [host_tests] HOST_TESTS_PASSED: 4 suites, 594 assertions executed, 0 failed.
=== Validation Summary: 33 INFO/PASS | 1 WARNING | 1 UNVERIFIED | 0 ERROR ===
```

---

## 5. Host Unit Tests (`594/594`) & Static Analysis (`0` Warnings)

### 5.1 `make -C tests clean test`

```text
--- [1/4] Executing Stage 01-03 Embedded C Foundations Tests ---
./build/test_c_foundations.exe
Running Stage 01-03 Embedded C Foundations Unit Tests...
Results: 116 assertions executed, 0 failed.
--- [2/4] Executing Ring Buffer Driver Unit Tests ---
./build/test_ring_buffer.exe
Running Ring Buffer Host Unit Tests...
Results: 104 assertions executed, 0 failed.
--- [3/4] Executing 8-Driver Pure Helper Math Tests (-DDRIVER_HOST_TEST) ---
./build/test_drivers_math.exe
Running STM32G474RE 8-Driver Pure Helper Unit Tests (-DDRIVER_HOST_TEST)...
Results: 292 assertions executed, 0 failed.
--- [4/4] Executing GPIO Register Mock & Fault Bounds Tests ---
./build/test_gpio_mock.exe
Running STM32G474RE GPIO Mock & Fault Bounds Unit Tests...
Results: 82 assertions executed, 0 failed.
[ALL 4 HOST UNIT TEST SUITES PASSED]
```
- **Total Host Assertions:** **`594 / 594` passed (`0` failed)** (`116 + 104 + 292 + 82`).

### 5.2 Host GCC `-fanalyzer` (`make -C tests analyze`)

```text
--- Running GCC -fanalyzer Static Analysis ---
gcc -std=c11 -O1 -fanalyzer -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -I../23-code-library/Drivers -fsyntax-only test_c_foundations.c
gcc -std=c11 -O1 -fanalyzer -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -I../23-code-library/Drivers -fsyntax-only unit/test_ring_buffer.c ../23-code-library/Drivers/ring_buffer.c
gcc -std=c11 -O1 -fanalyzer -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -I../23-code-library/Drivers -fsyntax-only -DDRIVER_HOST_TEST unit/test_drivers_math.c ../23-code-library/Drivers/exti_g4.c ../23-code-library/Drivers/tim_g4.c ../23-code-library/Drivers/usart_g4.c ../23-code-library/Drivers/spi_g4.c ../23-code-library/Drivers/i2c_g4.c ../23-code-library/Drivers/adc_g4.c ../23-code-library/Drivers/dma_g4.c ../23-code-library/Drivers/iwdg_g4.c
gcc -std=c11 -O1 -fanalyzer -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef -I../23-code-library/Drivers -fsyntax-only -Imocks unit/test_gpio_mock.c ../23-code-library/Drivers/gpio_g4.c
[GCC -FANALYZER STATIC ANALYSIS PASSED: 0 WARNINGS]
```

### 5.3 ARM Cross-Compiler `-fanalyzer` (All 12 Drivers + Template `main.c`)

```bash
arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -DSTM32G474xx -O2 -fanalyzer -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion \
  -Wdouble-promotion -Wundef \
  -I23-code-library/Drivers \
  -isystem 23-code-library/Drivers/CMSIS/Core/Include \
  -isystem 23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include \
  -I23-code-library/templates/Inc -fsyntax-only \
  23-code-library/Drivers/{clock,gpio,ring_buffer,fault_handler,exti,tim,usart,spi,i2c,adc,dma,iwdg}_g4.c \
  23-code-library/templates/Src/main.c
```
- **Result:** **`EXIT=0` (`0` warnings, `0` errors)** across all 12 bare-metal drivers + starter template `main.c`.

---

## 6. ARM Cortex-M4F Firmware Builds (`arm-none-eabi-gcc 14.3.1`)

All 6 firmware targets compiled and linked cleanly with `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`:

| Firmware Target | Exact Build Command | FLASH Used | RAM Used | CCMRAM Used | `text / data / bss` | Status |
|---|---|---:|---:|---:|---|---|
| **Starter Template (`debug`)** | `make -C 23-code-library/templates clean all BUILD_TYPE=debug` | `2,768 B` (`0.53%`) | `1,704 B` (`1.73%`) | `12 B` (`0.04%`) | `2756 / 12 / 1700` | **PASS (`0` warn)** |
| **Starter Template (`release`)** | `make -C 23-code-library/templates clean all BUILD_TYPE=release` | `2,632 B` (`0.50%`) | `1,704 B` (`1.73%`) | `12 B` (`0.04%`) | `2620 / 12 / 1700` | **PASS (`0` warn)** |
| **FreeRTOS V11.1.0 BSP** | `make -C 23-code-library/freertos clean all` | `21,592 B` (`4.12%`) | `35,728 B` (`36.34%`) | `0 B` (`0.00%`) | `21488 / 96 / 35640` | **PASS (`0` warn)** |
| **Capstone 01 — Instrument Panel** | `make -C 19-projects/01-beginner-smart-instrument-panel clean all` | `12,724 B` (`2.43%`) | `2,232 B` (`2.27%`) | `0 B` (`0.00%`) | `12624 / 92 / 2148` | **PASS (`0` warn)** |
| **Capstone 02 — DMA Data Logger** | `make -C 19-projects/02-intermediate-dma-data-logger clean all` | `11,784 B` (`2.25%`) | `2,296 B` (`2.34%`) | `0 B` (`0.00%`) | `11684 / 92 / 2208` | **PASS (`0` warn)** |
| **Capstone 03 — Digital Power Ctrl** | `make -C 19-projects/03-advanced-freertos-digital-power-controller clean all` | `21,424 B` (`4.09%`) | `31,552 B` (`32.10%`) | `0 B` (`0.00%`) | `21316 / 100 / 31460` | **PASS (`0` warn)** |

---

## 7. Practical Laboratories (`18/18` Bare-Metal Cross-Compiles + `Lab 20` Host + Honest `Lab 05` LL/HAL Note)

- **ARM GCC Bare-Metal Lab Compilation (`arm-none-eabi-gcc 14.3.1`, `-std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c`):**
  - **`18 / 18` bare-metal STM32 labs (`Labs 01–14, 16–19`) PASS (`0` warnings / `0` errors):**
    - `lab-01` (`960 B`), `lab-02` (`1,288 B`), `lab-03` (`1,416 B`), `lab-04` (`1,520 B`), `lab-05` Approach A (`1,304 B`), `lab-06` (`1,524 B`), `lab-07` (`1,656 B`), `lab-08` (`2,148 B`), `lab-09` (`1,612 B`), `lab-10` (`2,208 B`), `lab-11` (`1,536 B`), `lab-12` (`2,284 B`), `lab-13` (`1,388 B`), `lab-14` (`1,852 B`), `lab-16` (`1,848 B`), `lab-17` (`2,748 B`), `lab-18` (`2,116 B`), `lab-19` (`1,916 B`).
- **Lab 15 (FreeRTOS Multitask Producer-Consumer):** Backed by pinned `FreeRTOS-Kernel V11.1.0` build (`23-code-library/freertos/`, `PASS`).
- **Lab 20 (Host-Based Unit Testing):** `gcc -std=c11 -Wall -Wextra -Werror -Wconversion -O2` -> **PASS (`exit 0`, `[PASS] All embedded host unit tests passed (0 failures).`)**.
- **Honest `Lab 05` (and `Lab 01` / `Lab 08`) LL/HAL Verification Note:** Local host search confirmed **STM32CubeG4 HAL/LL headers (`stm32g4xx_hal.h`, `stm32g4xx_ll_exti.h`, `stm32g4xx_ll_gpio.h`) are NOT installed on this host (`0` files found)**. Approaches B (LL) and C (HAL) are therefore **Statically Reviewed against official `STMicroelectronics/stm32g4xx-hal-driver` API** and explicitly labeled as requiring an STM32CubeG4 environment to compile.

---

## 8. Mermaid Diagram Verification Honesty

- **Static Structural Syntax (`mermaid_syntax`):** **`12 / 12` `.mmd` diagrams PASS** (`INFO/PASS`).
- **Visual SVG Rendering (`mermaid_render`):** **`UNVERIFIED` — `'mmdc'` (`@mermaid-js/mermaid-cli`) tooling unavailable on host** (`shutil.which("mmdc") == None`). Tested under mocked `mmdc` in synthetic regression `REG-18` (`INFO/PASS` when present).
