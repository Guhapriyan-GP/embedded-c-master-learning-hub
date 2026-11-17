# Lab 09: ADC1 Single-Channel Acquisition with Hardware Calibration

## 1. Learning Objectives
Execute the internal hardware self-calibration sequence on ADC1, configure PA0 as an analog input, and sample a potentiometer voltage in 12-bit resolution.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE, 10k potentiometer.
- **Wiring:** Potentiometer ends to 3.3V and GND; wiper connected to PA0 (ADC1_IN1).

## 3. Peripheral Registers & Official Documentation
- ADC1->CR (ADVREGEN, ADCAL, ADEN, ADSTART)
- ADC1->CFGR (RES)
- ADC1->SMPR1 (SMP1)
- ADC1->SQR1 (SQ1, L)
- ADC1->DR
- ADC12_COMMON->CCR (CKMODE)

**Reference Documentation:** RM0440 Chapter 21 (ADC).

## 4. Mathematical Calculations & Bitfield Settings
12-bit ADC result: V_in = (ADC_DR * 3.3V) / 4095. Sampling time = 47.5 cycles.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"
#include <stdio.h>

void ADC1_Init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;

    /* PA0 Analog Mode */
    GPIOA->MODER |= (3U << 0);

    /* Synchronous clock HCLK/1 */
    ADC12_COMMON->CCR |= (1U << ADC_CCR_CKMODE_Pos);

    /* Enable voltage regulator */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    for (volatile int i = 0; i < 5000; i++);

    /* Calibrate */
    ADC1->CR &= ~ADC_CR_ADEN;
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);

    /* Enable ADC */
    ADC1->ISR |= ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));

    /* Channel 1, 1 conversion, 47.5 cycles */
    ADC1->SQR1 = (1U << ADC_SQR1_SQ1_Pos);
    ADC1->SMPR1 |= (4U << ADC_SMPR1_SMP1_Pos);
}

uint16_t ADC1_Sample(void) {
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC));
    return (uint16_t)ADC1->DR;
}

int main(void) {
    ADC1_Init();
    while (1) {
        uint16_t raw = ADC1_Sample();
        uint32_t mv = (raw * 3300UL) / 4095UL;
        (void)mv;
        for (volatile int i = 0; i < 1000000; i++);
    }
}
```

## 6. Debugging & Register Verification Procedure
Place a breakpoint after `ADC1_Sample()`. Rotate the potentiometer wiper from 0V to 3.3V and verify `raw` varies smoothly from 0 to 4095.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Skipping the internal voltage regulator startup delay ($20\ \mu	ext{s}$).
2. Omitting calibration, leading to non-linear offset errors.

## 8. Expected Output & Verification
The ADC tracks input voltage with high precision across the entire 0-3.3V range.
