# Validator Quality & Regression-Resistance Report (`scripts/validate_repo.py` v4.2.0)

**Date:** 2026-11-18  
**Branch:** `feature/complete-remediation`  
**Validator Artifact:** [`scripts/validate_repo.py`](../scripts/validate_repo.py) (upgraded `v3.0.0` → `v4.0.0` → `v4.1.0` → **`v4.2.0`**, closing all Audit Round-2 Section 4 requirements `4.A–4.G`)  
**Target Platform Ground Truth:** `stm32g474xx.h` + `core_cm4.h` (STM32G474RET6 / ARM Cortex-M4F r0p1 / NUCLEO-G474RE MB1367)  
**Verification Honesty Notice:** Firmware builds and register-map C snippets are **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, Not Tested on Physical Hardware in this CI pass)**; host unit tests are **Host-Executed (`gcc 15.2.0`, 594 assertions, 0 failed)**; Mermaid diagrams are **Syntax-Validated Only (`mmdc` visual renderer unavailable on host → explicitly reported `UNVERIFIED`)**; validator checks are **Live-Executed & Regression-Proven (19/19 synthetic regressions caught)**.

---

## 1. Environment & Toolchain Versions

| Tool | Exact Version | Role in Validation |
|---|---|---|
| Python | `3.14.6` | Executes `scripts/validate_repo.py` v4.2.0 |
| ARM Cross-Compiler | `arm-none-eabi-gcc.exe (GNU Tools for STM32 14.3.rel1.20251027-0700) 14.3.1 20250623` | Compiles starter template (`debug` & `release`), FreeRTOS V11.1.0, 3 Capstones, and 16 register-map CMSIS C snippets |
| ELF Inspector | `arm-none-eabi-readelf` (GNU Binutils) | Verifies `.isr_vector` (`0x08000000`, `472 B`, `512 B` alignment) |
| Host C Compiler | `gcc.exe (Rev8, Built by MSYS2 project) 15.2.0` | Compiles and executes 4 host C11 unit-test suites (`594` assertions) |
| Build Tool | `GNU Make 4.4.1_st_20260330-0700` | Orchestrates firmware and host test Makefiles |
| Mermaid CLI (`mmdc`) | Not installed on host (`shutil.which("mmdc") == None`) | Actively probed at runtime; `mermaid_render` honestly reports `UNVERIFIED` |

---

## 2. Architecture Upgrade Summary (`v4.1.0` → `v4.2.0` — Audit Round-2 Section 4)

All 7 Audit Round-2 Section 4 requirements (`4.A–4.G`) have been implemented on top of the 10 Section 2.C verification pillars and 5 Agent 12 blind-spot closures:

