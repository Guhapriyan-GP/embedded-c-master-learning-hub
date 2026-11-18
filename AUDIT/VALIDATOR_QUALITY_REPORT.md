# Validator Quality & Regression-Resistance Report (`scripts/validate_repo.py` v5.0.0)

**Date:** 2026-11-18  
**Branch:** `feature/complete-remediation`  
**Validator Artifact:** [`scripts/validate_repo.py`](../scripts/validate_repo.py) (upgraded `v3.0.0` → `v4.0.0` → `v4.1.0` → `v4.2.0` → **`v5.0.0`**, implementing all Section 11 quality, integrity, and regression-resistance requirements)  
**Target Platform Ground Truth:** `stm32g474xx.h` + `core_cm4.h` (STM32G474RET6 / ARM Cortex-M4F r0p1 / NUCLEO-G474RE MB1367)  
**Regression Test Artifact:** [`tests/test_validator_regressions.py`](../tests/test_validator_regressions.py) (**12/12 defect classes caught with ERROR**)  

---

## 1. Environment & Toolchain Versions

| Tool | Exact Version | Role in Validation |
|---|---|---|
| Python | `3.14.6` | Executes `scripts/validate_repo.py` v5.0.0 & `tests/test_validator_regressions.py` |
| ARM Cross-Compiler | `arm-none-eabi-gcc.exe (GNU Tools for STM32 14.3.rel1.20251027-0700) 14.3.1 20250623` | Compiles starter template (`debug` & `release`), FreeRTOS V11.1.0, 3 Capstones, and 16 register-map CMSIS C snippets |
| ELF Inspector | `arm-none-eabi-readelf` (GNU Binutils) | Verifies `.isr_vector` (`0x08000000`, `472 B`, `512 B` alignment) |
| Host C Compiler | `gcc.exe (Rev8, Built by MSYS2 project) 15.2.0` | Compiles and executes 5 host C11 unit-test suites (`873` assertions) |
| Build Tool | `GNU Make 4.4.1_st_20260330-0700` | Orchestrates firmware and host test Makefiles |
| Mermaid CLI (`mmdc`) | Pinned `@mermaid-js/mermaid-cli@11.4.2` in Ubuntu CI | Actively probed at runtime; `mermaid_render` honestly reports `UNVERIFIED` on host |

---

## 2. Validator v5.0.0 Architectural Enhancements

Building upon the 10 Section 2.C verification pillars and Round-2 Section 4 hardening, **Validator v5.0.0** introduces four major verification capabilities:

### 2.1 Release Metrics Single Source of Truth Gate (`check_release_metrics`)
- Validates [`AUDIT/release_metrics.json`](release_metrics.json) as the authoritative, canonical release scorecard.
- Enforces strict synchronization between `release_metrics.json` and [`AUDIT/concepts_sot.json`](concepts_sot.json):
  - Total concepts (`350`)
  - Status counts (`265 COMPLETE`, `74 PARTIAL`, `0 MISSING`, `5 NEEDS_VERIFICATION`, `6 NOT_APPLICABLE`)
- Enforces version synchronization between `release_metrics.json`, [`README.md`](../README.md), and [`START_HERE.md`](../START_HERE.md).

### 2.2 Register Reset Value & Access Type Cross-Check (Pass D in `check_cmsis_registers`)
- Extends the real CMSIS parser to parse Register Overview Markdown tables across all 16 peripheral register maps.
- Validates that every register row contains:
  1. A valid hexadecimal byte offset (`0x000`–`0x3FC`).
  2. A recognized hardware reset value (hex pattern `0x...` or architectural state: `Boot-dependent`, `Option Byte`, `Factory Trim`, `Unknown`, `Impl-defined`, etc.).
  3. A valid access type qualifier (`RW`, `RO`, `WO`, `rc_w1`, `rc_w0`, `R/W`, `R`, `W`, `t`).
- Verified **310 register access types** and **222 register reset values** across all 16 maps with **0 mismatches**.

