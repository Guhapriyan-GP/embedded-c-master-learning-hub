# Lab 11: SPI Master Interfacing with Sensor Registers

## 1. Learning Objectives
Build a bare-metal full-duplex SPI1 master driver on PA5 (SCK), PA6 (MISO), PA7 (MOSI) and communicate with an external sensor using Mode 3.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE, SPI digital sensor (e.g. BME280 or MPU6050/ICM-20948).
- **Wiring:** PA5 -> SCK, PA6 -> MISO, PA7 -> MOSI, PA4 -> CS (Chip Select).

## 3. Peripheral Registers & Official Documentation
- SPI1->CR1 (MSTR, CPOL, CPHA, BR, SPE, SSI, SSM)
- SPI1->CR2 (DS, FRXTH)
- SPI1->SR / DR

**Reference Documentation:** RM0440 Chapter 35 (SPI).

## 4. Mathematical Calculations & Bitfield Settings
PCLK2 = 170 MHz. Prescaler /16: SPI baud = 170 / 16 = 10.625 MHz.

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"

void SPI1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* PA5 (SCK), PA6 (MISO), PA7 (MOSI) -> AF5 */
    GPIOA->MODER &= ~((3U << 10) | (3U << 12) | (3U << 14));
    GPIOA->MODER |=  ((2U << 10) | (2U << 12) | (2U << 14));
    GPIOA->AFR[0] &= ~((0xFU << 20) | (0xFU << 24) | (0xFU << 28));
    GPIOA->AFR[0] |=  ((5U   << 20) | (5U   << 24) | (5U   << 28));

    /* PA4 as CS Output */
    GPIOA->MODER &= ~(3U << 8);
    GPIOA->MODER |=  (1U << 8);
    GPIOA->BSRR = (1U << 4); /* CS High (Inactive) */

    /* SPI1: Master, Mode 3 (CPOL=1, CPHA=1), Prescaler /16, 8-bit, Software NSS */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA |
                (3U << SPI_CR1_BR_Pos) | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR2 = (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH; /* 8-bit DS, 8-bit FIFO threshold */
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI1_Transfer(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    *(__IO uint8_t *)&SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return *(__IO uint8_t *)&SPI1->DR;
}

int main(void) {
    SPI1_Init();
    while (1) {
        GPIOA->BSRR = (1U << (4 + 16)); /* CS Low */
        uint8_t dev_id = SPI1_Transfer(0x80 | 0x00); /* Read register 0 */
        (void)dev_id;
        GPIOA->BSRR = (1U << 4);        /* CS High */
        for (volatile int i = 0; i < 1000000; i++);
    }
}
```

## 6. Debugging & Register Verification Procedure
Monitor the SPI lines with a logic analyzer. Verify CPOL idle state is HIGH, and 8 clock pulses occur during each transfer.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Forgetting to set `SPI_CR2_FRXTH` for 8-bit data transfers (causes `RXNE` to wait for 16 bits of data before asserting).
2. Leaving Software NSS disabled, causing a MODF fault.

## 8. Expected Output & Verification
The SPI master exchanges data with the sensor and receives the correct device WHO_AM_I ID byte.
