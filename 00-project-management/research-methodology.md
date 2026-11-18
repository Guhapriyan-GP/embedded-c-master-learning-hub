# Research Methodology & Verification Protocols

## 1. Documentation Verification Standard
All register descriptions, peripheral behaviors, bit positions, and memory addresses are grounded in official technical literature:
1. **Primary Manufacturer Reference:** STMicroelectronics Reference Manual RM0440 (*STM32G4 Series advanced Arm-based 32-bit MCUs*).
2. **Device Datasheet:** STMicroelectronics Datasheet DS12288 (*STM32G474xB/xC/xE*).
3. **Core Architecture Manual:** ARMv7-M Architecture Reference Manual (DDI 0403E.e) and ST Programming Manual PM0214 (*STM32 Cortex-M4 MCUs and MPUs programming manual*).
4. **Board Hardware Reference:** ST User Manual UM2505 (*STM32G4 Nucleo-64 boards*).

No claims regarding peripheral configuration are made based on generic internet blog posts or third-party interpretations without explicit cross-verification against RM0440.

## 2. Mathematical Validation Protocols
All operational formulas included in the curriculum must be stated with full unit dimensional analysis and worked examples:
- **Timer Frequency Equation:**

  $$
  \text{Update Frequency } (f_{\text{UEV}}) = \frac{f_{\text{TIM-CLK}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}
  $$

- **UART Baud Rate Divisor Equation:**

  $$
  \text{USARTDIV} = \frac{f_{\text{CK}}}{\text{Baud Rate}}
  $$

  (with oversampling by 16 or 8 properly accounted for in the BRR register formatting).

- **ADC Sampling Time Equation:**

  $$
  t_{\text{CONV}} = t_{\text{SMP}} + t_{\text{SAR}} = t_{\text{SMP}} + 12.5 \text{ ADC clock cycles (for 12-bit resolution)}
  $$

## 3. Video Lecture Verification Protocol
To satisfy the mandatory video research standard:
1. Every video link must be an authentic, publicly accessible YouTube URL (`https://www.youtube.com/watch?v=...`).
2. Video metadata (exact duration, creator, publication date) must be programmatically verified against canonical source endpoints.
3. Every entry must detail:
   - Primary curriculum alignment
   - Technical prerequisites
   - Specific instructional value
   - Key concepts intentionally omitted or requiring supplemental reading.
