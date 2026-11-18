# Security Policy & Hardware Safety Boundaries

**Repository:** `embedded-c-master-learning-hub`  
**Target MCU / Board:** STMicroelectronics **STM32G474RET6** on **NUCLEO-G474RE** (`MB1367 Rev C`, ARM Cortex-M4F)  
**Concept IDs:** `SEC-BOOT-001`, `SEC-BOOT-002`, `MEM-OB-001`, `TOOL-CI-001`, `PROF-CI-001`, `REL-FAULT-001`

---

## 1. Supported Release Scope

Security updates, C undefined-behavior/memory fixes, and supply-chain advisories covering vendored firmware dependencies (`ARM CMSIS-Core`, `STMicroelectronics CMSIS Device STM32G4xx`, `FreeRTOS-Kernel V11.1.0`) and CI automation are maintained for the active release branch:

| Release Line | Maintenance State | Scope |
|---|---|---|
| `v4.x` / `main` | Active | Firmware safety fixes, memory/UB remediation, SBOM & CVE updates, CI hardening |
| `v3.x` | Critical Only | High-severity safety or vendored CVE patches on a best-effort basis |
| `< v3.0.0` | End of Life | Upgrade to `v4.x` |

---

## 2. Private Vulnerability Reporting

Do **not** open public GitHub Issues or Pull Requests for exploitable memory corruption, safety bypasses, CI supply-chain weaknesses, or exposed credentials.

### Reporting Procedure (GitHub Security Advisories)
1. Open the repository **Security** tab on GitHub and select **Report a vulnerability** (GitHub Private Vulnerability Reporting).
2. Include:
   - Exact file path(s), symbol name(s), or vendored dependency affected.
   - Minimal reproduction instructions (host `gcc` test command, `arm-none-eabi-gcc` target, or register/CI configuration).
   - Impact description (e.g., out-of-bounds DMA/ring-buffer access, power-stage fail-safe bypass, or CI workflow injection).
   - Proposed fix or mitigation, if known.

### Target Response Schedule
- **Initial Acknowledgement:** Within `72 hours`.
- **Technical Triage:** Within `7 calendar days`.
- **Patch & Coordinated Advisory:** Targeted within `30 calendar days` for High/Critical issues, with remediation notes recorded in `CHANGELOG.md`.

---

## 3. Embedded Hardware Safety Boundaries (STM32G474RE / NUCLEO-G474RE)

Bare-metal firmware directly drives electrical pins, Flash option bytes, clock rails, and power-stage peripherals. All learners and contributors must follow these hardware safety boundaries:

### 3.1 Flash Option Bytes, RDP & Write Protection (RM0440)
- **Never program `RDP Level 2` (`0xCC`):** On STM32G474RE, setting `FLASH_OPTR.RDP = 0xCC` permanently and irreversibly disables SWD/JTAG debug ports and the ROM bootloader, permanently sealing the MCU.
- **Maintain `RDP Level 0` (`0xAA`) for all lab work:** Option-byte registers (`FLASH_OPTKEYR`, `FLASH_OPTR`, `OBL_LAUNCH`) are documented for study in `14-memory-and-storage/` and must never be invoked automatically at boot in default firmware targets.
- **Preserve `DBANK` and Boot Option Bits:** Do not alter dual-bank Flash configuration (`DBANK`) or `nSWBOOT0`/`nBOOT0` unless executing a deliberate, documented recovery procedure.
- **No Unattended Flash Sector Locking:** Do not commit default-enabled code that locks `WRP1A`/`WRP1B`, `PCROP`, or `SEC_SIZE1`.

### 3.2 Electrical Limits, Jumpers & External Power Stages (DS12288 / UM2505)
- **Supply & Pin Voltage Limits:** Operate at nominal `VDD = 3.3 V` (`3.6 V` absolute maximum per DS12288). Keep analog/`TTa` inputs strictly inside `0 V .. VDDA` (`~3.3 V`), and never exceed `5.5 V` on `FT` pins.
- **Current Measurement Jumper (`JP6` vs `JP5`):** Measure MCU current strictly across **`JP6` (`IDD`)** per UM2505 — never miswire **`JP5` (`PWR`)**.
- **`PB8-BOOT0` Strapping Caution:** `PB8` shares the `BOOT0` pin on LQFP64; avoid driving `PB8` high across reset if main Flash boot is required.
- **Capstone 03 Power-Stage Guard (`19-projects/03-advanced-freertos-digital-power-controller/`):**
  - Default firmware compiles with **`POWER_SIM_MODE=1`** (internal software plant; zero external power wiring).
  - External hardware mode (**`POWER_SIM_MODE=0`**) drives complementary `TIM1` PWM (`CH1`/`CH1N`) with dead-time and `COMP1 -> TIM1_BKIN` hardware break. Verify dead-time and `BKIN` trip behavior on an oscilloscope with a current-limited bench supply before connecting any live power converter stage.
- **Educational Use Only:** Firmware in this repository is provided for education and prototyping and is **not** certified to IEC 61508, ISO 26262, DO-178C, or IEC 62304.

---

## 4. Public Repository Hygiene (Secrets, PII & Hardware Serials)

All commits, logs, and audit documents must obey strict public-repository hygiene:

1. **No Secrets or Private Keys:** Never commit API keys, tokens, passwords, `.env` files, or cryptographic private signing keys.
2. **No Local Absolute Paths or Usernames:** Replace workstation-specific absolute paths with repository-relative POSIX paths (e.g., `23-code-library/templates/Makefile`).
3. **Redact Hardware Identifiers:** Replace physical 24-hex STLINK-V3E serial numbers and STM32 96-bit Unique Device IDs (`0x1FFF7590`) with `<REDACTED-STLINK-SERIAL>` or `<REDACTED-STM32-UID>` before committing logs or reports.
4. **Automated Supply-Chain Controls:** CI enforces SHA-pinned GitHub Actions, least-privilege workflow permissions (`contents: read`), deterministic SPDX 2.3 SBOM verification (`scripts/generate_sbom.py --check`), and static analysis. See [`AUDIT/CI_CD_AND_SECURITY_REPORT.md`](AUDIT/CI_CD_AND_SECURITY_REPORT.md) and [`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md).
