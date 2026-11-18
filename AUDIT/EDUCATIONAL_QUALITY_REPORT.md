# Educational Quality, GitHub MathJax Rendering, Navigation & External Resources Audit Report

**Audit Scope:** Specialist 3 (Agents 10 & 11 — Educational Quality, GitHub MathJax Rendering, Sequential Navigation, External Resources & MkDocs Assessment)  
**Repository:** `<repo-root>/` (Branch: `feature/complete-remediation`)  
**Target Platform:** STM32G474RET6 (LQFP64, ARM Cortex-M4F @ 170 / 160 MHz) on NUCLEO-G474RE (MB1367, `UM2505`)  
**Audit & Remediation Date:** 2026-11-18  
**Verification Honesty:** All Markdown rendering audits, relative-link validations, and external HTTP/oEmbed checks were executed programmatically via Python. No code was executed on physical NUCLEO-G474RE hardware during this documentation audit.

---

## 1. Executive Summary

| Audit Dimension | Pre-Remediation State | Post-Remediation State | Verification Evidence |
|---|---|---|---|
| **GitHub MathJax Rendering (`_` / `\_` in `\text{}` / `\texttt{}` / math)** | **86 defects** across **15 owned files** (causing GitHub MathJax crash: `'_' allowed only in math mode`, e.g., `$f_{\text{PLL\_IN}}$` in `learning-sequence.md` L69) | **0 defects** across **43 owned `.md` files** (**86 / 86 fixed**) | Automated braced-command + math-span Python scanner (`deep_mathjax_audit.py`) |
| **Raw ASCII Control Characters** | 0 in current owned `.md` files | **0** | Automated byte-level scan (`ord(c) < 32` excl. `\n`, `\r`, `\t`) |
| **Sequential Navigation Bars (Top + Bottom)** | 0 / 14 owned stage docs had Prev/Next/Home/Prereq bars | **14 / 14 owned stage docs** updated (13 stage `README.md` + `02-c-fundamentals/advanced-c-ub-memory-model.md`) | Automated link-existence check — 100% of relative nav links resolve on disk |
| **Canonical Roadmap & Concept Map (`01-master-concept-map/`)** | Missing `01b` link in Stage 00; Stage 06B omitted 160 MHz profile; Stage 07 omitted ARM AN321 readback before `__DSB()` | All 3 files (`learning-sequence.md`, `overview.md`, `prerequisite-matrix.md`) reconciled with clickable links, **170 & 160 MHz** profiles, and **ARM AN321** readback + `__DSB()` | Direct file inspection + relative link check |
| **MkDocs Material Learning Portal Assessment** | No `mkdocs.yml` or portal blueprint existed | Assessed (`mkdocs.yml` absent) and authored zero-duplication blueprint [`LEARNERS/MKDOCS_PORTAL_PLAN.md`](../LEARNERS/MKDOCS_PORTAL_PLAN.md) (`docs_dir: .` + `exclude_docs` + MathJax v3 + Mermaid + Pages OIDC CI) | Linked from [`START_HERE.md`](../START_HERE.md) |
| **External URL & Resource Audit (`21-reference-library/` & `22-video-library/`)** | Unverified live link status; `arm-architecture-references.md` omitted ARM AN321 (`DAI 0321A`) & `DDI 0439D` r0p1 clarity | **4 / 4** `21-reference-library/` external URLs **ACCESSIBLE (HTTP 200)**; **70 / 70** `22-video-library/video-database.csv` YouTube URLs **ACCESSIBLE (HTTP 200 oEmbed)**; `0` broken; `0` unmapped in matrix | Live Python HTTP + YouTube oEmbed audit (`check_urls.py`, `check_all_urls.py`) |

---

## 2. GitHub MathJax Rendering Audit & Remediation (`86 -> 0`)

