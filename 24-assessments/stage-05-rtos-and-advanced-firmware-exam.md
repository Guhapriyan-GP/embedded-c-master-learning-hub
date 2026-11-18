# Stage 05 Mastery Exam: FreeRTOS, Bootloaders, Security, Reliability & Verification

**Aligned Stages:** Stages 12–15 (`14-memory-and-storage/`, `15-power-management/`, `16-rtos/`, `17-advanced-firmware-engineering/`)  
**Concept IDs:** `RTOS-*`, `BOOT-*`, `SEC-*`, `REL-*`, `PROF-STD-*`, `TEST-UNIT-*`, `TEST-MOCK-001`  
**Total Points:** 100 | **Pass Threshold:** $\ge 85$ Points | **Answer Key:** [`answer-keys-and-rubrics.md`](answer-keys-and-rubrics.md#stage-05-answer-key--rubric)

---

### Questions (100 Points Total)

1. **FreeRTOS `ARM_CM4F` Context Switch & `BASEPRI` Partitioning (20 Points):**
   (a) Detail the roles of `SVC`, `SysTick`, and `PendSV` in a FreeRTOS Cortex-M4F context switch. Why is context switching deferred to lowest-priority `PendSV`? How does `PendSV_Handler` conditionally save FPU registers `S16–S31`?
   (b) On STM32G474RE (`__NVIC_PRIO_BITS = 4`), explain `NVIC_SetPriorityGrouping(0)` and `configMAX_SYSCALL_INTERRUPT_PRIORITY = (5 << 4)`. Which numeric IRQ priorities (`0..15`) may call `FromISR` APIs, which are unmasked high-priority ISRs (unaffected by `BASEPRI` masking), and why must `configMAX_SYSCALL_INTERRUPT_PRIORITY` NEVER be `0`?

2. **Priority Inversion vs. Deadlock & Interrupt-Safe Communication (15 Points):**
   (a) Contrast **unbounded priority inversion** (3-task L/M/H scenario) with **circular-wait deadlock**. Why does Priority Inheritance solve priority inversion yet **still deadlock** if two tasks acquire Mutex A and Mutex B in opposite orders?
   (b) Why are blocking APIs (`xQueueSend`, `xSemaphoreTake`) forbidden in ISRs? Explain the role of `xHigherPriorityTaskWoken` + `portYIELD_FROM_ISR()`, and why Direct-to-Task Notifications (`vTaskNotifyGiveFromISR`) outperform binary semaphores.

3. **Watchdogs (`IWDG`/`WWDG`) & STM32G4 Low-Power Modes (15 Points):**
   (a) Calculate `IWDG->PR` (exact binary & decimal) and `IWDG->RLR` for a 1.0 s timeout at $f_{\text{LSI}} = 32\ \text{kHz}$ using prescaler `/64`. Which `DBGMCU` register bit freezes `IWDG` during GDB halts?
   (b) How does `WWDG` trigger a reset on both too-early and too-late refreshes, and what is `WWDG_IRQn` (`EWI`) used for?
   (c) List the **actual** STM32G474RE low-power modes (explicitly noting whether Stop 2 exists!), state which NUCLEO-G474RE jumper measures $I_{DD}$ (`JP5` vs `JP6`), and list 3 mandatory actions upon waking from Stop 0/1.

4. **Dual-Bank Flash Bootloader Handoff & STM32G474RE Hardware Security (20 Points):**
   (a) List the 6+ mandatory steps for a safe Cortex-M4 bootloader jump from `0x08000000` to an application at `0x08008000` (including MSP validation, SysTick/NVIC teardown, `SCB->VTOR` alignment, and barriers).
   (b) Contrast STM32G474RE `RDP` Levels 0 (`0xAA`), 1, and 2 (`0xCC`), `WRP`, `PCROP`, `SEC1R`, and `TAMP` tamper zeroization. Does STM32G474RE contain a hardware AES/CRYP accelerator?

5. **MISRA C:2023, SEI CERT C & Formal Deviation Handling (15 Points):**
   Identify **four** distinct MISRA C / SEI CERT C / Barr Group violations in the snippet below, rewrite it safely for STM32G474RE `GPIOA->BSRR`, and explain what a **Deviation Permit** must document for the remaining integer-to-pointer cast:
   ```c
   int *p = 0x48000014;
   *p = *p | (1 << 5);
   ```

6. **Host-Based Unit Testing & Test Seams for Bare-Metal Drivers (15 Points):**
   Why can a driver that directly hardcodes `I2C1->CR2` not be unit-tested natively on an x86/x64 CI runner? Show two architectural patterns (pointer-injected fake `I2C_TypeDef` struct OR function-pointer vtable `const i2c_ops_t *`) that enable **Unity/CMock** testing with `-fsanitize=address,undefined` (ASan/UBSan) in GitHub Actions without physical hardware.
