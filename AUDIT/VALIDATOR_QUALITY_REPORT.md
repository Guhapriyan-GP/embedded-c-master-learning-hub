# Validator Quality & Regression-Resistance Report (`scripts/validate_repo.py` v4.1.0)

**Date:** 2026-11-18  
**Branch:** `feature/complete-remediation`  
**Validator Artifact:** [`scripts/validate_repo.py`](../scripts/validate_repo.py) (upgraded from v3.0.0 → v4.0.0 → **v4.1.0**, closing all 5 Agent 12 blind spots)  
**Target Platform Ground Truth:** `stm32g474xx.h` + `core_cm4.h` (STM32G474RET6 / ARM Cortex-M4F r0p1 / NUCLEO-G474RE MB1367)  
**Verification Honesty Notice:** Firmware builds and register-map C snippets are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, Not Tested on Physical Hardware in this CI pass)**; host unit tests are **Host-Executed (`gcc 15.2.0`, 594 assertions, 0 failed)**; validator checks are **Live-Executed & Regression-Proven (13/13 synthetic regressions caught)**.

---

## 1. Environment & Toolchain Versions

| Tool | Exact Version | Role in Validation |
|---|---|---|
| Python | `3.14.6` | Executes `scripts/validate_repo.py` v4.0.0 |
| ARM Cross-Compiler | `arm-none-eabi-gcc.exe (GNU Tools for STM32 14.3.rel1.20251027-0700) 14.3.1 20250623` | Compiles starter template (`debug` & `release`), FreeRTOS V11.1.0, and 3 Capstones |
| Host C Compiler | `gcc.exe (Rev8, Built by MSYS2 project) 15.2.0` | Compiles and executes 4 host C11 unit-test suites (`594` assertions) |
| Build Tool | `GNU Make 4.4.1_st_20260330-0700` | Orchestrates firmware and host test Makefiles |

---

## 2. Architecture Upgrade Summary (v3.0.0 → v4.0.0)

All 10 mandatory Section 2.C verification pillars have been implemented programmatically, eliminating weak string-matching and length-only heuristics:

| # | Mandatory Pillar | v3.0.0 Weakness (Before) | v4.0.0 Implementation (After) |
|---|---|---|---|
| 1 | **Real CMSIS Header Register Cross-Checking** | 10 hardcoded Markdown regexes | Programmatically parses `stm32g474xx.h` & `core_cm4.h` (`171` base macros, `57` C `typedef struct` layouts with exact alignment/array/union byte offsets, `4,082` `_Pos` & `3,916` `_Msk` bitfields, `110` `IRQn_Type` entries) and cross-verifies all 16 register maps (`87` base addresses, `344` struct register offsets, `21` bit positions) directly against header ground truth. |
| 2 | **Evidence-Based `COMPLETE` Concept Audit** | Checked only JSON schema & file existence | Retains full 350-concept DAG check (`0` cycles, `0` unknown prereqs, `0` inverted edges, `27/27` domains) AND audits every `COMPLETE` concept (`265`) for non-stub target evidence (Concept ID or title keywords), substantive `completion_criteria`/`reference`/`practical_implementation` (`>= 20` chars, no `TBD`/`TODO`, no pipe-split truncation), and `hardware_status` consistency. |
| 3 | **Lab 11-Section + Clock Profile + Verification Record Audit** | Counted `lab-*` directory count only | Verifies all 20/20 `18-practical-laboratory/lab-*/README.md` files for all 11 core pedagogical sections + Explicit Clock Configuration Profile Declaration/Comparison + Standardized Verification Record. |
| 4 | **Full Markdown Link, Reference-Style Link, Image & External URL Audit** | Inline `](...)` only; ignored ref links, images, and external URLs | Audits inline links, local images (`![alt](target)`), and reference-style links (`[text][id]` + `[id]: target`) while ignoring C 2D array subscripts (`buf[32][2]`). Tracks unique external URLs as `UNVERIFIED` by default, or probes concurrently via `--check-external-urls` (`accessible`, `bot-blocked 403/429`, `broken 404/DNS`, `timeout/unverified`). |
| 5 | **GitHub MathJax Safety Audit** | Not checked at all | Scans all inline `$...$` and display `$$...$$` math spans (outside fenced code blocks and inline backticks) with brace-balanced `\text{...}` / `\mathrm{...}` parsing to catch any `_` or `\_` (which breaks GitHub GFM+MathJax with `'_' allowed only in math mode`), reporting exact `file:line` violations. |
| 6 | **Real Mermaid Diagram Syntax Parser** | Checked Line 1 header and `<br` only | Parses every `.mmd` file for valid diagram headers, balanced `subgraph`/`end` (and sequence block) nesting stack depth, balanced `"` quotes per line, balanced `[]`/`()`/`{}` delimiters outside strings, valid connectors/edge-label pipes, non-zero nodes & edges, and zero HTML tags. |
| 7 | **Curriculum 13-Section + Primary Ref + Navigation Audit** | Checked `"Completion Criteria"` substring and `len >= 5000` only | Verifies all 17 stages (`01b` + `02–17`) for all 13 required section headings, at least one authoritative primary reference (`RM0440`, `DS12288`, `PM0214`, `UM2505`, `ES0430`, `ARM`, `ISO`), and explicit Prev/Next navigation links (`WARNING` if missing). |
| 8 | **Missing Tools vs Build Failures Distinction** | Emitted `[WARN]` when tools missing | Strictly records `UNVERIFIED` when `arm-none-eabi-gcc`, `arm-none-eabi-readelf`, `gcc`, or `make` is missing (never `PASS`, never `ERROR`), and `ERROR` when present tools exit non-zero. |
| 9 | **Host Unit Test Verification & Cleanup** | Did not clean up `tests/build/` afterward | Executes `make -C tests clean test`, verifies exit code `0`, confirms `0 failed` across all suites and `>= 500` executed assertions (`594` across `4` suites), and runs `make -C tests clean` in a `finally` block. |
| 10 | **Structured Severity Taxonomy & CLI** | Flat PASS/WARN/FAIL print only | Clean 4-level taxonomy (`INFO/PASS`, `WARNING`, `UNVERIFIED`, `ERROR`), exits `1` iff `ERROR > 0`, supports `--skip-builds`, `--check-external-urls`, `--json-report <path>`, `--repo-root <path>`, and `--quiet`. |

