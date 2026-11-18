# CI/CD, Supply Chain, SBOM & Security Publication Hardening Report

**Repository:** `embedded-c-master-learning-hub`  
**Audit Stream:** Section 10 (CI/CD Verification Architecture) & Section 12 (Supply Chain, Security & Public Release Hygiene)  
**Target Platform:** STMicroelectronics **STM32G474RET6** on **NUCLEO-G474RE** (`MB1367 Rev C`, ARM Cortex-M4F)  
**Concept IDs:** `TOOL-CI-001`, `TOOL-QA-001`, `TOOL-STATIC-001`, `PROF-CI-001`, `PROF-SC-001`, `SEC-BOOT-001`, `SEC-BOOT-002`, `MEM-OB-001`, `REL-FAULT-001`  
**Verification Honesty Notice:** YAML syntax (`PyYAML 6.0.3`), deterministic SPDX 2.3 SBOM generation/drift verification (`scripts/generate_sbom.py --check`), and host baseline tests (`594/594` assertions) were executed locally on Windows. GitHub-hosted `ubuntu-24.04` cloud CI steps (including `ASan+UBSan` linking against Linux `libasan`/`libubsan`, `cppcheck`, `clang-tidy`, `doxygen`, and `@mermaid-js/mermaid-cli@11.4.2`) are **Statically Reviewed & Workflow-Verified** and will execute live upon push to GitHub. Physical NUCLEO-G474RE silicon flashing remains strictly decoupled from cloud CI.

---

## 1. Executive Summary

| Hardening Domain | Deliverable(s) | Status | Verification Evidence |
|---|---|---|---|
| Security Policy & Hardware Safety | [`SECURITY.md`](../SECURITY.md) | **CREATED** | Covers supported versions, GitHub Security Advisories, RM0440 RDP/WRP/option-byte safety, DS12288/UM2505 electrical limits (`JP6` vs `JP5`, `PB8-BOOT0`, `POWER_SIM_MODE`), and public secret/PII/serial hygiene |
| Third-Party Licensing | [`THIRD_PARTY_LICENSES.md`](../THIRD_PARTY_LICENSES.md) | **CREATED** | Exact version, license, copyright, and path inventory for ARM CMSIS-Core (`5.6.0`, `Apache-2.0`), ST STM32G4 CMSIS Device (`V1.2.6`, `BSD-3-Clause`), and FreeRTOS-Kernel (`V11.1.0`, `MIT`) |
| Deterministic SPDX 2.3 SBOM | [`scripts/generate_sbom.py`](../scripts/generate_sbom.py), [`SBOM/spdx-sbom.json`](../SBOM/spdx-sbom.json) | **VERIFIED** | `python scripts/generate_sbom.py --check` exit `0`: `4` packages (`3` vendored third-party), `43` files, `47` relationships, LF-canonicalized SHA-1/SHA-256 + SPDX `packageVerificationCode` |
| Dependabot Supply-Chain Automation | [`.github/dependabot.yml`](../.github/dependabot.yml) | **YAML-VALID** | Weekly Monday `06:00 UTC` updates for `github-actions`, PR cap `5`, grouped updates, `ci(deps)` commit prefix |
| Hardened 4-Layer Cloud CI | [`.github/workflows/firmware-ci.yml`](../.github/workflows/firmware-ci.yml) | **YAML-VALID** | `permissions: contents: read`, `ubuntu-24.04` pinned, `100%` 40-char SHA-pinned Actions, ASan+UBSan, `>= 90%` coverage gate, full firmware matrix + `onboard_selftest`, SBOM check, pinned `mmdc@11.4.2` |
| Public-Release Sanitization Inventory | Section 7 below (routed to Wave 2) | **AUDITED** | Exact scan confirmed `7` STLINK-V3E serial hits (`6` files), `28` local workspace prefix hits (`10` files), and `2` user-profile path hits (`1` file); `0` hits in new Stream 3 deliverables |

---