### Root Cause Analysis
GitHub Flavored Markdown (GFM) runs Markdown backslash-unescaping **before** MathJax parses `$ ... $` and `$$ ... $$` expressions. Consequently, writing `\text{PLL\_IN}`, `\text{TIM\_CLK}`, `\texttt{0010\_1010}`, or `t_{S\_\text{temp}}` causes GFM to pass `\text{PLL_IN}` to MathJax, which aborts rendering with **`'_' allowed only in math mode`**.

### Exact File-by-File Remediation Counts (Owned Scope: 43 `.md` Files)

| File (Relative to Repo Root) | Issues Found | Issues Fixed | Remaining | Examples of Applied MathJax/GFM/KaTeX-Safe Replacements |
|---|---:|---:|---:|---|
| `01-master-concept-map/learning-sequence.md` | 1 | 1 | **0** | `\text{PLL\_IN}` $\to$ `\text{PLL-IN}` |
| `02-c-fundamentals/README.md` | 4 | 4 | **0** | `\text{uint32\_t}` $\to$ `\bmod 2^{32}`; `\text{\_Alignof}` $\to$ `\text{alignof}`; `\text{current\_offset}` / `\text{end\_offset}` $\to$ `\text{offset}_{\text{curr}}` / `\text{offset}_{\text{end}}` |
| `03-embedded-c/README.md` | 10 | 10 | **0** | `\text{uint32\_t}` $\to$ `\bmod 2^{32}`; `\text{is\_empty}`/`\text{is\_full}` $\to$ `\text{empty}`/`\text{full}`; `\text{BASEPRI\_REG}` / `\text{\_\_NVIC\_PRIO\_BITS}` $\to$ `\text{BASEPRI}`, $N_{\text{prio}}$; `\text{min\_masked\_prio}` $\to$ $p_{\text{mask}}$ |
| `05-toolchain-and-debugging/README.md` | 9 | 9 | **0** | `\text{SWO\_Baud}` $\to$ $f_{\text{SWO}}$; `\text{.isr\_vector}` $\to$ $S_{\text{vectors}}$; `\text{.\_user\_heap\_stack}` $\to$ $S_{\text{heap+stack}}$; `\text{main\_call\_chain}`/`\text{\_Min\_Stack\_Size}` $\to$ $S_{\text{main}}$ / $S_{\text{stack budget}}$ |
| `06-stm32-fundamentals/README.md` | 1 | 1 | **0** | `\text{TIMx\_CLK}` $\to$ `\text{TIMx-CLK}` |
| `07-rcc-and-clocks/README.md` | 14 | 14 | **0** | `\text{PLL\_IN}`/`\text{PLL\_R}`/`\text{PLL\_Q}`/`\text{PLL\_P}` $\to$ `\text{PLL-IN}`/`\text{PLL-R}`/`\text{PLL-Q}`/`\text{PLL-P}` |
| `12-adc-dac-analog/README.md` | 8 | 8 | **0** | `\text{VREFINT\_CAL}`/`\text{TS\_CAL1}`/`\text{TS\_CAL2}`/`\text{ADCVREG\_STUP}` $\to$ hyphenated; `t_{S\_\text{temp}}` $\to$ `t_{S,\text{temp}}` (3 occurrences) |
| `13-dma/README.md` | 1 | 1 | **0** | `\text{ISR\_overhead}` $\to$ `\text{ISR ovh}` |
| `17-advanced-firmware-engineering/README.md` | 6 | 6 | **0** | `\text{main\_peak}`/`\text{hw\_frame}`/`\text{ISR\_local}` $\to$ comma subscripts; `\text{\_Min\_Stack\_Size}` $\to$ $S_{\text{stack budget}}$; `(uint32\_t)(...)` $\to$ `(...) \bmod 2^{32}` |
| `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md` | 21 | 21 | **0** | `\text{PLL\_*}`, `\text{TIM\_CLK}`, `\text{KER\_PRESC}`, `\text{VREFINT\_CAL}`, `\text{TS\_CAL1/2}`, `\text{alias\_addr}`/`\text{alias\_base}`/`\text{byte\_offset}`/`\text{bit\_num}` $\to$ clean hyphen/space/comma forms |
| `LEARNERS/TROUBLESHOOTING_GUIDE.md` | 1 | 1 | **0** | `\text{PLL\_IN}` $\to$ `\text{PLL-IN}` |
| `LEARNERS/WORKED_EXAMPLES.md` | 1 | 1 | **0** | `\text{PLL\_IN}` $\to$ `\text{PLL-IN}` |
| `21-reference-library/official-st-documentation.md` | 1 | 1 | **0** | `\text{PLL\_IN}` $\to$ `\text{PLL-IN}` |
| `24-assessments/answer-keys-and-rubrics.md` | 5 | 5 | **0** | `\texttt{0010\_1010}_2` $\to$ `\texttt{0010\,1010}_2`; `\texttt{sizeof(uint32\_t)}` moved to inline code + `$4 \times 4$` |
| `24-assessments/stage-04-stm32-peripherals-exam.md` | 3 | 3 | **0** | `\text{PLL\_IN}`, `\text{PLL\_P}`, `\text{TIM\_CLK}` $\to$ `\text{PLL-IN}`, `\text{PLL-P}`, `\text{TIM-CLK}` |
| **All other 28 owned `.md` files** | 0 | 0 | **0** | Already clean (`0` defects) |
| **TOTAL (43 owned `.md` files)** | **86** | **86** | **0** | **100% MathJax / GFM / KaTeX Clean** |

