# Lab 08: USART2 Bare-Metal Driver with Retargeted printf

## 1. Learning Objectives
Build a bare-metal serial driver on USART2 (PA2/PA3) communicating with the onboard ST-LINK Virtual COM port at 115,200 baud and retarget standard C printf.

## 2. Hardware Requirements & Wiring
- **Hardware:** NUCLEO-G474RE, PC running serial terminal (e.g. PuTTY, Tera Term, screen) at 115200 8N1.
- **Wiring:** PA2 (TX) and PA3 (RX) route directly through the onboard ST-LINK debugger to the host USB Virtual COM port.

## 3. Peripheral Registers & Official Documentation
- RCC->APB1ENR1 (USART2EN)
- GPIOA->AFR[0] (AF7 on PA2/PA3)
- USART2->BRR (Baud rate divisor)
- USART2->CR1 (TE, RE, UE)
- USART2->ISR / TDR / RDR

**Reference Documentation:** RM0440 Chapter 34 (USART) and UM2505 (ST-LINK VCP routing).

## 4. Mathematical Calculations & Bitfield Settings
PCLK1 = 170 MHz. 115200 baud: BRR = 170,000,000 / 115,200 = 1476 (0x05C4).

## 5. Complete Bare-Metal C Source Code
```c
#include "stm32g474xx.h"
#include <stdio.h>

void USART2_Init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* PA2 (TX), PA3 (RX) -> AF7 */
    GPIOA->MODER &= ~((3U << 4) | (3U << 6));
    GPIOA->MODER |=  ((2U << 4) | (2U << 6));
    GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA->AFR[0] |=  ((7U   << 8) | (7U   << 12));

    USART2->BRR = 1476U; /* 115200 baud @ 170 MHz */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        while (!(USART2->ISR & USART_ISR_TXE));
        USART2->TDR = (uint8_t)ptr[i];
    }
    return len;
}

int main(void) {
    USART2_Init();

    int counter = 0;
    while (1) {
        printf("[STM32G474RE] System Heartbeat #%d | Core: 170 MHz\r\n", counter++);
        for (volatile int i = 0; i < 2000000; i++) __NOP();
    }
}
```

## 6. Debugging & Register Verification Procedure
Open serial terminal on host PC at 115200 baud. If garbled characters appear, verify that PCLK1 is genuinely 170 MHz and BRR is 1476.

## 7. Common Beginner Mistakes & Diagnostic Guidance
1. Incorrect baud divisor calculation due to misidentifying peripheral bus clock.
2. Omitting `\r\n` newline formatting causing text overlap in terminals.

## 8. Expected Output & Verification
The host terminal displays structured telemetry messages clearly at 1-second intervals.