| Section | Enhancement Pillar | v4.1.0 Behavior (Before) | v4.2.0 Implementation (After) |
|---|---|---|---|
| **4.A** | **Register Verification Scope Transparency & RM0440 Citation Check** | Verified `87` bases, `344` offsets, `114` bit positions, and `16/16` C snippets, without checking RM0440/PM0214 citations or printing explicit scope limits | Retains full CMSIS header parser (`stm32g474xx.h` + `core_cm4.h`), verifies every register map in `20-technical-diagrams/registers/*.md` cites `RM0440` or `PM0214`/`ARMv7-M` (`16/16`), and prints exact coverage counts + explicit scope boundary (`scope limited to parsed table entries, bullet bit positions & CMSIS C snippets, not every RM0440 reset value or unparsed prose field`). |
| **4.B** | **Concept Inventory Evidence Audit & Manual-Review Queue** | Flagged only `18` hardware-pending `COMPLETE` concepts | Never auto-promotes `PARTIAL` (`74` preserved untouched). Emits a transparent **`WARNING (MANUAL REVIEW RECOMMENDED)`** (`sot_manual_review`) listing every `COMPLETE` concept with `hardware_status` in `EXTERNAL_HARDWARE_PENDING`/`NEEDS_BENCH_INSTRUMENT` (`18`) OR lacking an explicit lab/driver/test path in `practical_implementation` (`72`; `88` unique union, `2` in both), with full ID arrays in JSON metrics. |
| **4.C** | **Laboratory `Prerequisites` Enforcement** | Checked 11 core sections + Clock Profile + Verification Record | Adds explicit `Prerequisites` check (`labs_prerequisites`, heading or bold field) across all `20/20` labs, enforced as `ERROR` now that all `20/20` labs declare Prerequisites. |
| **4.D** | **Mermaid Syntax vs Render Separation** | Emitted single `[mermaid]` pass line without checking `mmdc` | Cleanly separates `mermaid_syntax` (full Python structural parser → `INFO/PASS` on `12/12`) from `mermaid_render` (runtime `shutil.which("mmdc")` probe → renders SVGs if present, or explicitly emits `[UNVERIFIED] [mermaid_render] Mermaid visual rendering NOT TESTED — 'mmdc' (@mermaid-js/mermaid-cli) tooling unavailable on host.`). |
| **4.E** | **Video CSV Schema, Redirect Tracking & Offline Resilience** | Checked `https://` prefix & YouTube IDs only; DNS error caused `ERROR` offline | Strictly validates all `12/12` exact CSV columns, unique `index` IDs, unique valid HTTPS URLs, and `0` duplicate YouTube IDs. Live `--check-external-urls` probe uses `_TrackingRedirectHandler` to track `accessible (2xx)`, `redirected (3xx -> final URL)`, `bot-blocked (403/429)`, `broken (404/410)`, and `network/DNS/timeout failures`. **Offline Resilience Rule:** `0` HTTP responses + network/DNS failures emit `UNVERIFIED` (never `ERROR`); only definite HTTP `404`/`410` when online emits `ERROR`. |
| **4.F** | **Local Markdown Links Separation** | Combined local Markdown checks and remote URL probing in one function | Cleanly separates `check_markdown_links_and_mathjax()` (local inline links, `#anchor` GFM slugs, local images, reference-style links, expanded MathJax safety) from `check_external_urls_live()`. |
| **4.G** | **Exact 9-State Build/Test Taxonomy & ZERO Short-Circuiting** | `break` on starter template `debug` failure hid `release`; generic skip messages | Eliminates all early returns / `break` short-circuits in `check_builds_and_tests()`. All 9 independent stages (`register_snippets`, `firmware_build_template_debug`, `firmware_build_template_release`, `firmware_elf`, `firmware_build_freertos`, `firmware_build_capstone_01..03`, `host_tests`) always execute independently and emit exact tags from the 9-state taxonomy (`TOOLCHAIN_UNAVAILABLE`, `BUILD_SKIPPED`, `BUILD_FAILED`, `BUILD_PASSED`, `HOST_TESTS_SKIPPED`, `HOST_TESTS_FAILED`, `HOST_TESTS_PASSED`, `ELF_INSPECTION_UNAVAILABLE`, `ELF_INSPECTION_PASSED`). |

---

## 3. Mandatory Regression-Resistance Proof (19 / 19 Regressions Caught)

All 13 baseline + Agent 12 synthetic regressions (`REG-01..13`) remain enforced, plus **6 new v4.2.0 Round-2 fault-injection tests (`REG-14..19`)** executed in isolated temporary directories / mocks (leaving the live repository untouched):