## 2. Verified GitHub Action Commit SHA Pinning

Every third-party GitHub Action invoked in [`.github/workflows/firmware-ci.yml`](../.github/workflows/firmware-ci.yml) is pinned to its immutable **40-character commit SHA** queried directly from the official GitHub REST API (`/repos/{owner}/{repo}/git/ref/tags/{tag}` and `/commits/{sha}`), eliminating mutable tag-repointing supply-chain risk:

| Action | Release Tag | Verified 40-Character Commit SHA | Commit Date (UTC) | Purpose in Workflow |
|---|---|---|---|---|
| `actions/checkout` | `v4.4.0` | `11d5960a326750d5838078e36cf38b85af677262` | `2026-07-16T19:43:47Z` | Repository checkout across all 4 CI layers |
| `actions/setup-python` | `v5.6.0` | `a26af69be951a213d495a4c3e4e4022e16d87065` | `2025-04-24T02:42:27Z` | Python `3.12` for coverage gate, ELF assertions, SBOM check, and repo validator |
| `actions/setup-node` | `v4.4.0` | `49933ea5288caeca8642d1e84afbd3f7d6820020` | `2025-04-02T19:20:51Z` | Node.js `20` for pinned `@mermaid-js/mermaid-cli@11.4.2` (`mmdc`) |
| `actions/upload-artifact` | `v4.6.2` | `ea165f8d65b6e75b540449e92b4886f43607fa02` | `2025-03-19T17:34:59Z` | Retention-bounded (`14`-day) upload of coverage, build summaries, Doxygen HTML, SBOM, and validation JSON |

**Additional Workflow Security Controls:**
- **Least-Privilege Token Permissions:** Top-level `permissions: contents: read` restricts `GITHUB_TOKEN` to read-only repository contents across every job.
- **Pinned Runner OS:** All jobs execute on `ubuntu-24.04` instead of floating `ubuntu-latest`.
- **Concurrency Control:** `cancel-in-progress: true` grouped by `${{ github.workflow }}-${{ github.ref }}`.
- **Automated Pin Maintenance:** [`.github/dependabot.yml`](../.github/dependabot.yml) monitors `github-actions` weekly (`monday` `06:00 UTC`, max `5` open PRs, grouped into `github-actions-updates`).

---

## 3. Section 10 Cleanly Separated 4-Layer Cloud CI Architecture

[`.github/workflows/firmware-ci.yml`](../.github/workflows/firmware-ci.yml) strictly separates cloud-verifiable layers from physical hardware layers:

