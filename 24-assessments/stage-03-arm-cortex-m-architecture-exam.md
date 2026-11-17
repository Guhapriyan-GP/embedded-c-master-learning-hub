# Stage 03 Mastery Exam: ARM Cortex-M4 Architecture & Assembly

### Questions (100 Points Total)
1. **Core Registers & Stack Pointers (20 Points):** Contrast the Main Stack Pointer (MSP) and Process Stack Pointer (PSP). Which stack pointer is active on hardware reset, and which is typically used by an RTOS task?
2. **Operating Modes & Privilege (20 Points):** Detail the transition between Thread Mode and Handler Mode. Under what condition can software switch from unprivileged back to privileged execution?
3. **Exception Stacking (25 Points):** List all 8 registers automatically pushed to the stack upon entering an interrupt service routine on the Cortex-M4 (when FPU is inactive). What special value is placed in the Link Register (LR), and what does it signify?
4. **Linker Script Sections (20 Points):** Explain the difference between Virtual Memory Address (VMA) and Load Memory Address (LMA). Why does the `.data` section have different VMA and LMA addresses?
5. **HardFault Diagnosis (15 Points):** If the processor enters `HardFault_Handler`, which SCB registers (`CFSR`, `HFSR`, `MMFAR`, `BFAR`) must be inspected to determine whether the fault was caused by an invalid memory access or a divide-by-zero error?
