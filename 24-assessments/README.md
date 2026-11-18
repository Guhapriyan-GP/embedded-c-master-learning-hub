# Curriculum Mastery Assessments & Grading Rubrics

This directory provides **five phase-aligned written and practical examinations** (100 points each) paired with **complete worked solutions and objective grading rubrics** in [`answer-keys-and-rubrics.md`](answer-keys-and-rubrics.md). A learner must achieve **$\ge 85\%$ (85/100 points)** on each examination before advancing past the corresponding phase.

**Concept ID:** `RES-ASSESS-001`

---

| Assessment File | Aligned Stages | Concept IDs Tested | Total Points |
|---|---|---|---:|
| [`stage-01-c-mastery-exam.md`](stage-01-c-mastery-exam.md) | Stages 00–01 | `DIG-ELEC-*`, `C-TYPE-*`, `C-BIT-*`, `C-PTR-*`, `C-STRUCT-*`, `C-QUAL-*`, `PROF-STD-002` (`INT30-C`) | 100 |
| [`stage-02-embedded-c-register-exam.md`](stage-02-embedded-c-register-exam.md) | Stage 02 | `EMB-MMIO-001`, `EMB-VOL-001`, `EMB-ATOM-001`, `EMB-CMSIS-001`, `ARM-SYNC-002` | 100 |
| [`stage-03-arm-cortex-m-architecture-exam.md`](stage-03-arm-cortex-m-architecture-exam.md) | Stages 03–05 | `ARM-CORE-*`, `ARM-ABI-*`, `ARM-EXC-*`, `ARM-FLT-001`, `ARM-MPU-001`, `TOOL-LD-001`, `STM32-ARCH-001`, `STM32-SRAM-001` | 100 |
| [`stage-04-stm32-peripherals-exam.md`](stage-04-stm32-peripherals-exam.md) | Stages 06–11 | `STM32-RCC-*`, `STM32-PWR-001`, `STM32-FLASH-001`, `PERIPH-GPIO/EXTI/TIM/UART/SPI/I2C/FDCAN/ADC/DAC/DMA-001` | 100 |
| [`stage-05-rtos-and-advanced-firmware-exam.md`](stage-05-rtos-and-advanced-firmware-exam.md) | Stages 12–15 | `RTOS-*`, `BOOT-*`, `SEC-*`, `REL-*`, `PROF-STD-*`, `TEST-UNIT-*` | 100 |
| [`answer-keys-and-rubrics.md`](answer-keys-and-rubrics.md) | All Stages | Complete worked solutions, exact register values, and point-by-point pass/fail rubrics | 500 |

---

### Grading Policy
- **Pass Threshold:** $\ge 85 / 100$ per exam AND zero safety-critical hardware violations (e.g., overclocking ADC $> 60\ \text{MHz}$, omitting Flash 4WS at 170 MHz, using `|=` on `rc_w1` flags, or enabling irreversible `RDP` Level 2 on a development board).
- **Code Verification Rule:** All practical C solutions must compile cleanly under `arm-none-eabi-gcc -std=c11 -Wall -Wextra -Werror -Wconversion` (or host `gcc -fsanitize=address,undefined` for Stage 01 & Stage 05 unit tests).