| Layer | CI Job ID | Scope & Quality Gates | Execution Environment |
|---|---|---|---|
| **Layer 1** | `host-unit-tests-and-sanitizers` | 1. `make -C tests clean test` (all 5 host test suites incl. full peripheral register mocks)<br>2. **ASan + UBSan** (`-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`, `detect_leaks=1:abort_on_error=1`) across all 5 suites<br>3. `gcov` / `lcov` (`--rc branch_coverage=1 --ignore-errors inconsistent,gcov`) with **mandatory `>= 90.0%` aggregate driver line coverage gate** (baseline: `96.04%` raw / `100.00%` reachable) | Cloud CI (`ubuntu-24.04`) |
| **Layer 2** | `firmware-cross-compile-matrix` | 1. Starter template + 12 drivers (`debug` & `release`) + ELF `.isr_vector` assertion (`0x08000000`, `472 B` = 118 entries, `>= 512 B` aligned)<br>2. FreeRTOS `V11.1.0` (`debug` + `nm-check`, `release` + `size`)<br>3. All 3 Capstones (`01`, `02`, `03` with **both** `POWER_SIM_MODE=1` and `POWER_SIM_MODE=0` in `debug` & `release`)<br>4. `HARDWARE_VALIDATION/onboard_selftest` (`debug` & `release`)<br>5. Reproducible `firmware-build-summary.txt` + `firmware-sha256.txt` artifact upload | Cloud CI (`ubuntu-24.04`) |
| **Layer 3** | `static-analysis-and-sbom` | 1. Host `gcc -fanalyzer` (`make -C tests analyze`)<br>2. ARM `arm-none-eabi-gcc -fanalyzer` across all 12 drivers + starter template<br>3. `cppcheck` (`warning,performance,portability`, `--error-exitcode=1`)<br>4. `clang-tidy` (root `.clang-tidy`)<br>5. `doxygen Doxyfile` (`WARN_AS_ERROR = YES`)<br>6. `python3 scripts/generate_sbom.py --check` (zero-drift SBOM gate)<br>7. Advisory `clang-format --dry-run` diff report artifact (transparently non-blocking until a dedicated formatting PR is approved) | Cloud CI (`ubuntu-24.04`) |
| **Layer 4** | `mermaid-and-repo-validation` | 1. Installs pinned **`@mermaid-js/mermaid-cli@11.4.2`** (`mmdc`) + Ubuntu 24.04 Chromium libs (`apparmor_restrict_unprivileged_userns=0`)<br>2. Executes `python3 scripts/validate_repo.py --skip-builds --json-report build/validation-report.json`, exercising live `mermaid_render` SVG compilation alongside SOT, register-map, link, and curriculum checks | Cloud CI (`ubuntu-24.04`) |
| **Layer 5** | *(Decoupled — Not in Cloud CI)* | Onboard NUCLEO-G474RE silicon flashing & self-test (`HARDWARE_VALIDATION/onboard_selftest`, FreeRTOS `6/6`, Capstones `01–03`) | Physical Bench Only (no self-hosted HIL runner) |
| **Layer 6** | *(Decoupled — Not in Cloud CI)* | External breadboard peripherals, `JP6` current meter, oscilloscope, logic analyzer, and external power stage (`HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`) | Physical Bench Only |

### Sanitizer Local vs Cloud Transparency Note
- Local Windows MSYS2 (`gcc 15.2.0`) compiles all 4 test suites cleanly with `-fsanitize=address,undefined` syntax/warning flags, but Windows MinGW-w64 does not ship `libasan` (`ld: cannot find -lasan`).
- On `ubuntu-24.04` GitHub Actions runners, GNU GCC natively links `libasan` and `libubsan`, executing all `594` assertions under leak, out-of-bounds, use-after-free, signed-overflow, shift, and alignment sanitizer checks.

---

## 4. Deterministic SPDX 2.3 SBOM Verification

[`scripts/generate_sbom.py`](../scripts/generate_sbom.py) parses vendored version macros directly from disk headers and generates [`SBOM/spdx-sbom.json`](../SBOM/spdx-sbom.json) using LF-canonicalized SHA-1/SHA-256 hashing so that Windows (CRLF) and Ubuntu Linux (LF) checkouts produce byte-for-byte identical digests.

| Metric / Field | Verified Value |
|---|---|
| Specification Version | `SPDX-2.3` (`dataLicense: CC0-1.0`) |
| Total SPDX Packages | **`4`** (`1` root application package + **`3` vendored third-party libraries**) |
| Total Inventoried Vendored Files | **`43`** (`5` CMSIS-Core + `4` STM32G4 CMSIS Device + `34` FreeRTOS-Kernel) |
| Total SPDX Relationships | **`47`** (`1` `DESCRIBES` + `3` root-to-package `CONTAINS` + `43` package-to-file `CONTAINS`) |
| Drift Check Command | `python3 scripts/generate_sbom.py --check` -> **PASS (exit code `0`)** |

