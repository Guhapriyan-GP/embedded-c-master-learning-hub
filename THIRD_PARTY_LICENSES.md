# Third-Party Licenses & Vendored Software Inventory

**Repository:** `embedded-c-master-learning-hub` (Original repository code & curriculum licensed under `MIT` — see [`LICENSE`](LICENSE))  
**Machine-Readable SBOM:** [`SBOM/spdx-sbom.json`](SBOM/spdx-sbom.json) (SPDX 2.3 JSON, deterministically generated and CI-verified by [`scripts/generate_sbom.py`](scripts/generate_sbom.py))  
**Concept IDs:** `PROF-SC-001`, `PROF-CI-001`, `STM32-ARCH-001`, `RTOS-KERN-001`

---

## 1. Summary of Vendored Third-Party Components

This repository vendors exactly **3** upstream third-party C firmware components (minimal Cortex-M4F / STM32G474xx subsets only, with zero embedded `.git` metadata):

| # | Component | Exact Vendored Version | Upstream Provenance | SPDX License | Copyright Notice | Vendored Path(s) | File Count |
|---:|---|---|---|---|---|---|---:|
| 1 | **ARM CMSIS-Core (Cortex-M)** | `5.6.0` (`__CM_CMSIS_VERSION = 0x00050006`; `cmsis_version.h` `V5.0.5`, `core_cm4.h` `V5.1.2`, `cmsis_compiler.h` `V5.1.0`, `cmsis_gcc.h` `V5.4.1`, `mpu_armv7.h` `V5.1.2`) | `https://github.com/ARM-software/CMSIS_5` | `Apache-2.0` | `Copyright (c) 2009-2022 ARM Limited. All rights reserved.` | `23-code-library/Drivers/CMSIS/Core/Include/` | 5 |
| 2 | **STMicroelectronics CMSIS Device STM32G4xx** | `V1.2.6` (`__STM32G4_CMSIS_VERSION = 0x01020600` in `stm32g4xx.h`) | `https://github.com/STMicroelectronics/cmsis_device_g4` (tag `v1.2.6`) | `BSD-3-Clause` | `Copyright (c) 2019 STMicroelectronics. All rights reserved.` | `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/` + `23-code-library/templates/Src/system_stm32g4xx.c` | 4 |
| 3 | **FreeRTOS-Kernel** | `V11.1.0` (commit `dbf70559b27d39c1fdb68dfb9a32140b6a6777a0`, tag object `f388a5c8078e152913e4eb3c5d75bf89561392df`) | `https://github.com/FreeRTOS/FreeRTOS-Kernel` (release `V11.1.0`) | `MIT` | `Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.` | `23-code-library/freertos/FreeRTOS-Kernel/` | 34 |

---

## 2. Detailed Component Provenance & Compliance Notes

### 2.1 ARM CMSIS-Core (Cortex-M) — `Apache-2.0`
- **Source Directory:** `23-code-library/Drivers/CMSIS/Core/Include/`
- **Vendored Files (5):**
  - `cmsis_compiler.h` (`V5.1.0`, `21. March 2018`)
  - `cmsis_gcc.h` (`V5.4.1`, `27. May 2021`)
  - `cmsis_version.h` (`V5.0.5`, `02. February 2022`; defines `__CM_CMSIS_VERSION_MAIN = 5U`, `__CM_CMSIS_VERSION_SUB = 6U`)
  - `core_cm4.h` (`V5.1.2`, `04. June 2021`)
  - `mpu_armv7.h` (`V5.1.2`, `10. February 2020`)
- **License Terms:** Every header explicitly declares `SPDX-License-Identifier: Apache-2.0` with full Apache License 2.0 reference (`https://www.apache.org/licenses/LICENSE-2.0`). All files are unmodified upstream CMSIS-Core headers.

### 2.2 STMicroelectronics CMSIS Device STM32G4xx — `BSD-3-Clause`
- **Source Paths (4 files):**
  - `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g4xx.h` (declares `CMSIS Device version number V1.2.6`, `__STM32G4_CMSIS_VERSION = 0x01020600`)
  - `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h` (canonical STM32G474xx peripheral register definitions & `IRQn_Type`)
  - `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/system_stm32g4xx.h`
  - `23-code-library/templates/Src/system_stm32g4xx.c` (ST MCD Application Team CMSIS system initialization source)
- **License Terms:** Upstream `STMicroelectronics/cmsis_device_g4` (`v1.2.6`) is distributed under the **BSD 3-Clause License** (`BSD-3-Clause`, `https://github.com/STMicroelectronics/cmsis_device_g4/blob/v1.2.6/LICENSE.txt`). ST copyright notices are preserved verbatim in every vendored header and source file.

### 2.3 FreeRTOS-Kernel `V11.1.0` — `MIT`
- **Source Directory:** `23-code-library/freertos/FreeRTOS-Kernel/`
- **Pinned Revision:**
  - Upstream Repository: `https://github.com/FreeRTOS/FreeRTOS-Kernel`
  - Release Tag: `V11.1.0`
  - Annotated Tag Object: `f388a5c8078e152913e4eb3c5d75bf89561392df`
  - Commit SHA: `dbf70559b27d39c1fdb68dfb9a32140b6a6777a0`
  - Subset Included: Core kernel (`tasks.c`, `queue.c`, `list.c`, `timers.c`, `event_groups.c`, `stream_buffer.c`), `include/*`, ARM Cortex-M4F GCC port (`portable/GCC/ARM_CM4F/port.c`, `portmacro.h`), coalescing heap allocator (`portable/MemMang/heap_4.c`), `VERSION.txt`, and upstream [`LICENSE.md`](23-code-library/freertos/FreeRTOS-Kernel/LICENSE.md).
- **License Terms:** `MIT` License (full upstream license text preserved in `23-code-library/freertos/FreeRTOS-Kernel/LICENSE.md`).

---

## 3. CI / Build-Time Tooling (Not Vendored in Repository)

The following external tools are fetched only inside ephemeral GitHub Actions CI runners (never vendored into firmware binaries):
- **GitHub Actions** (all SHA-pinned in `.github/workflows/firmware-ci.yml`): `actions/checkout` (`MIT`), `actions/setup-python` (`MIT`), `actions/setup-node` (`MIT`), `actions/upload-artifact` (`MIT`).
- **Mermaid CLI:** `@mermaid-js/mermaid-cli@11.4.2` (`MIT`, installed in CI only for SVG render verification).
- **Ubuntu 24.04 Packages:** `gcc`, `gcov`, `lcov`, `make`, `gcc-arm-none-eabi`, `binutils-arm-none-eabi`, `cppcheck`, `clang-tidy`, `clang-format`, `doxygen`.

---

## 4. Reproducing & Verifying the SBOM

Regenerate or verify the deterministic SPDX 2.3 JSON SBOM at any time:

```bash
# Verify SBOM/spdx-sbom.json matches current vendored files with zero drift:
python3 scripts/generate_sbom.py --check

# Regenerate SBOM/spdx-sbom.json after an intentional vendored component upgrade:
python3 scripts/generate_sbom.py
```