| Regression ID | Injected Fault / Simulated Condition | Validator Check | Result | Exact Emitted Diagnostic / Proof |
|---|---|---|---|---|
| **REG-01..08** | Baseline faults: corrupted `PWR_CR5` offset, STM32F4 `RCC` base, MathJax `\text{PLL\_IN}`, unclosed Mermaid `subgraph`, missing Lab section, undefined ref link, bogus `COMPLETE` stub, missing toolchain | Core pillars `1–9` | **8/8 CAUGHT** | Exact targeted `ERROR` / `UNVERIFIED` diagnostics (retained in v4.2.0) |
| **REG-09..13** | Agent 12 blind spots: `M2` table bitfield `BLANKING[21:18]`, `M4` broken `#anchor`, `M5` `\texttt{TIM\_CLK}`, `M6` unclosed `$` & bare `\_`, `M7` invalid CMSIS macro `OPAMP_CSR_OPAEN` | Hardened pillars | **5/5 CAUGHT** | Exact targeted `ERROR` diagnostics (retained in v4.2.0) |
| **REG-14 (`4.A`)** | Stripped `RM0440` citation from `gpio-register-map.md` in isolated temp copy | `cmsis_registers` | **CAUGHT** | `Official reference citation check FAILED: 1/16 register map(s) missing RM0440 or PM0214/ARMv7-M citation` |
| **REG-15 (`4.C`)** | Removed `Prerequisites` from `lab-01-baremetal-gpio-led-blinky/README.md` in isolated temp copy | `labs_prerequisites` | **CAUGHT** | `1/20 practical lab(s) missing explicit Prerequisites declaration` (`ERROR`) |
| **REG-16 (`4.E`)** | Renamed `missing_concepts` column header to `bad_col_name` in `video-database.csv` | `video_db` | **CAUGHT** | `CSV schema column mismatch (missing=['missing_concepts'], extra=['bad_col_name'], ...)` (`ERROR`) |
| **REG-17 (`4.E`)** | Simulated offline host (`DNS error` on all URL probes) AND simulated online mix (`200`, `301 -> 200`, `403`, `404`) | `external_urls` | **CAUGHT** | Offline: `0 ERROR`, `1 UNVERIFIED` (offline resilience). Online: `1 ERROR` (`404`), `1 WARNING` (`403`), `1 redirected (3xx -> final URL)`, `1 accessible (2xx)` |
| **REG-18 (`4.D`)** | Probed Mermaid with `mmdc = None` vs mocked `mmdc` rendering SVGs | `mermaid_syntax` + `mermaid_render` | **CAUGHT** | Without `mmdc`: `mermaid_syntax=INFO/PASS` + `mermaid_render=UNVERIFIED`; with `mmdc`: `Successfully rendered all 12/12 Mermaid (.mmd) diagrams to SVG via mmdc.` |
| **REG-19 (`4.G`)** | Simulated starter template `BUILD_TYPE=debug` failing (`exit 2`) + simulated missing toolchain (`which=None`) | `check_builds_and_tests` | **CAUGHT** | Debug failure (`BUILD_FAILED`, `1 ERROR`) did **NOT** short-circuit: `release`, `firmware_elf`, `freertos`, `capstone_01..03`, `register_snippets`, and `host_tests` ALL executed (`8 INFO/PASS`). Missing toolchain emitted `9 UNVERIFIED` (`TOOLCHAIN_UNAVAILABLE`/`ELF_INSPECTION_UNAVAILABLE`, `0 ERROR`). |

---

## 4. Live Repository Run Results (`v4.2.0` — All 3 Modes, `0 ERROR`)

### Mode Summary Comparison

| Mode | Command | `INFO/PASS` | `WARNING` | `UNVERIFIED` | `ERROR` | Exit Code |
|---|---|---:|---:|---:|---:|---:|
| **Mode 1 (`--skip-builds`)** | `python scripts/validate_repo.py --skip-builds` | `23` | `1` | `11` | **`0`** | `0` |
| **Mode 2 (Default)** | `python scripts/validate_repo.py` | `32` | `1` | `2` | **`0`** | `0` |
| **Mode 3 (Live URLs)** | `python scripts/validate_repo.py --check-external-urls` | `33` | `1` | `1` | **`0`** | `0` |

### Exact Per-Pillar Results (Mode 3: `python scripts/validate_repo.py --check-external-urls`)