| Package SPDXID | Name | Version | License | Upstream PURL | Files |
|---|---|---|---|---|---:|
| `SPDXRef-Package-Root` | `embedded-c-master-learning-hub` | `4.0.0` | `MIT` | — | — |
| `SPDXRef-Package-CMSIS-Core` | `ARM-CMSIS-Core` | `5.6.0` | `Apache-2.0` | `pkg:github/ARM-software/CMSIS_5@5.6.0` | 5 |
| `SPDXRef-Package-STM32G4-CMSIS-Device` | `STM32G4xx-CMSIS-Device` | `1.2.6` | `BSD-3-Clause` | `pkg:github/STMicroelectronics/cmsis_device_g4@v1.2.6` | 4 |
| `SPDXRef-Package-FreeRTOS-Kernel` | `FreeRTOS-Kernel` | `V11.1.0` | `MIT` | `pkg:github/FreeRTOS/FreeRTOS-Kernel@V11.1.0` | 34 |

---

## 5. YAML & Script Validation Results

| Artifact | Validation Command / Parser | Result |
|---|---|---|
| `.github/workflows/firmware-ci.yml` | `python -c "import yaml; yaml.safe_load(...)"` (`PyYAML 6.0.3`) | **PASS** (`4` jobs, valid schema, all Actions 40-char SHA pinned) |
| `.github/dependabot.yml` | `python -c "import yaml; yaml.safe_load(...)"` (`PyYAML 6.0.3`) | **PASS** (`version: 2`, `1` update rule for `github-actions`) |
| `scripts/generate_sbom.py` & `SBOM/spdx-sbom.json` | `python scripts/generate_sbom.py --check` | **PASS** (`SPDX-2.3`, `4` packages, `43` files, `47` relationships) |

---

## 6. Recommended GitHub Repository Security & Release Settings

The following repository-level controls reside in GitHub Settings (outside git-tracked files) and should be enabled prior to public release of `v4.0.0`:

1. **Branch Protection Ruleset on `main`:**
   - Require a Pull Request before merging (minimum `1` approving review; dismiss stale approvals on new commits).
   - Require all **4** CI status checks to pass before merging:
     - `Layer 1: Host Unit Tests, ASan/UBSan & gcov Coverage (>= 90%)`
     - `Layer 2: STM32G474RE Firmware Cross-Compile Matrix`
     - `Layer 3: Static Analysis, Doxygen, Formatting & SPDX 2.3 SBOM`
     - `Layer 4: Mermaid CLI Render (@11.4.2) & Repo Integrity Validator`
   - Require branches to be up to date before merging; block force pushes and branch deletion; enforce on administrators.
2. **Secret Scanning & Push Protection:**
   - Enable **GitHub Secret Scanning** and **Push Protection** under *Settings -> Code security and analysis* to block accidental pushes of tokens, keys, or credentials.
   - Enable **Private Vulnerability Reporting** to back [`SECURITY.md`](../SECURITY.md).
3. **CodeQL Code Scanning:**
   - Enable GitHub Default Setup or a dedicated CodeQL workflow covering `c-cpp`, `python`, and `actions` on pushes to `main`, Pull Requests, and a weekly cron schedule.
4. **Signed Release Tagging (`v4.0.0`):**
   - Sign the release tag using GPG or SSH commit/tag signing:
     ```bash
     git tag -s v4.0.0 -m "Release v4.0.0: Hardened CI, SPDX 2.3 SBOM, Security & Curriculum Release"
     git push origin v4.0.0
     ```
   - Attach `SBOM/spdx-sbom.json` and CI-generated `firmware-sha256.txt` checksums to the GitHub Release page so downstream learners and auditors can verify release provenance.

---

## 7. Public-Release Sanitization Inventory (Routed to Wave 2)

Automated repository-wide regex scanning (excluding `.git/`, `build/`, `CMSIS/`, and `FreeRTOS-Kernel/`) identified the exact pre-existing hygiene items to be redacted during **Wave 2 Sanitization**:

### 7.1 Physical STLINK-V3E 24-Hex Hardware Serial Identifier (`7` Hits across `6` Files)
Replace all occurrences with `<REDACTED-STLINK-SERIAL>`:

