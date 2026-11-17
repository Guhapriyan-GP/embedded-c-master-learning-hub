# Contributing to Embedded C Master Learning Hub

We welcome contributions, bug reports, and enhancements from the embedded systems engineering community.

---

### 1. Code Standards & Style Guidelines
All code contributions must adhere to the following standards:
- **Language Standard:** C99 or C11 using `<stdint.h>` and `<stdbool.h>` types exclusively.
- **Hardware Naming:** Use official CMSIS register definitions from `stm32g474xx.h` (e.g. `GPIOA->BSRR`, `RCC->AHB2ENR`).
- **Literal Constants:** Always append unsigned literals with `U` (e.g., `(1U << 5)`).
- **Control Structures:** Always enclose loop and conditional blocks within braces `{}`.
- **Compiler Warnings:** Code must compile cleanly with `arm-none-eabi-gcc` under `-Wall -Wextra -Werror` with zero warnings.
- **Comments & Documentation:** Document non-obvious bitfield manipulations with exact section references to ST Reference Manual RM0440.

---

### 2. Pull Request Workflow
1. Fork the repository and create a feature branch (`git checkout -b feature/new-peripheral-lab`).
2. Verify all internal markdown links and run `python scripts/validate_repo.py`.
3. If submitting code, verify compilation using `make` in `23-code-library/templates`.
4. Commit changes with clear, structured messages (`feat: add CAN FD register map and lab`).
5. Open a Pull Request detailing the technical verification steps performed on hardware.
