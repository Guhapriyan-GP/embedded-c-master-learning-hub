# MkDocs Material Learning Portal — Zero-Duplication Implementation Plan

> **Current Repository Status (Audited 2026-11-18):** `mkdocs.yml` does **not** currently exist in the repository root. Learners presently browse the curriculum directly via GitHub Flavored Markdown (GFM). This engineering plan specifies how to deploy a searchable, dark/light-mode **MkDocs Material** portal on **GitHub Pages** while keeping the GitHub repository as the **strict single source of truth (zero duplicated Markdown files)**.

---

## 1. Architectural Assessment & Single-Source-of-Truth Guarantee

### Why MkDocs Material Adds High Pedagogical Value
While GitHub's native Markdown viewer works well for single files, a 17-stage bare-metal curriculum + 20 labs + 7 learner handbooks benefits strongly from:
1. **Instant Full-Text Search** across all 17 stages, register maps, `RM0440`/`ES0430` errata notes, and 250+ Concept IDs.
2. **Persistent Sidebar in Canonical Acyclic Order** — eliminating any visual confusion caused by physical folder prefixes (`08-gpio/` appearing *before* `07-rcc-and-clocks/`, and `10-interrupts/` appearing *before* `09-timers/`).
3. **Unified MathJax v3 + Mermaid Rendering** — rendering `$ ... $` / `$$ ... $$` equations and Mermaid diagrams cleanly without GFM edge-case quirks.
4. **Mobile/Tablet & Offline Desk Reference Mode** — sticky Table of Contents, copy-to-clipboard code buttons, and dark/light theme toggle beside VS Code and GDB.

### Option Comparison: Avoiding Content Duplication

| Strategy | Mechanism | Windows 11 / CI Portability | Relative Link Parity (GitHub + Site) | Verdict |
|---|---|---|---|---|
| **Option A (Recommended): Direct Root Mount (`docs_dir: .`)** | `mkdocs.yml` sets `docs_dir: .` and `exclude_docs:` filters out C sources, build artifacts, `.git/`, `.github/`, and scripts. Explicit `nav:` mounts existing files directly. | **100% Native** (Windows 11, Linux, macOS, GitHub Actions — zero symlinks required!) | **100% Identical** (`../08-gpio/README.md` works identically on GitHub and in MkDocs 1.6+!) | **RECOMMENDED** |
| **Option B: Symlinks inside `docs/`** | `docs/01b -> ../01b-...` | Fragile on Windows 11 without Developer Mode / elevated `mklink` privileges. | Risk of broken `../` traversal across symlink boundaries. | Reject |
| **Option C: Copy Markdown into `docs/` at CI build time** | Python script copies `.md` into temp `docs/` folder before `mkdocs build`. | Works in CI, but breaks `mkdocs serve` live-reload during local authoring. | Requires path-rewriting script. | Fallback only |

**Engineering Decision:** Use **Option A (`docs_dir: .` with `exclude_docs` + explicit acyclic `nav:`)**. Zero Markdown files are ever copied or moved; editing any stage `README.md` immediately updates both GitHub and the published MkDocs Material site.

---

## 2. MathJax & Mermaid Compatibility Audit

