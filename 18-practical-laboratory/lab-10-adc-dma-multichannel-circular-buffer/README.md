# Lab 10: Multichannel ADC with DMA Circular Buffer

## 1. Learning Objectives
Sample two analog channels (PA0 potentiometer and internal temperature sensor) continuously into an SRAM array using DMA1 Channel 1 and DMAMUX.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE, potentiometer on PA0.
- **Wiring:** Potentiometer wiper to PA0 (ADC1_IN1). Internal temperature sensor connected internally to ADC1_IN16.

## 3. Peripheral Registers & Official Documentation
- DMA1_Channel1->CCR, CNDTR, CPAR, CMAR
- DMAMUX1_Channel0->CCR
- ADC1->CFGR (DMAEN, DMACFG, CONT)
- ADC1->SQR1 (L=1 for 2 conversions, SQ1=1, SQ2=16)

**Reference Documentation:** RM0440 Chapter 11 (DMA), Chapter 12 (DMAMUX), and Chapter 21 (ADC).

## 4. Mathematical Calculations & Bitfield Settings
Buffer length = 2 channels * 32 samples = 64 halfwords. Circular DMA automatically loops back to index 0.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

#define NUM_CHANNELS 2
#define SAMPLES 32
uint16_t adc_dma_buffer[NUM_CHANNELS * SAMPLES];

void DMA_ADC_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;

    GPIOA->MODER |= (3U << 0); /* PA0 Analog */

    /* Route ADC1 (ID=5) to DMA1 Channel 1 */
    DMAMUX1_Channel0->CCR = (5U << DMAMUX_CxCR_DMAREQ_ID_Pos);

    /* DMA1 Channel 1 setup */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    DMA1_Channel1->CPAR = (uint32_t)&(ADC1->DR);
    DMA1_Channel1->CMAR = (uint32_t)adc_dma_buffer;
    DMA1_Channel1->CNDTR = NUM_CHANNELS * SAMPLES;
    DMA1_Channel1->CCR = DMA_CCR_CIRC | DMA_CCR_MINC |
                         (1U << DMA_CCR_PSIZE_Pos) | (1U << DMA_CCR_MSIZE_Pos) |
                         (2U << DMA_CCR_PL_Pos);
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    /* ADC1 setup */
    ADC12_COMMON->CCR |= (1U << ADC_CCR_CKMODE_Pos) | ADC_CCR_VSENSESEL;
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    for (volatile int i = 0; i < 5000; i++);

    ADC1->CR &= ~ADC_CR_ADEN;
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);

    ADC1->ISR |= ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));

    /* Sequence: L=1 (2 conversions), SQ1=Channel 1, SQ2=Channel 16 (Temp) */
    ADC1->SQR1 = (1U << ADC_SQR1_L_Pos) | (1U << ADC_SQR1_SQ1_Pos) | (16U << ADC_SQR1_SQ2_Pos);
    ADC1->CFGR |= ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;

    /* Start continuous conversion */
    ADC1->CR |= ADC_CR_ADSTART;
}

int main(void) {
    DMA_ADC_Init();
    while (1) {
        /* CPU does not participate in data sampling; buffer fills autonomously */
        __WFI();
    }
}
```

## 6. Debugging & Register Verification Procedure
In GDB, inspect `adc_dma_buffer`. Verify that even elements track the potentiometer and odd elements reflect internal temperature sensor voltage.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to set `DMAMUX1_Channel0` routing ID to 5.
2. Setting `ADC_CFGR_DMAEN` without `ADC_CFGR_DMACFG` (results in a single burst rather than continuous circular sampling).

## 8. Expected Output & Verification
The memory buffer updates continuously in real-time with zero CPU execution cycles consumed.
