# Revision Notes & Measurable Mastery Gates (Levels 1, 2 & 3)

**Rule #1 of Mastery:** Reading a chapter or watching a video is **never** mastery. You have passed a Mastery Gate **only** when you can satisfy every measurable criterion below: **Explain from first principles**, **Implement in strict C11 (`-Werror`)**, **Inspect live registers/memory in GDB**, and **Verify expected behavior**.

Use this document right before advancing to the next level in [`LEARNER_PROGRESS_DASHBOARD.md`](LEARNER_PROGRESS_DASHBOARD.md) and taking the corresponding exam in [`24-assessments/`](../24-assessments/README.md).

---

## Level 1 — Essential Foundations Revision & Mastery Gate
**Covers:** Setup, `Stage 01b` (Digital HW), `Stage 02` (ISO C11), `Stage 03` (Embedded C / MMIO), `Stage 04` (Cortex-M4F Core), `Stage 05` (Toolchain / Linker / GDB), `Stage 06` (STM32G474RE Platform), `Stage 07 / 06A` (`08-gpio/`, Labs 01–02)  
**Key Concept IDs:** `DIG-ELEC-001..004`, `HWSW-PCB-001..002`, `C-*`, `EMB-*`, `ARM-CORE-*`, `ARM-EXC-*`, `ARM-FLT-001`, `TOOL-GCC-001`, `TOOL-LD-001`, `STM32-ARCH-001`, `PERIPH-GPIO-001..004`

### High-Yield Level 1 Revision Sheet
1. **NUCLEO-G474RE Hardware Traps:** `LD2` = `PA5` (Active-HIGH), `B1` = `PC13` (**Active-HIGH** with external pull-down — do NOT enable pull-up!), `JP6` = `IDD` current jumper (NOT `JP5`), `PA9`/`PA10` have 5.1 kΩ UCPD dead-battery pull-downs until `PWR->CR3 |= PWR_CR3_UCPD_DBDIS;`, `PB8-BOOT0` has a 10 kΩ pull-down (`R36`), and `GPIOE`/`SPI4`/`FMC`/`AES` do not exist on LQFP64.
2. **ISO C11 Rules:** Always use `<stdint.h>` (`uint32_t`), `U` suffixes on bit shifts (`1U << 5U`), `f` suffixes on float literals (`3.3f`), and understand integer promotions, struct padding, `static` linkage, and strict aliasing / Undefined Behavior.
3. **Embedded C Register Rules:** `volatile` prevents compiler elision/reordering of MMIO accesses (but is **not** a CPU barrier or mutex). Use atomic `BSRR`/`BRR` for GPIO (AHB2 `0x48000000` is outside Cortex-M4 bit-banding!), direct `=` for `rc_w1` (`EXTI->PR1`, `ADC->ISR`) and `WO` (`TIM->EGR`), and direct `= ~mask` for `rc_w0` (`TIM->SR`).
4. **STM32G4 GPIO Reset State:** `GPIOx->MODER` resets to **`11b` Analog Mode** (`0xFFFFFFFF` on `GPIOC`), disabling digital input Schmitt triggers until cleared to `00b`. Always configure `AFR`/`OTYPER`/`OSPEEDR`/`PUPDR` **before** `MODER`.
5. **Cortex-M4F & Linker Essentials:** 118-entry vector table = **472 B**, **512 B aligned** at `0x08000000`; `FLASH` 512 KB, `RAM` 96 KB (`SRAM1` 80 KB + `SRAM2` 16 KB, `_estack = 0x20018000`), `CCMRAM` 32 KB (`0x10000000` I/D-Code, `0x20018000` DMA alias); 8-word basic exception stack frame (`R0,R1,R2,R3,R12,LR,PC,xPSR`).

### Level 1 Measurable Mastery Gate (All 5 Must Pass)
- [ ] **Explain:** State without notes why `GPIOA->ODR ^= (1U << 5U)` has an ISR race, why `GPIOC->IDR` reads `0` before clearing `MODER`, and why `.isr_vector` requires 512-byte alignment.
- [ ] **Implement & Compile (`-Werror`):** Write from scratch (no copy-paste) an HSI16 16 MHz bare-metal `PA5` blink + `PC13` Active-HIGH debounced toggle that compiles with **0 warnings** under `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wdouble-promotion`.
- [ ] **Host Tests:** Execute `make -C tests test` and confirm **35/35** `test_c_foundations` assertions pass.
- [ ] **GDB Inspection:** Halt at `main()` and inside a GPIO helper; verify `GPIOA->MODER` (`0x48000000`), `GPIOC->IDR` (`0x48000810`), and inspect `.isr_vector` with `arm-none-eabi-readelf -SW`.
- [ ] **Assessments:** Score $\ge 85\%$ on [Stage 01 Exam](../24-assessments/stage-01-c-mastery-exam.md), [Stage 02 Exam](../24-assessments/stage-02-embedded-c-register-exam.md), and [Stage 03 Exam](../24-assessments/stage-03-arm-cortex-m-architecture-exam.md).