---

## 3. Educational Progression & Sequential Navigation Coverage

### Canonical Acyclic Sequence Enforced Everywhere
To eliminate circular hardware dependencies, every navigation bar, roadmap entry, and MkDocs `nav:` blueprint strictly enforces:
```text
01b -> 02 (+ 02B) -> 03 -> 04 -> 05 -> 06 -> 08 [Basic GPIO @ 16 MHz HSI16] -> 07 [170/160 MHz PWR/Flash/RCC]
    -> 10 [SysTick/NVIC/EXTI] -> 09 [Timers/PWM/HRTIM1] -> 11 [Comms] -> 12 [Analog] -> 13 [DMA]
    -> 14 [Flash/Boot/Security] -> 15 [Watchdogs/Low-Power] -> 16 [FreeRTOS] -> 17 [Adv FW & CI]
```

### Navigation Bar Deployment Status
- **Updated with Top + Bottom Sequential Navigation Bars (14 files in Specialist 3 scope):**
  1. `01b-digital-electronics-and-hw-integration/README.md` *(Step 1 of 17)*
  2. `02-c-fundamentals/README.md` *(Step 2 of 17)*
  3. `02-c-fundamentals/advanced-c-ub-memory-model.md` *(Step 2B of 17 — Companion Deep-Dive)*
  4. `03-embedded-c/README.md` *(Step 3 of 17)*
  5. `05-toolchain-and-debugging/README.md` *(Step 5 of 17)*
  6. `06-stm32-fundamentals/README.md` *(Step 6 of 17 — Next points directly to `08-gpio/`!)*
  7. `07-rcc-and-clocks/README.md` *(Step 8 of 17 — Prev: `08-gpio/`, Next: `10-interrupts/`!)*
  8. `11-communication/README.md` *(Step 11 of 17)*
  9. `12-adc-dac-analog/README.md` *(Step 12 of 17)*
  10. `13-dma/README.md` *(Step 13 of 17)*
  11. `14-memory-and-storage/README.md` *(Step 14 of 17)*
  12. `15-power-management/README.md` *(Step 15 of 17)*
  13. `16-rtos/README.md` *(Step 16 of 17)*
  14. `17-advanced-firmware-engineering/README.md` *(Step 17 of 17)*
