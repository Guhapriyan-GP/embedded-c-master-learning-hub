# Lab 12: I2C Master Interfacing with SSD1306 OLED Display

## 1. Learning Objectives
Configure I2C1 in Master Mode with custom TIMINGR values to drive an SSD1306 128x64 I2C OLED display at 400 kHz Fast Mode.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE, 0.96" SSD1306 I2C OLED display (Address 0x78 / 0x3C).
- **Wiring:** PB8 -> SCL, PB9 -> SDA, 3.3V -> VCC, GND -> GND. External 4.7k pull-up resistors.

## 3. Peripheral Registers & Official Documentation
- I2C1->CR1, CR2, TIMINGR, ISR, ICR, TXDR
- GPIOB->MODER (Open-drain on PB8/PB9)
- GPIOB->AFR[1] (AF4)

**Reference Documentation:** RM0440 Chapter 36 (I2C).

## 4. Mathematical Calculations & Bitfield Settings
TIMINGR calculated for 400 kHz Fast Mode from 170 MHz peripheral clock.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void I2C1_Init_400kHz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;

    /* PB8 (SCL), PB9 (SDA) -> Open-Drain, Alternate Function AF4 */
    GPIOB->MODER &= ~((3U << 16) | (3U << 18));
    GPIOB->MODER |=  ((2U << 16) | (2U << 18));
    GPIOB->OTYPER |= (1U << 8) | (1U << 9); /* Open-drain */
    GPIOB->PUPDR  |= (1U << 16) | (1U << 18); /* Pull-up */
    GPIOB->AFR[1] &= ~((0xFU << 0) | (0xFU << 4));
    GPIOB->AFR[1] |=  ((4U   << 0) | (4U   << 4));

    /* 400 kHz Fast Mode TIMINGR for 170 MHz I2C clock */
    I2C1->TIMINGR = 0x10802D9B;
    I2C1->CR1 |= I2C_CR1_PE;
}

void I2C1_Write(uint8_t dev_addr, uint8_t *data, uint16_t len) {
    I2C1->CR2 = (dev_addr & 0xFE) | ((uint32_t)len << I2C_CR2_NBYTES_Pos) |
                I2C_CR2_AUTOEND | I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        while (!(I2C1->ISR & I2C_ISR_TXIS));
        I2C1->TXDR = data[i];
    }
    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR = I2C_ICR_STOPCF;
}

int main(void) {
    I2C1_Init_400kHz();
    uint8_t init_cmd[] = {0x00, 0xAF}; /* Command: Display ON */
    I2C1_Write(0x78, init_cmd, sizeof(init_cmd));
    while (1);
}
```

## 6. Debugging & Register Verification Procedure
Probe SDA and SCL with an oscilloscope. Verify clean rise times (under 300 ns) and that the slave acknowledges (ACK pulled LOW on 9th clock).

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to configure GPIO pins as Open-Drain (`OTYPER = 1`).
2. Missing pull-up resistors causing the bus to hang in a continuous LOW state.

## 8. Expected Output & Verification
The OLED display initializes and turns on its pixel panel.
