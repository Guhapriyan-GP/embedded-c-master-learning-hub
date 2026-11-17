# ADC & DAC (Analog Engine) Register Map

**Target MCU:** STM32G474RE  
**ADC1 Base:** `0x50000000` (`AHB2PERIPH_BASE + 0x08000000UL`)  
**ADC Common Base:** `0x50000300` (`ADC12_COMMON`)  
**DAC1 Base:** `0x50000800`  
**Reference Manual:** RM0440, Chapter 21 (*ADC*) & Chapter 22 (*DAC*)

---

### 1. High-Speed 12-Bit SAR ADC Register Overview

The STM32G4 features up to five independent successive approximation register (SAR) ADCs capable of sampling up to 4 Msps with integrated hardware oversamplers (up to 16-bit resolution).

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `ADC_ISR` | `0x00` | `0x00000000` | Interrupt and status register (ADRDY ready, EOC conversion complete, EOS sequence complete) |
| `ADC_IER` | `0x04` | `0x00000000` | Interrupt enable register (ADRDYIE, EOCIE, EOSIE) |
| `ADC_CR` | `0x08` | `0x00000000` | Control register (ADEN enable, ADSTART start, ADVREGEN voltage regulator, ADCAL calibration) |
| `ADC_CFGR` | `0x0C` | `0x80000000` | Configuration register (RES resolution, CONT continuous mode, DMAEN, DMACFG circular) |
| `ADC_SMPR1`| `0x14` | `0x00000000` | Sample time register 1 (Channels 0 to 9; 3 bits per channel) |
| `ADC_SMPR2`| `0x18` | `0x00000000` | Sample time register 2 (Channels 10 to 18) |
| `ADC_SQR1` | `0x30` | `0x00000000` | Regular sequence register 1 (L sequence length: 1-16; SQ1 to SQ4 channel mapping) |
| `ADC_DR` | `0x40` | `0x00000000` | Regular data register (Contains 12-bit conversion result) |

---

### 2. Mandatory Power-Up & Calibration Sequence
Before an ADC conversion can take place, the hardware requires a strict power-up protocol:
1. **Enable Internal Voltage Regulator:** Set `ADVREGEN` bit in `ADC_CR`. Wait minimum $20\ \mu\text{s}$ for regulator stabilization.
2. **Execute Hardware Self-Calibration:** Ensure `ADEN = 0`. Set `ADCAL` in `ADC_CR`. Wait until `ADCAL` hardware clears to `0`.
3. **Enable ADC Engine:** Set `ADEN` in `ADC_CR`. Wait until the hardware sets the `ADRDY` flag in `ADC_ISR`.

---

### 3. Sampling Time & Conversion Mathematics

Total conversion time ($t_{\text{CONV}}$) is the sum of the sampling time ($t_{\text{SMP}}$) and the successive approximation time ($t_{\text{SAR}}$):

$$t_{\text{CONV}} = t_{\text{SMP}} + t_{\text{SAR}} = t_{\text{SMP}} + 12.5 \text{ ADC clock cycles (for 12-bit)}$$

**SMPx Bit Encoding (Sample Cycles):**
- `000`: 2.5 cycles
- `001`: 6.5 cycles
- `010`: 12.5 cycles
- `011`: 24.5 cycles
- `100`: 47.5 cycles
- `101`: 92.5 cycles
- `110`: 247.5 cycles
- `111`: 640.5 cycles

---

### 4. CMSIS Code Example: Single-Shot ADC1 Channel 1 Conversion (PA0)
```c
void ADC1_Init(void) {
    /* 1. Enable ADC1/2 bus clock (AHB2) and GPIOA clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;

    /* 2. Configure PA0 as Analog Mode (MODER0 = 11b) */
    GPIOA->MODER |= (3U << (0U * 2U));

    /* 3. Select Synchronous Clock (HCLK/1 = 170 MHz) in Common Control Register */
    ADC12_COMMON->CCR &= ~ADC_CCR_CKMODE;
    ADC12_COMMON->CCR |= (1U << ADC_CCR_CKMODE_Pos); /* HCLK / 1 */

    /* 4. Enable Internal Voltage Regulator */
    ADC1->CR &= ~ADC_CR_DEEPPWD; /* Exit deep power-down */
    ADC1->CR |= ADC_CR_ADVREGEN;
    for (volatile int i = 0; i < 5000; i++); /* Wait > 20 microseconds */

    /* 5. Start Calibration */
    ADC1->CR &= ~ADC_CR_ADEN;     /* Ensure ADEN is 0 */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL); /* Wait for calibration to finish */

    /* 6. Enable ADC Engine */
    ADC1->ISR |= ADC_ISR_ADRDY;   /* Clear ADRDY flag */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)); /* Wait until ADC is ready */

    /* 7. Configure Sequence: 1 Conversion (L=0), Channel 1 on 1st Conversion */
    ADC1->SQR1 &= ~ADC_SQR1_L;    /* L = 0 (1 conversion) */
    ADC1->SQR1 &= ~ADC_SQR1_SQ1;
    ADC1->SQR1 |= (1U << ADC_SQR1_SQ1_Pos); /* Channel 1 */

    /* 8. Configure Sampling Time: 47.5 cycles for Channel 1 */
    ADC1->SMPR1 |= (4U << ADC_SMPR1_SMP1_Pos);
}

uint16_t ADC1_Read(void) {
    /* Start conversion */
    ADC1->CR |= ADC_CR_ADSTART;
    
    /* Wait for End of Conversion (EOC) */
    while (!(ADC1->ISR & ADC_ISR_EOC));
    
    /* Read 12-bit data (automatically clears EOC flag) */
    return (uint16_t)ADC1->DR;
}
```