| Verification Pillar | Status | Exact Measured Result |
|---|---|---|
| Structure & Hygiene | `INFO/PASS` | `34/34` dirs, `14/14` onboarding/tooling files, `32/32` mandatory AUDIT files, `0` stale `scratch/` |
| Raw Control Characters | `INFO/PASS` | `0` raw ASCII control bytes across `145` Markdown & Mermaid files |
| **CMSIS Register Ground Truth (`4.A`)** | `INFO/PASS` | `16/16` maps cite `RM0440` or `PM0214`/`ARMv7-M`; verified `87` bases, `344` struct offsets, `114` bit positions across `16` maps (`0` mismatches; explicit scope boundary reported) |
| **SOT DAG & `COMPLETE` Evidence (`4.B`)** | `INFO/PASS` + `WARNING` | `350` unique IDs, `27/27` domains, `0` cycles; `265/265` `COMPLETE` verified (`74` `PARTIAL` preserved untouched); **`WARNING (MANUAL REVIEW RECOMMENDED)`** flags `88` `COMPLETE` concepts (`18` hardware/bench-pending, `72` lacking explicit lab/driver/test path in `practical_implementation`; `2` in both) |
| **Practical Labs & Prerequisites (`4.C`)** | `INFO/PASS` | `20/20` labs verified with all 11 core sections + Clock Profile + Verification Record + explicit `Prerequisites` |
| **Local Markdown Links, `#anchors` & MathJax (`4.F`)** | `INFO/PASS` | `1,192/1,192` internal inline links, `12/12` `#anchor` GFM headings, `0` broken images/ref-links, `0` MathJax violations across `133` `.md` files |
| **External URL Probe (`4.E`)** | `INFO/PASS` / `UNVERIFIED` | `70/70` accessible (`2xx`), `0` redirected (`3xx`), `0` bot-blocked (`403/429`), `0` broken (`404/410`), `0` network/DNS/timeout failures (`UNVERIFIED` in default offline-safe mode) |
| **Mermaid Syntax vs Render (`4.D`)** | `INFO/PASS` + `UNVERIFIED` | `mermaid_syntax`: `12/12` passed static structural syntax validation; `mermaid_render`: `UNVERIFIED` (`'mmdc'` tooling unavailable on host) |
| Curriculum, Drivers, FreeRTOS, Capstones, Video DB (`4.E`) | `INFO/PASS` | `17/17` stages (`13/13` sections + primary refs + Prev/Next nav), `12/12` drivers, FreeRTOS V11.1.0, `3/3` Capstones, `video-database.csv` (`70` rows, `12/12` exact columns, `70` unique IDs & HTTPS URLs, `0` duplicate YouTube IDs) |
| **9 Independent Build / ELF / Test Stages (`4.G`)** | `INFO/PASS` (9/9) | `register_snippets`: `BUILD_PASSED` (`16/16`); `template_debug`: `BUILD_PASSED`; `template_release`: `BUILD_PASSED`; `firmware_elf`: `ELF_INSPECTION_PASSED` (`0x08000000`, `472 B`, `512 B` align); `freertos`: `BUILD_PASSED`; `capstone_01..03`: `BUILD_PASSED` (`3/3`); `host_tests`: `HOST_TESTS_PASSED` (`4` suites, `594` assertions, `0` failed; cleaned afterward) |

---

## 5. Verification Checklist (`v4.2.0` Final Closure)

- [x] **4.A:** Real `stm32g474xx.h` + `core_cm4.h` parser retained (`87` bases, `344` offsets, `114` bit positions, `16/16` C snippets), `16/16` RM0440/PM0214/ARMv7-M citations enforced, and explicit scope boundary printed (no overclaiming).
- [x] **4.B:** `PARTIAL` concepts are never auto-promoted (`74` untouched); transparent `WARNING (MANUAL REVIEW RECOMMENDED)` queue reports all `88` `COMPLETE` concepts with hardware-pending status (`18`) or lacking an explicit lab/driver/test path in `practical_implementation` (`72`).
- [x] **4.C:** Explicit `Prerequisites` check enforced as `ERROR` across all `20/20` practical labs (`20/20` pass).
- [x] **4.D:** Mermaid syntax validation (`mermaid_syntax`, `12/12` `INFO/PASS`) strictly separated from `mmdc` visual rendering (`mermaid_render`, explicitly `UNVERIFIED` when `mmdc` is absent).
- [x] **4.E:** Video CSV strict `12`-column schema, unique ID, and HTTPS URL format checks implemented; live URL probe tracks `2xx`, `3xx -> final URL` redirects, `403/429`, `404/410`, and offline resilience (`UNVERIFIED` when offline, never `ERROR`).
- [x] **4.F:** Local Markdown link/`#anchor`/image/ref-link/MathJax checks cleanly separated from external URL probing.
- [x] **4.G:** All 9 build, ELF inspection, and host unit-test stages execute independently with ZERO short-circuiting and emit exact 9-state taxonomy tags; proven via isolated fault injection (`REG-19`).
- [x] All 3 live modes (`--skip-builds`, default, `--check-external-urls`) executed cleanly with `0 ERROR` (exit code `0`).