1. **GitHub MathJax vs. `pymdownx.arithmatex` Parity:**
   - All owned Markdown files in this repository have been audited and remediated to have **0 underscores (`_` or `\_`) inside `\text{...}`, `\mathrm{...}`, `\texttt{...}`, or bare `\_` inside math blocks** (preventing GitHub's GFM pre-unescaping crash `'_' allowed only in math mode`).
   - Configuring `pymdownx.arithmatex` with `generic: true` + MathJax v3 (`tex-mml-chtml.js`) renders the exact same `$ ... $` and `$$ ... $$` delimiters cleanly with zero syntax changes.
2. **Mermaid Diagrams:**
   - **Recommended:** Use MkDocs Material's **built-in native Mermaid integration** via `pymdownx.superfences` custom fence (`name: mermaid`, `class: mermaid`, `format: !!python/name:pymdownx.superfences.fence_code_format`), which requires zero extra third-party plugins and automatically adapts to dark/light theme toggles. *(Alternatively, `mkdocs-mermaid2-plugin` can be used if custom Mermaid v10/v11 config overrides are needed.)*
   - Standalone `.mmd` files (`01-master-concept-map/concept-map.mmd`, `dependency-graph.mmd`) can be rendered on a Markdown page using `pymdownx.snippets` (`--8<-- "01-master-concept-map/dependency-graph.mmd"`) wrapped in a ```` ```mermaid ```` fence — keeping the `.mmd` source as the single source of truth.

---

## 3. Ready-to-Use `mkdocs.yml` Blueprint (Zero Duplication)

Save the following configuration at the repository root (`mkdocs.yml`) when ready to enable GitHub Pages:

```yaml
site_name: Embedded C Master Learning Hub (STM32G474RE)
site_description: Bare-metal ISO C11, ARM Cortex-M4F, STM32G474RE (NUCLEO-G474RE), FreeRTOS & Production Firmware Curriculum
docs_dir: .
site_dir: site
use_directory_urls: true

# Exclude non-documentation build artifacts, C code, and internal tooling from site output
exclude_docs: |
  site/
  .git/
  .github/
  .vscode/
  tests/
  scripts/
  23-code-library/**/build/
  **/*.c
  **/*.h
  **/*.s
  **/*.S
  **/*.ld
  **/*.o
  **/*.d
  **/*.su
  **/*.elf
  **/*.bin
  **/*.hex
  **/*.map
  **/*.csv
  **/*.py
  Makefile
  .gdbinit
  .gitignore

theme:
  name: material
  language: en
  palette:
    - scheme: default
      primary: indigo
      accent: teal
      toggle:
        icon: material/brightness-7
        name: Switch to dark mode
    - scheme: slate
      primary: indigo
      accent: teal
      toggle:
        icon: material/brightness-4
        name: Switch to light mode
  features:
    - navigation.tabs
    - navigation.sections
    - navigation.path
    - navigation.top
    - navigation.footer
    - search.suggest
    - search.highlight
    - content.code.copy

markdown_extensions:
  - admonition
  - attr_list
  - def_list
  - footnotes
  - tables
  - toc:
      permalink: true
  - pymdownx.arithmatex:
      generic: true
  - pymdownx.details
  - pymdownx.highlight:
      anchor_linenums: true
  - pymdownx.inlinehilite
  - pymdownx.snippets
  - pymdownx.superfences:
      custom_fences:
        - name: mermaid
          class: mermaid
          format: !!python/name:pymdownx.superfences.fence_code_format
  - pymdownx.tasklist:
      custom_checkbox: true

plugins:
  - search
  # Optional alternative to native Material Mermaid fence:
  # - mermaid2

extra_javascript:
  - LEARNERS/assets/mathjax-config.js
  - https://unpkg.com/mathjax@3/es5/tex-mml-chtml.js

# Canonical Acyclic Learning Order (strictly 08-gpio before 07-rcc; 10-interrupts before 09-timers)
nav:
  - Start Here:
      - Overview: README.md
      - Beginner Entry Guide: START_HERE.md
      - Curriculum Roadmap: ROADMAP.md
      - Master Concept Map:
          - Architecture Overview: 01-master-concept-map/overview.md
          - Sequential Learning Roadmap: 01-master-concept-map/learning-sequence.md
          - Prerequisite Matrix: 01-master-concept-map/prerequisite-matrix.md
  - Curriculum (Acyclic Order):
      - "01. Digital Electronics & HW Integration (01b)": 01b-digital-electronics-and-hw-integration/README.md
      - "02. C Language Foundations (02)": 02-c-fundamentals/README.md
      - "02B. Advanced C, UB & Memory Model": 02-c-fundamentals/advanced-c-ub-memory-model.md
      - "03. Embedded C & MMIO (03)": 03-embedded-c/README.md
      - "04. ARM Cortex-M4F Core (04)": 04-arm-cortex-m/README.md
      - "05. GNU Toolchain, Linker & Debugging (05)": 05-toolchain-and-debugging/README.md
      - "06. STM32G474RE Architecture & Errata (06)": 06-stm32-fundamentals/README.md
      - "07. Basic GPIO on 16 MHz HSI16 (08)": 08-gpio/README.md
      - "08. PWR Boost, Flash 4WS & 170/160 MHz RCC (07)": 07-rcc-and-clocks/README.md
      - "09. Interrupts, SysTick, NVIC & EXTI (10)": 10-interrupts/README.md
      - "10. Hardware Timers, PWM & HRTIM1 (09)": 09-timers/README.md
      - "11. Communication Protocols (11)": 11-communication/README.md
      - "12. Mixed-Signal Analog & Math (12)": 12-adc-dac-analog/README.md
      - "13. DMA1/2 & DMAMUX1 (13)": 13-dma/README.md
      - "14. Dual-Bank Flash, ECC & Bootloaders (14)": 14-memory-and-storage/README.md
      - "15. Watchdogs, RTC & Low-Power Modes (15)": 15-power-management/README.md
      - "16. FreeRTOS ARM_CM4F & Scheduling (16)": 16-rtos/README.md
      - "17. Advanced Firmware Engineering & CI (17)": 17-advanced-firmware-engineering/README.md
  - Learner Toolkit:
      - Windows 11 Setup Guide: LEARNERS/BEGINNER_SETUP_WINDOWS11.md
      - Progress Dashboard: LEARNERS/LEARNER_PROGRESS_DASHBOARD.md
      - Worked Examples: LEARNERS/WORKED_EXAMPLES.md
      - Formula & Calculation Sheet: LEARNERS/FORMULA_AND_CALCULATION_SHEET.md
      - Register Quick Reference: LEARNERS/REGISTER_QUICK_REFERENCE.md
      - Troubleshooting Guide: LEARNERS/TROUBLESHOOTING_GUIDE.md
      - Revision Notes & Mastery Gates: LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md
      - MkDocs Portal Plan: LEARNERS/MKDOCS_PORTAL_PLAN.md
  - Labs & Projects:
      - Practical Laboratory Index: 18-practical-laboratory/README.md
      - Capstone Projects: 19-projects/README.md
      - Reusable Code Library: 23-code-library/README.md
  - Reference & Assessments:
      - Register Maps & Diagrams: 20-technical-diagrams/README.md
      - Official ST Documentation: 21-reference-library/official-st-documentation.md
      - ARM Architecture References: 21-reference-library/arm-architecture-references.md
      - Industry Standards (MISRA / CERT / Barr): 21-reference-library/industry-standards-misra-barr.md
      - Open-Source Toolchain Utilities: 21-reference-library/github-tools.md
      - Textbooks: 21-reference-library/embedded-c-textbooks.md
      - Video Library Index: 22-video-library/video-index.md
      - Video Coverage Matrix: 22-video-library/topic-coverage-matrix.md
      - Mastery Assessments: 24-assessments/README.md
      - Answer Keys & Rubrics: 24-assessments/answer-keys-and-rubrics.md
      - Glossary & Register Abbreviations: 25-glossary/README.md
```

### Companion MathJax Configuration (`LEARNERS/assets/mathjax-config.js`)
```javascript
window.MathJax = {
  tex: {
    inlineMath: [["\\(", "\\)"], ["$", "$"]],
    displayMath: [["\\[", "\\]"], ["$$", "$$"]],
    processEscapes: true,
    processEnvironments: true
  },
  options: {
    ignoreHtmlClass: ".*|",
    processHtmlClass: "arithmatex"
  }
};

document$.subscribe(() => {
  MathJax.startup.output.clearCache();
  MathJax.typesetClear();
  MathJax.texReset();
  MathJax.typesetPromise();
});
```

---

## 4. Automated GitHub Pages Deployment (`.github/workflows/docs.yml`)

Use GitHub's official Pages OIDC actions (no `gh-pages` branch bloat) and enforce `mkdocs build --strict` in CI so broken internal links fail the PR immediately:

```yaml
name: Build & Deploy MkDocs Material Portal

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: pages
  cancel-in-progress: true

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"
      - name: Install MkDocs Material
        run: pip install --no-cache-dir mkdocs-material==9.* pymdown-extensions==10.*
      - name: Build & Validate Internal Links (--strict)
        run: mkdocs build --strict
      - name: Upload GitHub Pages Artifact
        if: github.ref == 'refs/heads/main'
        uses: actions/upload-pages-artifact@v3
        with:
          path: site

  deploy:
    if: github.ref == 'refs/heads/main'
    needs: build
    runs-on: ubuntu-latest
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

---

## 5. Local Windows 11 Preview & Pre-Deployment Checklist

### Local Preview Commands (PowerShell)
```powershell
python -m pip install mkdocs-material pymdown-extensions
mkdocs build --strict    # Fails on any broken internal relative Markdown link
mkdocs serve             # Live preview at http://127.0.0.1:8000
```

### Pre-Flight Checklist Before Enabling Pages
- [ ] Add `site/` to `.gitignore` so local builds are never committed.
- [ ] Create `mkdocs.yml` at repository root and `LEARNERS/assets/mathjax-config.js`.
- [ ] Run `mkdocs build --strict` and resolve any warnings for links targeting `.c`/`.h`/`.ld`/`.csv` files excluded by `exclude_docs` (convert code file links in Markdown to full GitHub blob URLs or keep source links unexcluded via `mkdocs-exclude` only for `build/`).
- [ ] Spot-check MathJax formulas on `07-rcc-and-clocks/`, `12-adc-dac-analog/`, and `LEARNERS/FORMULA_AND_CALCULATION_SHEET.md`, plus Mermaid diagrams and `navigation.footer` Prev/Next order (`08-gpio/` $\to$ `07-rcc-and-clocks/` $\to$ `10-interrupts/` $\to$ `09-timers/`).
- [ ] Enable **Settings → Pages → Source: GitHub Actions** on the GitHub repository.

---

> **Navigation:** [`START_HERE.md`](../START_HERE.md) | [`LEARNER_PROGRESS_DASHBOARD.md`](./LEARNER_PROGRESS_DASHBOARD.md) | [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md)