- **Peer-Owned Modules (Completed in Wave 1 by Specialist 1 — 17/17 Complete):**
  - `04-arm-cortex-m/README.md` *(Step 4 of 17)*, `08-gpio/README.md` *(Step 7 of 17)*, `10-interrupts/README.md` *(Step 9 of 17)*, and `09-timers/README.md` *(Step 10 of 17)* now carry matching Top + Bottom Sequential Navigation Bars, yielding **17/17 (100%)** verified stage navigation coverage (`scripts/validate_repo.py` v4.1.0 `curriculum_nav`: `17/17 INFO/PASS`).

### Concept Map & Roadmap Corrections (`01-master-concept-map/`)
- **`learning-sequence.md`:** Fixed `$f_{\text{PLL\_IN}}$` MathJax bug; added clickable folder link for Stage 00 (`01b-digital-electronics-and-hw-integration/`) and all 17 stages/labs; expanded Stage 06B (`07-rcc-and-clocks/`) to document both **170 MHz** ($M=6/N=85/R=2$ HSE or $M=4/N=85/R=2$ HSI16) and **160 MHz** ($M=3/N=40/R=2$ or $M=6/N=80/R=2$ HSE) Range 1 Boost profiles; updated Stage 07 (`10-interrupts/`) to state exact **ARM AN321** peripheral status-register readback (`(void)EXTI->PR1;`) followed by `__DSB()` before ISR exit.
- **`overview.md` & `prerequisite-matrix.md`:** Added clickable `01b-digital-electronics-and-hw-integration/` link, added Labs 16–20 mappings, updated Stage 06B to reflect both 170 and 160 MHz profiles, and updated NVIC/EXTI prerequisite rationale with ARM AN321 readback + `__DSB()`.

---

## 4. MkDocs Material Portal Assessment (`LEARNERS/MKDOCS_PORTAL_PLAN.md`)

- **Current State:** `mkdocs.yml` does **not** exist in the repository root.
- **Assessment Verdict:** **Strongly Recommended** as a zero-duplication GitHub Pages overlay.
- **Deliverable Created:** [`LEARNERS/MKDOCS_PORTAL_PLAN.md`](../LEARNERS/MKDOCS_PORTAL_PLAN.md) (also linked in [`START_HERE.md`](../START_HERE.md) Section 6), specifying:
  - Direct root mount (`docs_dir: .` + `exclude_docs:`) — zero copied Markdown files, 100% cross-platform on Windows 11/Linux/CI, preserving GitHub as the single source of truth.
  - Explicit `nav:` matching the canonical acyclic learning order (`08-gpio/` before `07-rcc-and-clocks/`, `10-interrupts/` before `09-timers/`).
  - Full `pymdownx.arithmatex` + MathJax v3 configuration and native MkDocs Material Mermaid (`pymdownx.superfences` custom fence) parity.
  - Ready-to-deploy GitHub Actions Pages OIDC workflow (`.github/workflows/docs.yml`) with `mkdocs build --strict` link checking.

---

## 5. External URL & Resource Verification Audit

### 5.1 Reference Library (`21-reference-library/*.md`)
Live Python HTTP verification (`HEAD` with `GET` fallback, browser `User-Agent`, 12 s timeout) across all external URLs in `21-reference-library/`:

| File & Line | URL | HTTP Code | Classification |
|---|---|---:|---|
| `21-reference-library/github-tools.md` L25 | `https://github.com/openocd-org/openocd` | `200` | **ACCESSIBLE (2xx/3xx)** |
| `21-reference-library/github-tools.md` L32 | `https://github.com/probe-rs/probe-rs` | `200` | **ACCESSIBLE (2xx/3xx)** |
| `21-reference-library/github-tools.md` L58 | `https://github.com/renode/renode` | `200` | **ACCESSIBLE (2xx/3xx)** |
| `21-reference-library/github-tools.md` L60 | `https://sigrok.org` | `200` | **ACCESSIBLE (2xx/3xx)** |

