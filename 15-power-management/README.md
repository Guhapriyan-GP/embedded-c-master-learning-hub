# Stage 14: System Reliability, Watchdogs & Low-Power Modes

## 1. Prerequisites
Stage 06 (RCC & Clocks) & Stage 10 (Interrupts).

## 2. Core Technical Theory & Engineering Principles
Production embedded systems must operate reliably in hostile electrical environments and maximize battery life using low-power modes.

### 1. Hardware Watchdogs
- **Independent Watchdog (IWDG):** Clocked by an isolated internal 32 kHz RC oscillator (LSI). Operates independently of the main PLL clock. Kicked periodically via `IWDG->KR = 0xAAAA`. If software deadlocks, the IWDG resets the microcontroller.
- **Window Watchdog (WWDG):** Clocked from APB1. Enforces both a maximum timeout AND a minimum refresh window; refreshing the watchdog too early or too late causes an immediate reset, catching erratic runaway code.

### 2. Low-Power Operational Modes
- **Sleep Mode:** CPU clock halted; all peripherals continue operating. Wakeup via any interrupt in 4 clock cycles.
- **Stop 0 / Stop 1 Mode:** All high-speed clocks (PLL, HSI, HSE) halted; SRAM and register content preserved; current consumption drops to microamps. Wakeup via EXTI interrupt.
- **Standby Mode:** Deepest low-power state. SRAM and register content lost (except backup domain). Wakeup acts as a hardware reset.

### 3. Brown-Out Reset (BOR) & Power Voltage Detector (PVD)
Monitors VDD supply voltage and issues an interrupt or forces safe system reset before supply rail drops below the minimum operating threshold, preventing Flash corruption.

## 3. Practical Coding Exercises & Challenges
1. Configure the IWDG with a 500 ms timeout and verify reset recovery in software.
2. Measure power consumption across Run, Sleep, and Stop 0 modes on the NUCLEO board.
3. Configure the PVD to save critical data to backup registers upon power failure.

## 4. Expected Learning Outcomes & Mastery Criteria
Competency in architecting fail-safe systems with watchdog monitoring and microamp low-power profiles.