---

## Level 2 — Intermediate Production Firmware Revision & Mastery Gate
**Covers:** `Stage 08 / 06B` (170 MHz RCC/PWR/Flash), `Stage 09 / 07` (SysTick/NVIC/EXTI), `Stage 10 / 08` (Timers/PWM), `Stage 11 / 09` (USART/SPI/I2C), `Stage 12 / 10` (ADC/DAC), `Stage 13 / 11` (DMA/DMAMUX), `Stage 14 / 12` (Flash/CRC), `Stage 15 / 13` (Watchdogs/Low-Power), `Stage 16 / 14` (FreeRTOS Core)  
**Key Concept IDs:** `STM32-RCC-*`, `STM32-PWR-001..002`, `STM32-FLASH-001..002`, `ARM-NVIC-001`, `ARM-SYST-001`, `PERIPH-EXTI-*`, `PERIPH-TIM-*`, `PERIPH-UART-*`, `PERIPH-SPI-*`, `PERIPH-I2C-*`, `PERIPH-ADC-*`, `PERIPH-DAC-*`, `PERIPH-DMA-*`, `RT-WDG-*`, `RTOS-*`

### High-Yield Level 2 Revision Sheet
1. **170 MHz & 160 MHz Boost Checklist (`RM0440 §6.1.5`):** `PWREN` + readback → `VOS=01b` + poll `PWR->SR2.VOSF == 0` (`0x14`) → `PWR->CR5.R1MODE = 0` (`0x80`) → `FLASH->ACR` `4WS` + ART → PLL (`f_IN=4 MHz`, `N=85` or `80`, `R=2`, `PLLREN=1`) → **`HPRE=/2` → `SW=PLL` → wait $\ge 1\ \mu\text{s}$ → `HPRE=/1`** → `SystemCoreClockUpdate()`.
2. **Interrupt & Timer Discipline:** STM32G474RE has 4 NVIC priority bits (`0..15`, lower number = higher priority). Always clear `EXTI->PR1 = mask` (`rc_w1`) or `TIMx->SR = ~flag` (`rc_w0`) + `__DSB()`. Force shadow reload with `TIMx->EGR = TIM_EGR_UG` and clear `UIF` before `UIE`.
3. **Serial Bus Gotchas:**
   - **USART2 (`PA2/PA3` AF7):** Separate `RDR`/`TDR`, `BRR = 1476` @ 170 MHz / 115200, clear `ORE` via `ICR`, use lock-free SPSC ring buffer (`Lab 08`).
   - **SPI1:** Set **`SPI_CR2.FRXTH = 1`** for 8-bit `RXNE`, access `DR` as `uint8_t*`, always clock a 2nd dummy byte on reads (`Lab 11`).
   - **I2C1:** `SADD[7:1] = addr7 << 1`, check `NACKF`, use `AUTOEND`/`RELOAD`, implement 9-clock bus recovery, watch `PB8-BOOT0` 10 kΩ pull-down (`Lab 12`).