- **Summary (`21-reference-library/`):** **4 / 4 ACCESSIBLE (200)** | **0 BOT-BLOCKED/403** | **0 BROKEN (404/DNS)** | **0 UNVERIFIED**.
- **Reference Documentation Accuracy Improvements Applied:**
  - `21-reference-library/arm-architecture-references.md`: Clarified Cortex-M4 TRM revision (`ARM DDI 0439D` for `r0p1`, matching STM32G474RE `__CM4_REV = 0x0001U`, alongside `DDI 0439F`), and added **ARM Application Note 321 (AN321 / `ARM DAI 0321A`)** covering memory barriers and peripheral readback + `__DSB()` before ISR exit.
  - `21-reference-library/official-st-documentation.md`: Fixed MathJax `$f_{\text{PLL\_IN}}$` bug on L43 and updated ES0430 §2.1.3 entry on L49 to cite ARM AN321 status-register readback + `__DSB()`.

### 5.2 Video Library (`22-video-library/`)
Live YouTube oEmbed API verification (`https://www.youtube.com/oembed?format=json`) across all rows in `22-video-library/video-database.csv`:
- **Total Video Entries:** **70**
- **ACCESSIBLE (HTTP 200 oEmbed):** **70 / 70 (100%)**
- **BOT-BLOCKED/403:** **0**
- **BROKEN (404/DNS):** **0**
- **UNVERIFIED:** **0**
- **Internal Cross-Consistency:** `video-database.csv` (70 unique URLs) matches `video-index.md` (70 unique URLs, symmetric difference = $\emptyset$), and **0 / 70** video IDs are missing from `topic-coverage-matrix.md`.

---

## 6. Post-Merge Status & Remaining Pedagogical Roadmap

1. **17/17 Sequential Stage Navigation Bars — COMPLETE:** All `17/17` stages (including `04-arm-cortex-m`, `08-gpio`, `10-interrupts`, and `09-timers`) now deploy verified Top + Bottom Prev/Next traversal bars enforcing the canonical acyclic bring-up order (`validate_repo.py` `curriculum_nav`: `17/17 INFO/PASS`).
2. **Automated GitHub MathJax CI Guard — ACTIVE (`scripts/validate_repo.py` v4.1.0):** `scripts/validate_repo.py` v4.1.0 permanently enforces zero `_` / `\_` inside `\text{}`/`\mathrm{}`/`\texttt{}`/`\textbf{}`/`\textit{}`/`\mbox{}`, zero bare `\_` inside `$...$`/`$$...$$`, and balanced braces / closed `$` math spans across all `.md` files (`0` violations repo-wide).
3. **Physical NUCLEO-G474RE Onboard Hardware Validation — COMPLETE (External Bench Pending):** Safe onboard NUCLEO-G474RE testing (`9/9` self-test, `6/6` FreeRTOS V11.1.0, `3/3` Capstones, `13/20` labs, `122` SoT concepts) is complete with verbatim `COM8` evidence in [`AUDIT/HARDWARE_VALIDATION_SUMMARY.md`](HARDWARE_VALIDATION_SUMMARY.md) and [`AUDIT/LAB_VERIFICATION_MATRIX.md`](LAB_VERIFICATION_MATRIX.md); the remaining `6` external-wiring labs (`07, 09, 11, 12, 14, 16`) and `5` bench-instrument concepts are tracked honestly as `PENDING — USER CONNECTION REQUIRED` in [`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`](../HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md).
4. **Optional Next Enhancement — Deploy the MkDocs Material Portal:** When ready to publish a static web portal, commit `mkdocs.yml`, `LEARNERS/assets/mathjax-config.js`, and `.github/workflows/docs.yml` from [`LEARNERS/MKDOCS_PORTAL_PLAN.md`](../LEARNERS/MKDOCS_PORTAL_PLAN.md), run `mkdocs build --strict`, and enable GitHub Pages.