---

## 3. Mandatory Regression-Resistance Proof (13 / 13 Regressions Caught)

To prove `scripts/validate_repo.py` **v4.1.0** is genuinely regression-resistant (including closing all 5 adversarial blind spots identified in Agent 12 Part III `[R12-HIGH-03]`), isolated synthetic fault-injection suites created lightweight temporary copies (`tempfile.TemporaryDirectory()`, leaving the live repository completely untouched) and injected **13 independent regressions** (`REG-01..08` original baseline + `REG-09..13` Agent 12 blind-spot mutations `M2`, `M4`, `M5`, `M6`, `M7`). Every check transitioned cleanly from `0` baseline errors to an exact targeted failure (**`13 / 13 CAUGHT`**):

| Regression ID | Injected Fault | Validator Check | Baseline Errors | Mutated Errors | Result | Exact Emitted Diagnostic |
|---|---|---|---:|---:|---|---|
| **REG-01** | Corrupted register offset in `pwr-flash-register-map.md`: `PWR_CR5` changed `0x080` → `0x010` | `cmsis_registers` | `0` | `1` | **CAUGHT** | `OFFSET MISMATCH pwr-flash-register-map.md:27 PWR_TypeDef.CR5 (from 'PWR_CR5'): doc=0x010 vs CMSIS=0x080` |
| **REG-02** | Corrupted peripheral base in `rcc-register-map.md`: STM32F4 `RCC` base `0x40023800` injected (true G474 = `0x40021000`) | `cmsis_registers` | `0` | `1` | **CAUGHT** | `BASE MISMATCH rcc-register-map.md:221 RCC: doc=0x40023800 vs CMSIS=0x40021000` |
| **REG-03** | GitHub MathJax bug injected: `$f_{\text{PLL\_IN}} = 4\text{ MHz}$` | `mathjax` | `0` | `1` | **CAUGHT** | `sample.md:3: \text{PLL\_IN}` |
| **REG-04** | Unbalanced Mermaid `subgraph` in `stm32g474re-clock-tree.mmd`: deleted closing `end` | `mermaid` | `0` | `1` | **CAUGHT** | `20-technical-diagrams\stm32g474re-clock-tree.mmd: unbalanced block structure (1 unclosed subgraph/block(s) missing 'end')` |
| **REG-05** | Missing mandatory Lab section in `lab-01-baremetal-gpio-led-blinky/README.md`: removed `## 8. Debugging Procedure` | `labs` | `0` | `1` | **CAUGHT** | `lab-01-baremetal-gpio-led-blinky: missing 1 section(s): Debugging Procedure` |
| **REG-06** | Broken reference-style Markdown link injected: `[foo][bar]` with no `[bar]: ...` definition | `ref_links` | `0` | `1` | **CAUGHT** | `doc.md:3: [foo][bar] (undefined ref 'bar')` |
| **REG-07** | Bogus `COMPLETE` concept (`CS-ALGO-001`): set `completion_criteria="TBD"`, `reference="TODO"`, `repository_location="stub_empty.md"` (17 B unrelated stub) | `sot_evidence` | `0` | `3` | **CAUGHT** | `CS-ALGO-001: completion_criteria contains placeholder ('TBD'); CS-ALGO-001: reference contains placeholder ('TODO'); CS-ALGO-001: repository_location 'stub_empty.md' is missing or a tiny stub (max_size=17 B)` |
| **REG-08** | Simulated runner missing `arm-none-eabi-gcc`, `gcc`, and `make` (`shutil.which` & `find_arm_tool` mocked to `None`) | `firmware_builds` + `host_tests` | `0` | `0` (`3 UNVERIFIED`) | **CAUGHT** | `Firmware builds UNVERIFIED: missing required tool(s): make, arm-none-eabi-gcc.; Register-map C snippet compilation UNVERIFIED...; Host C unit tests UNVERIFIED: missing required tool(s): gcc, make.` |
| **REG-09 (`M2`)** | Table-cell bitfield range corruption in `opamp-comp-vrefbuf-register-map.md`: `BLANKING[21:19]` → `BLANKING[21:18]` | `cmsis_registers` (Pass C2) | `0` | `1` | **CAUGHT** | `TABLE BITFIELD MISMATCH opamp-comp-vrefbuf-register-map.md:32 COMP_CSR_BLANKING: doc=[21:18] vs CMSIS=[21:19]` |
| **REG-10 (`M4`)** | Broken Markdown `#anchor` fragment link injected: `[broken anchor](test_doc.md#totally-fake-heading)` | `anchors` (GFM slug) | `0` | `1` | **CAUGHT** | `caller.md:3 -> test_doc.md#totally-fake-heading (missing heading '#totally-fake-heading')` |
| **REG-11 (`M5`)** | `\texttt{...}` MathJax underscore bug injected: `$f_{\texttt{TIM\_CLK}} = 170\text{ MHz}$` | `mathjax` (expanded) | `0` | `1` | **CAUGHT** | `doc.md:3: \texttt{TIM\_CLK}` |
| **REG-12 (`M6`)** | Unclosed inline math `$f_{SYSCLK} = 170 MHz` + bare `\_` in math `$t_{S\_temp} \ge 5\ \mu\text{s}$` injected | `mathjax` (expanded) | `0` | `2` | **CAUGHT** | `doc.md:3: unclosed inline math '$': '$f_{SYSCLK} = 170 MHz'; doc.md:5: bare \_ inside math: 't_{S\\_temp} \\ge 5\\ \\mu\\text{s}'` |
| **REG-13 (`M7`)** | Invalid CMSIS macro in `opamp-comp-vrefbuf-register-map.md` C snippet: `OPAMP_CSR_OPAMPxEN` → `OPAMP_CSR_OPAEN` | `register_snippets` | `0` | `1` | **CAUGHT** | `opamp-comp-vrefbuf-register-map.md C-block #1: opamp-comp-vrefbuf-register-map_1.c:...: error: 'OPAMP_CSR_OPAEN' undeclared` |