| # | Repository File | Line(s) | Hit Count | Action (Wave 2) |
|---:|---|---:|---:|---|
| 1 | `AUDIT/COMPLETE_CONCEPT_INVENTORY.md` | `349` | 1 | Replace with `<REDACTED-STLINK-SERIAL>` |
| 2 | `AUDIT/concepts_sot.json` | `7291` | 1 | Replace with `<REDACTED-STLINK-SERIAL>` |
| 3 | `AUDIT/DRIVER_LIBRARY_VERIFICATION.md` | `100` | 1 | Replace with `<REDACTED-STLINK-SERIAL>` |
| 4 | `AUDIT/FREERTOS_VERIFICATION_REPORT.md` | `99` | 1 | Replace with `<REDACTED-STLINK-SERIAL>` |
| 5 | `AUDIT/HARDWARE_VALIDATION_SUMMARY.md` | `6`, `206` | 2 | Replace with `<REDACTED-STLINK-SERIAL>` |
| 6 | `AUDIT/HARDWARE_VERIFICATION_MATRIX.md` | `5` | 1 | Replace with `<REDACTED-STLINK-SERIAL>` |
| | **Total ST-LINK Serial Hits** | | **7** | **Pending Wave 2** |

### 7.2 Windows Local Workspace Root Prefix (`28` Hits across `10` Files)
Replace all absolute local workspace drive prefixes (`<DRIVE>:\<WORKSPACE_DIR>\embedded-c-master-learning-hub`) with repository-relative POSIX paths:

| # | Repository File | Hit Count | Action (Wave 2) |
|---:|---|---:|---|
| 1 | `AUDIT/INDEPENDENT_RED_TEAM_REPORT.md` | 14 | Convert to repo-relative POSIX paths |
| 2 | `AUDIT/ROUND_2_VALIDATION_RESULTS.md` | 4 | Convert to repo-relative POSIX paths |
| 3 | `BACKUP/BACKUP_MANIFEST.md` | 3 | Convert to repo-relative POSIX paths |
| 4 | `AUDIT/EDUCATIONAL_QUALITY_REPORT.md` | 1 | Convert to repo-relative POSIX path |
| 5 | `AUDIT/FINAL_COMPLETENESS_REPORT.md` | 1 | Convert to repo-relative POSIX path |
| 6 | `AUDIT/FINAL_REMEDIATION_REPORT.md` | 1 | Convert to repo-relative POSIX path |
| 7 | `AUDIT/KNOWN_LIMITATIONS.md` | 1 | Convert to repo-relative POSIX path |
| 8 | `AUDIT/ROUND_2_REMEDIATION_REPORT.md` | 1 | Convert to repo-relative POSIX path |
| 9 | `AUDIT/VIDEO_COVERAGE_REPORT.md` | 1 | Convert to repo-relative POSIX path |
| 10 | `PROJECT_MANAGEMENT/REMEDIATION_TRACKER.md` | 1 | Convert to repo-relative POSIX path |
| | **Total Local Workspace Prefix Hits** | **28** | **Pending Wave 2** |

### 7.3 Windows User-Profile Absolute Paths (`2` Hits in `1` File)
Replace workstation user-profile paths (`<DRIVE>:\<USERS_DIR>\<USER>\...`) with neutral placeholders:

| # | Repository File | Line(s) | Hit Count | Action (Wave 2) |
|---:|---|---:|---:|---|
| 1 | `AUDIT/INDEPENDENT_RED_TEAM_REPORT.md` | `393`, `546` | 2 | Redact user-profile absolute paths |
| | **Total User-Profile Path Hits** | | **2** | **Pending Wave 2** |

### 7.4 Stream 3 Self-Check
All `7` files created or hardened by Stream 3 (`SECURITY.md`, `THIRD_PARTY_LICENSES.md`, `.github/dependabot.yml`, `.github/workflows/firmware-ci.yml`, `scripts/generate_sbom.py`, `SBOM/spdx-sbom.json`, and `AUDIT/CI_CD_AND_SECURITY_REPORT.md`) contain **`0`** raw secrets, **`0`** ST-LINK serial numbers, **`0`** local workspace drive paths, and **`0`** user-profile paths.
