/**
 * @file    rtos_bsp.h
 * @brief   Self-contained NUCLEO-G474RE Board Support Package for FreeRTOS
 *          verification firmware (170 MHz Boost clock, USART2 ST-LINK VCP,
 *          PA5 LD2 LED, PC13 B1 button EXTI13, TIM2 10 Hz IRQ, IWDG + DBGMCU).
 */

#ifndef RTOS_BSP_H
#define RTOS_BSP_H

#include <stdbool.h>
#include <stdint.h>

/* Clock initialization: RM0440 §6.1.5 Boost Mode 170 MHz from HSI16 */
bool     BSP_Clock_Init_170MHz(void);

/* Reset cause capture & clear (RCC->CSR) */
uint32_t BSP_GetAndClearResetFlags(void);

/* Onboard LED LD2 (PA5, Active-HIGH) */
void     BSP_LED_Init(void);
void     BSP_LED_On(void);
void     BSP_LED_Off(void);
void     BSP_LED_Toggle(void);

/* Onboard User Button B1 (PC13, Active-HIGH, EXTI13 on NVIC priority >= 5) */
void     BSP_Button_Init(uint32_t nvic_priority);
bool     BSP_Button_Read(void);

/* ST-LINK Virtual COM Port USART2 (PA2 TX / PA3 RX, AF7, 115200 8N1) */
void     BSP_USART2_Init_115200(void);
void     BSP_USART2_WriteChar(char c);
void     BSP_USART2_WriteStr(const char *str);
void     BSP_USART2_Printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* TIM2 10 Hz periodic hardware interrupt for FromISR FreeRTOS testing */
void     BSP_TIM2_Init_10Hz(uint32_t nvic_priority);
void     BSP_TIM2_Start(void);

/* IWDG watchdog (~3.0 s timeout, LSI/32 = 1 kHz, RLR = 3000) with DBGMCU freeze */
bool     BSP_IWDG_Init_3000ms(void);
void     BSP_IWDG_Refresh(void);

/* Safe debug breakpoint (only fires if CoreDebug C_DEBUGEN == 1) */
void     BSP_DebugBreakIfAttached(void);

#endif /* RTOS_BSP_H */