---

## 4. Live Repository Run Results (Post-Cleanup Final Gate — `0 ERROR`)

### Command Executed
```bash
python scripts/validate_repo.py --check-external-urls
```

### Final Summary Metrics (`28 INFO/PASS | 1 WARNING | 1 UNVERIFIED | 0 ERROR` default / `0 UNVERIFIED` with `--check-external-urls`, exit code `0`)

| Verification Pillar | Pre-Cleanup v4.0.0 Run | **Final Post-Closure v4.1.0 Run** | Exact Measured Result |
|---|---|---|---|
| Structure & Hygiene | `INFO/PASS` | **`INFO/PASS`** | `34/34` dirs, `14/14` onboarding/tooling files, **`29/29` mandatory AUDIT files** (all Section 9 reports + `FINAL_REMEDIATION_REPORT.md`), `0` stale `scratch/` |
| Raw Control Characters | `INFO/PASS` | **`INFO/PASS`** | `0` raw ASCII control bytes across `142` Markdown & Mermaid files |
| **CMSIS Register Ground Truth** | `INFO/PASS` | **`INFO/PASS`** | Parsed `171` base macros, `57` structs, `4,082` `_Pos`, `3,916` `_Msk`, `110` IRQns (`FMAC_IRQn = 101`); cross-verified **`87` base addresses, `344` struct register offsets, and `114` bit positions** (bullet + table-cell `FIELD[hi:lo]`/`(bN)`) across `16` register maps (`0` mismatches) |
| **Register-Map C Snippets (M7)** | *(Not checked in v4.0.0)* | **`INFO/PASS`** | All **`16/16`** standalone CMSIS C snippets in `20-technical-diagrams/registers/*.md` syntax-compiled cleanly (`-fsyntax-only -Wall -Wextra -Werror`; `3` prose fragments skipped) |
| **SOT DAG Integrity** | `INFO/PASS` | **`INFO/PASS`** | `AUDIT/concepts_sot.json` v2.1.0: `350` unique IDs, `27/27` domains, `0` cycles, `0` unknown prereqs, `0` inverted-level edges |
| **SOT `COMPLETE` Evidence** | `ERROR` + `WARNING` | **`INFO/PASS` + `WARNING`** | **`265/265` `COMPLETE` concepts verified with substantive evidence and complete metadata (`0` truncated/short fields, `0` `JP5` IDD hazards)**; `1` honest `WARNING` retained for `18` `COMPLETE` concepts with external-hardware/bench-pending status |
| **Practical Labs (20/20)** | `INFO/PASS` | **`INFO/PASS`** | `20/20` labs contain all 11 core sections + Clock Profile Declaration + Standardized Verification Record |
| **Internal Links, `#anchors` (M4) & Images** | `ERROR` (`1` broken) | **`INFO/PASS`** | **`1,058/1,058` inline links & `11/11` `#anchor` GFM heading fragments verified across `130` `.md` files (`0` broken)**, `0` broken images, `0` broken ref links |
| **Expanded GitHub MathJax Safety (M5/M6)** | `ERROR` (`2` left) | **`INFO/PASS`** | **`130/130` `.md` files clean (`0` violations)** across `\text`/`\mathrm`/`\texttt`/`\textbf`/`\textit`/`\mbox`, bare `\_` in math, balanced `{}` braces, and closed `$`/`$$` |
| **External URL Live Probe** | `INFO/PASS` | **`INFO/PASS` / `UNVERIFIED`** | **`70 / 70` unique URLs accessible** with `--check-external-urls` (`0` broken); cleanly reported `UNVERIFIED` in offline default mode |
| **Mermaid Syntax Audit** | `INFO/PASS` | **`INFO/PASS`** | `12/12` `.mmd` diagrams valid (headers, balanced subgraphs/quotes/delimiters, valid arrows, non-zero nodes/edges, `0` HTML tags) |
| **Curriculum 13-Section Audit** | `INFO/PASS` | **`INFO/PASS`** | `17/17` stages verified with all 13 required sections and authoritative primary references |
| **Curriculum Prev/Next Nav** | `WARNING` (`4` missed) | **`INFO/PASS`** | **`17/17` curriculum stages verified with Top & Bottom Prev/Next navigation links** (`STAGE_NAV_RE` aligned for bracketed arrows) |
| Drivers, FreeRTOS, Capstones, Video DB | `INFO/PASS` | **`INFO/PASS`** | `12/12` drivers, pinned FreeRTOS V11.1.0, `3/3` Capstones, `70` unique HTTPS videos (`0` dupes) |
| **Firmware Builds + ELF** | `INFO/PASS` | **`INFO/PASS`** | Starter template (`debug` & `release`), ELF `.isr_vector` (`0x08000000`, `472 B`, `512 B` align), FreeRTOS V11.1.0, and `3/3` Capstones all compiled cleanly (exit `0`) |
| **Host C Unit Tests** | `INFO/PASS` | **`INFO/PASS`** | `make -C tests clean test` passed: **`4` suites, `594` assertions executed, `0` failed**; `tests/` cleaned afterward |

