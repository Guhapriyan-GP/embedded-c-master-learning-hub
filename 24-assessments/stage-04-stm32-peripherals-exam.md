# Stage 04 Mastery Exam: STM32 Peripherals & Clock Systems

### Part A: Mathematical Calculations & Register Derivations (60 Points)
1. **PLL Multiplier Calculation:**
   Given an input clock $f_{\text{IN}} = 16\ \text{MHz}$ (HSI16), design the PLL parameters ($M, N, R$) to produce a target system clock $f_{\text{SYSCLK}} = 170\ \text{MHz}$ such that the internal VCO frequency lies within the valid specification window ($64\ \text{MHz} \le f_{\text{VCO}} \le 344\ \text{MHz}$). Specify the exact bit values to write to `RCC_PLLCFGR`.
2. **Flash Latency Verification:**
   Explain why switching the STM32G4 core to 170 MHz in Range 1 Boost Mode requires 4 wait states (`FLASH_ACR_LATENCY_4WS`). What physical failure occurs if this setting is omitted?
3. **Timer PWM Frequency & Resolution:**
   With a timer clock $f_{\text{TIM\_CLK}} = 170\ \text{MHz}$, calculate the exact `PSC` and `ARR` register values to produce a $32.0\ \text{kHz}$ PWM signal. What is the duty cycle resolution in steps?
4. **USART Baud Rate Divisor:**
   Calculate the integer value to write to `USART2->BRR` for 921,600 baud when clocked from a 170 MHz bus clock under 16x oversampling. Calculate the percentage error.
5. **ADC Sampling Time:**
   An analog source has an output impedance of $10\ \text{k}\Omega$. Calculate the minimum sampling time required for 12-bit accuracy on the STM32G4 SAR ADC and choose the appropriate `SMPRx` bitfield.

---

### Part B: Register-Level Driver Implementation (40 Points)
Write a complete bare-metal C function `void Configure_GPIO_TIM1_PWM(void)` that initializes PA8 as TIM1 Channel 1 PWM output with complementary dead-time insertion. Explain every register modification.
