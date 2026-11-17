# ARM Architecture & CMSIS Reference Library

### 1. Official ARM Architecture Standards
- **ARMv7-M Architecture Reference Manual:**
  - *Document ID:* ARM DDI 0403E.e
  - *Description:* Definitive hardware architecture reference specifying the ARMv7-M profile, 16/32-bit Thumb-2 instruction set encoding, processor operating states, exception models, memory barrier instructions (`DMB`, `DSB`, `ISB`), and debug architecture.
- **ARM Cortex-M4 Technical Reference Manual:**
  - *Document ID:* ARM DDI 0439F
  - *Description:* Microarchitecture specifications detailing the 3-stage pipeline, branch speculation, memory interfaces, Floating Point Unit (FPU), and DSP extension registers.
- **Procedure Call Standard for the ARM Architecture (AAPCS):**
  - *Description:* Defines how functions pass parameters (registers R0-R3, stack), return values (R0-R1), and which registers must be preserved across calls (callee-saved: R4-R11) versus volatile caller-saved registers (R0-R3, R12, LR, PC, PSR).

---

### 2. CMSIS Standards (Common Microcontroller Software Interface Standard)
- **CMSIS-Core (Cortex-M):**
  - Standardized hardware abstraction layer for Cortex-M processor core registers, NVIC functions (`NVIC_EnableIRQ`, `NVIC_SetPriority`), and intrinsic instructions (`__NOP`, `__WFI`, `__disable_irq`).
- **CMSIS-DSP:**
  - Optimized digital signal processing software library providing over 60 mathematical functions (FIR, IIR, FFT, Matrix operations) tuned specifically for Cortex-M4 SIMD instructions.
- **CMSIS-SVD (System View Description):**
  - XML schema representing microcontroller peripheral memory maps, register offsets, bitfields, and reset states, enabling register-level inspection in modern IDEs like VS Code Cortex-Debug.