*(In default mode without `--check-external-urls`, the external URL check cleanly records `[UNVERIFIED] [external_urls] 70 unique external URLs UNVERIFIED (pass --check-external-urls to probe live).`)*

---

## 5. Disposition of Findings Surfaced by Validator v4.0.0

Every real defect surfaced by the hardened v4.0.0 validator has been resolved and re-verified (`0 ERROR`):

### 5.1 `AUDIT/concepts_sot.json` — 14 Truncated / Short Metadata Fields (`ERROR` -> **RESOLVED**)
All 12 flagged `COMPLETE` concepts (`EMB-MMIO-003`, `EMB-REG-001`, `EMB-REG-003`, `EMB-REG-004`, `PERIPH-EXTI-002`, `RT-TIME-001`, `RT-WDG-001`, `RTOS-SYNC-003`, `TOOL-DBG-002`, `FW-PAT-003`, `PROF-PERF-002`, `PROF-REL-001`) plus 7 additional records caught during full-SoT audit (`TOOL-BUILD-001`, `PROF-REL-002`, `STM32-PWR-003`, `STM32-PWR-002` `JP5 -> JP6`, and `PERIPH-ENC-001`/`PERIPH-IC-001`/`PERIPH-IC-002`) were restored with complete, non-truncated technical explanations, completion criteria, and authoritative references. [`AUDIT/COMPLETE_CONCEPT_INVENTORY.md`](COMPLETE_CONCEPT_INVENTORY.md) was deterministically regenerated with pipe-safe (`\|`) and math/backtick-safe escaping.