### 2.3 Dual Clock-Profile Consistency Gate (`check_dual_clock_profiles`)
- Verifies that both the **160 MHz** (Profile B, clean integer) and **170 MHz** (Profile A, max boost) clock profiles are documented and preserved across:
  - [`07-rcc-and-clocks/README.md`](../07-rcc-and-clocks/README.md)
  - [`20-technical-diagrams/registers/rcc-register-map.md`](../20-technical-diagrams/registers/rcc-register-map.md)
  - [`23-code-library/Drivers/clock_g4.h`](../23-code-library/Drivers/clock_g4.h)
  - [`AUDIT/release_metrics.json`](release_metrics.json)
- Validates that both profiles correctly specify Range 1 Boost (`PWR_CR5.R1MODE = 0`), Flash 4 Wait States, and the mandatory RM0440 §6.1.5 `HPRE=/2` intermediate frequency ramp.

### 2.4 Public Sanitization & Leak Detection Gate (`check_public_sanitization`)
- Recursively scans all repository files (excluding `.git` and `build/`) for:
  - Leaked ST-LINK hardware serial number (24-hex string).
  - Leaked workstation filesystem paths.
  - Leaked local user profile paths.
  - Secret API tokens (`ghp_*`, `github_pat_*`).
- Emits an `ERROR` if any sensitive pattern is detected. Verified **0 leaks across 227 files**.

### 2.5 Expanded Governance and Audit File Registries
- **Required Root & Support Files:** Expanded from 14 to **19** files, registering [`SECURITY.md`](../SECURITY.md), [`THIRD_PARTY_LICENSES.md`](../THIRD_PARTY_LICENSES.md), [`.github/dependabot.yml`](../.github/dependabot.yml), [`SBOM/spdx-sbom.json`](../SBOM/spdx-sbom.json), and [`scripts/generate_sbom.py`](../scripts/generate_sbom.py).
- **Mandatory Audit Files:** Expanded from 32 to **41** files, registering [`AUDIT/release_metrics.json`](release_metrics.json), [`AUDIT/COMPREHENSIVE_FINAL_AUDIT.md`](COMPREHENSIVE_FINAL_AUDIT.md), [`AUDIT/RELEASE_INTEGRITY_REPORT.md`](RELEASE_INTEGRITY_REPORT.md), [`AUDIT/HARDWARE_EVIDENCE_AUDIT.md`](HARDWARE_EVIDENCE_AUDIT.md), [`AUDIT/TECHNICAL_CORRECTNESS_REPORT.md`](TECHNICAL_CORRECTNESS_REPORT.md), [`AUDIT/DRIVER_QUALITY_REPORT.md`](DRIVER_QUALITY_REPORT.md), [`AUDIT/FREERTOS_REVIEW.md`](FREERTOS_REVIEW.md), [`AUDIT/CURRICULUM_QUALITY_REPORT.md`](CURRICULUM_QUALITY_REPORT.md), and [`AUDIT/CI_CD_AND_SECURITY_REPORT.md`](CI_CD_AND_SECURITY_REPORT.md).

---

## 3. Deliberate Negative Regression Test Suite Results

A dedicated automated test runner ([`tests/test_validator_regressions.py`](../tests/test_validator_regressions.py)) executes synthetic defect injection across an isolated repository clone to prove that Validator v5.0.0 reliably halts with `[ERROR]` on every required defect class.

### Test Execution Output (`python tests/test_validator_regressions.py`)