4. **Analog & DMA Rules:** `ADC1` resets in `DEEPPWD=1` (`0x20000000`); use **`CKMODE=11b` (`42.5 MHz <= 60 MHz`)**; calibrate before `ADEN`; temp sensor needs $t_S \ge 5\ \mu\text{s}$ (`247.5` or `640.5` cycles). `DAC1–4` are on **AHB2** and require **`MCR.HFSEL = 10b` at 170 MHz**. Enable `DMAMUX1EN`, never clear `DMA_IFCR.CGIFx` (`ES0430 §2.3.1`), and use `0x20018000` alias if DMA targets CCM-SRAM.
5. **Reliability, Power & FreeRTOS:** STM32G4 has Sleep, LP-Run, LP-Sleep, **Stop 0, Stop 1**, Standby (`RRS`), Shutdown (*no Stop 2!*). Measure current on **`JP6`** (`IDD`). Freeze `IWDG`/`WWDG` in `DBGMCU->APB1FZR1` during debug. For FreeRTOS `ARM_CM4F`, call `NVIC_SetPriorityGrouping(0U)`, keep ISR priorities numerically $\ge$ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`, use `*FromISR()` APIs + `portYIELD_FROM_ISR`, and prefer static allocation (`xTaskCreateStatic`).

### Level 2 Measurable Mastery Gate (All 5 Must Pass)
- [ ] **Explain:** Derive by hand (1) 170 MHz and 160 MHz PLL parameters, (2) why `ADC CKMODE=01` fails at 170 MHz, (3) why `DMA_IFCR.CGIFx` violates `ES0430 §2.3.1`, and (4) why a Mutex supports Priority Inheritance while a Binary Semaphore does not.
- [ ] **Implement & Compile (`-Werror`):** Build cleanly at least **Labs 03, 04, 05, 06, 08, 09, 10, 13, 14** (`arm-none-eabi-gcc`, 0 warnings) and run `make -C tests test` (**78/78** ring-buffer unit assertions pass).
- [ ] **GDB Inspection:** Run `.gdbinit` `clock_regs` after 170 MHz switch (`SWS=11b`, `CR5=0`, `4WS`), dump an `EXTI13` 8-word stack frame with `x/8wx $sp`, and verify `TIM2->SR` + `DMA1_Channel1->CNDTR` live.
- [ ] **Functional Verification:** Verify onboard `USART2` `115200 8N1` VCP output (`Lab 08`), `IWDG` reset cause in `RCC->CSR.IWDGRSTF` (`Lab 13`), and Stop 0 wakeup via `PC13` (`Lab 14`).
- [ ] **Assessment:** Score $\ge 85\%$ on [Stage 04 Mastery Exam](../24-assessments/stage-04-stm32-peripherals-exam.md).

---

## Level 3 — Advanced STM32G474RE Specialist & Production Gate
**Covers:** `OPAMP`/`COMP` → `TIM1_BKIN` (`Lab 16`), `CORDIC`/`FMAC` (`Lab 17`), `FDCAN1`/`CRC-32` (`Lab 18`), `HRTIM1`, Dual-Bank Flash / Bootloaders / Security, `MPU` + HardFault `.noinit` Forensics (`Lab 19`), Host TDD / MISRA C / Static Analysis / CI (`Lab 20`), and Capstone Blueprint (`19-projects/`)  
**Key Concept IDs:** `G4-OPAMP-001`, `G4-COMP-001`, `G4-CORDIC-001`, `G4-FMAC-001`, `G4-FDCAN-001`, `G4-HRTIM-001`, `ARM-MPU-001`, `REL-CRASH-001`, `BOOT-*`, `SEC-*`, `TEST-*`, `PROF-STD-*`, `PROF-CI-001`

### High-Yield Level 3 Revision Sheet
1. **Hardware Safety Loops:** Overcurrent protection must **never** rely on software polling or RTOS tasks — route `COMP1` directly into `TIM1_BKIN` / `HRTIM1` fault (`< 50 ns` hardware `MOE` clear, `AOE=0` latched fault).
2. **STM32G4 Accelerators & Buses:** `CORDIC` (`q1.31` trig) and `FMAC` (`q1.15` FIR/IIR) sit on AHB1; benchmark exact cycles via `DWT->CYCCNT`. `FDCAN1–3` share Bosch M_CAN Message RAM at `0x4000A400`.
3. **Flash, Bootloaders & Security:** `DBANK=1` gives 2 KB pages with Read-While-Write; all Flash writes are 64-bit aligned with 8-bit ECC (`FLASH_ECCR`) and require `DCRST` cache flush (`ES0430 §2.2.2`). Bootloader handoff disables IRQs, resets SysTick/clocks, aligns `SCB->VTOR` (512 B), loads app `MSP`, and jumps to app `Reset_Handler`. **NEVER program `RDP` Level 2 (`0xCC`) on a development board (permanent JTAG/SWD brick!).**
4. **Defensive Engineering:** Configure an 8-region MPU 32-byte No-Access stack guard (`Lab 19`), persist HardFault diagnostics in `.noinit` (`g_crash_log`), audit worst-case stack via `-fstack-usage` (`.su`), and unit-test drivers on the host with fake CMSIS register structs (`Lab 20`).

### Level 3 Measurable Mastery Gate (All 5 Must Pass)
- [ ] **Explain:** Walk through (1) `COMP1 -> TIM1_BKIN` asynchronous trip path, (2) 9-step safe bootloader handoff, (3) `DBANK`/`OBL_LAUNCH`/`RDP` anti-bricking rules, and (4) Liu & Layland RMS utilization + Response Time Analysis.
- [ ] **Implement & Compile (`-Werror`):** Build cleanly **Labs 16, 17, 18, 19, 20** with 0 warnings and generate `.su` stack-usage reports (`-fstack-usage`).
- [ ] **GDB Forensics:** Deliberately trigger the MPU stack guard or `DIVBYZERO` (`Lab 19`), decode `CFSR`/`HFSR`/`MMFAR`/`BFAR` via `fault_regs`, recover `g_crash_log` across reset via `crash_log`, and benchmark `CORDIC` vs FPU cycles via `DWT->CYCCNT` (`Lab 17`).
- [ ] **Host TDD & CI Hygiene:** Add at least 5 new edge-case assertions to a host unit test under `-Wall -Wextra -Werror` and confirm `python scripts/validate_repo.py` passes with `0 FAIL`.
- [ ] **Assessment & Capstone:** Score $\ge 85\%$ on [Stage 05 Mastery Exam](../24-assessments/stage-05-rtos-and-advanced-firmware-exam.md) and implement one complete Capstone from [`19-projects/`](../19-projects/README.md).