### 5.2 Broken Internal Inline Link & Fragile `.elf` Links (`ERROR` -> **RESOLVED**)
- `01-master-concept-map/learning-sequence.md:50` fixed (`../23-code-library/templates/` + `../23-code-library/README.md`).
- `AUDIT/HARDWARE_VALIDATION_SUMMARY.md` converted all 7 links to gitignored `build/*.elf` artifacts into inline code paths so link validation never fails after `make clean`.

### 5.3 Remaining GitHub MathJax Violations (`ERROR`, `136 -> 2 -> 0` -> **RESOLVED**)
- `00-project-management/research-methodology.md:15` — `\text{TIM\_CLK}` -> `\text{TIM-CLK}`.
- `AUDIT/STM32G474RE_PERIPHERAL_INVENTORY.md:54` — `\text{PLL\_IN}` -> `\text{PLL-IN}`.
- All `130/130` Markdown files across the repository now pass with `0` MathJax violations.

### 5.4 Curriculum Prev/Next Navigation (`WARNING`, `4 -> 0` -> **RESOLVED**)
- Verified top + bottom Prev/Next navigation bars exist across all `17/17` stages (`04`, `08`, `09`, `10` added by Specialist 1); extended `STAGE_NAV_RE` in `scripts/validate_repo.py` to accept both `← [...]` and `[← ...]` / `[... →]` arrow formats (`17/17 INFO/PASS`).

### 5.5 Hardware-Pending `COMPLETE` Concepts (Intentional Honesty `WARNING`, `18` concepts)
18 concepts marked `current_status = "COMPLETE"` carry `hardware_status` of `EXTERNAL_HARDWARE_PENDING` (`12`) or `NEEDS_BENCH_INSTRUMENT` (`6`), honestly documenting that code and curriculum docs are complete while external breadboard/bench-instrument tests remain tracked as `PENDING — USER CONNECTION REQUIRED` in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).

---

## 6. Verification Checklist (v4.1.0 Final Closure)
- [x] `scripts/validate_repo.py` **v4.1.0** implements all 10 mandatory Section 2.C requirements PLUS all 5 Agent 12 Part III blind-spot closures (`M2` table-cell bitfields, `M4` GFM `#anchor` verification, `M5`/`M6` expanded MathJax safety, `M7` register-map C-snippet CMSIS compilation, and `29/29` mandatory audit files including all Section 9 reports).
- [x] Zero hardcoded Markdown register regexes; 100% programmatic CMSIS header parsing (`stm32g474xx.h` + `core_cm4.h`).
- [x] **`13 / 13`** synthetic injected regressions (`REG-01..08` + `REG-09..13` for `M2, M4, M5, M6, M7`) caught and verified in isolated temporary directories.
- [x] All validator-surfaced and Agent 12 Part III defects (`10/10`) remediated and verified (`0 ERROR`).
- [x] Full live execution of `python scripts/validate_repo.py` **v4.1.0** completed with **`0 ERROR` (exit code `0`)**, including all firmware builds (`arm-none-eabi-gcc 14.3.1`), ELF `.isr_vector` check, `16/16` register-map CMSIS C-snippet syntax compilations, and host unit tests (`594/594` assertions, `0` failed, cleaned afterward).