```text
=== Running Validator v5.0.0 Deliberate Negative Regression Test Suite ===
[PASS] Baseline clean repo check: 0 ERROR, exit code 0.
  [PASS] Defect Class 01: Wrong register offset in register map -> Caught with ERROR
  [PASS] Defect Class 02: Wrong bitfield bit position in register map -> Caught with ERROR
  [PASS] Defect Class 03: Malformed register reset value in table -> Caught with ERROR
  [PASS] Defect Class 04: Invalid register access type in table -> Caught with ERROR
  [PASS] Defect Class 05: Broken Markdown internal #anchor link -> Caught with ERROR
  [PASS] Defect Class 06: Missing lab Prerequisites heading/field -> Caught with ERROR
  [PASS] Defect Class 07: False COMPLETE claim in SoT (empty location/evidence) -> Caught with ERROR
  [PASS] Defect Class 08: Missing required lab section (Expected Output) -> Caught with ERROR
  [PASS] Defect Class 09: Inconsistent dual clock profile configuration -> Caught with ERROR
  [PASS] Defect Class 10: Conflicting release_metrics.json concept totals -> Caught with ERROR
  [PASS] Defect Class 11: Leaked ST-LINK hardware serial number -> Caught with ERROR
  [PASS] Defect Class 12: Leaked workstation filesystem local path -> Caught with ERROR

=== Regression Suite Results: 12/12 Defect Classes Caught (100% PASS) ===
```

---

## 4. Live Validator Execution Summary (v5.0.0)

| Mode | Command | `INFO/PASS` | `WARNING` | `UNVERIFIED` | `ERROR` | Exit Code |
|---|---|---:|---:|---:|---:|---:|
| **Mode 1 (`--skip-builds`)** | `python scripts/validate_repo.py --skip-builds` | `26` | `1` | `11` | **`0`** | `0` |
| **Mode 2 (Default)** | `python scripts/validate_repo.py` | `35` | `1` | `2` | **`0`** | `0` |

### Detailed Pillar Breakdown

| Verification Pillar | Status | Exact Measured Result |
|---|---|---|
| Structure & Governance | `INFO/PASS` | `34/34` dirs, `19/19` onboarding/tooling files, `41/41` mandatory AUDIT files, `0` stale scratch files |
| Raw Control Characters | `INFO/PASS` | `0` raw ASCII control bytes across `155` Markdown/Mermaid files |
| CMSIS Register Ground Truth | `INFO/PASS` | `16/16` maps cite RM0440/PM0214; verified `87` bases, `344` struct offsets, `114` bit positions, `310` access types, `222` reset values (`0` mismatches) |
| SOT DAG & Evidence | `INFO/PASS` + `WARNING` | `350` unique IDs, `27/27` domains, `0` cycles; `265/265` `COMPLETE` verified; `WARNING` flags `88` concepts for manual review |
| Release Metrics Single Source of Truth | `INFO/PASS` | `release_metrics.json` verified; `350` concepts, `v4.0.0` version, strict sync with SoT, README, and START_HERE |
| Dual Clock Profile Consistency | `INFO/PASS` | 160 MHz & 170 MHz Range 1 Boost profiles verified across 4 files |
| Public Sanitization Gate | `INFO/PASS` | `0` serials, `0` local workstation paths, `0` user profile paths, `0` tokens across `227` files |
| Practical Laboratories | `INFO/PASS` | `20/20` labs verified with all 11 core sections + Clock Profile + Verification Record + Prerequisites |
| Local Markdown Links, Anchors & MathJax | `INFO/PASS` | `1,408/1,408` inline links, `22/22` `#anchor` fragments, `0` broken images/ref-links, `0` MathJax violations across `143` `.md` files |
| Mermaid Diagrams | `INFO/PASS` + `UNVERIFIED` | `mermaid_syntax`: `12/12` passed; `mermaid_render`: `UNVERIFIED` (`mmdc` absent on host) |
| Curriculum, Drivers, RTOS, Capstones, Videos | `INFO/PASS` | `17/17` stages, `12/12` drivers, FreeRTOS V11.1.0, `3/3` Capstones, `70` videos (12/12 columns, 0 duplicate IDs) |
| Firmware & Host Test Builds | `INFO/PASS` | `16/16` register C snippets; template debug & release; FreeRTOS; 3 Capstones; `873/873` host unit assertions |

---

## 5. Verification Sign-Off

Validator **v5.0.0** is confirmed robust, regression-resistant, and verified with zero errors on the canonical codebase.
