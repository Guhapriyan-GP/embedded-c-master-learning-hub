/**
 * @file    stm32g474xx.h
 * @brief   Minimal host-test CMSIS mock header for STM32G474RE register-level unit tests.
 *
 * All struct layouts, offsets, memory map bounds, and bit masks are verified 1:1
 * against `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h`
 * and `23-code-library/Drivers/CMSIS/Core/Include/core_cm4.h`.
 *
 * Used strictly on the host compiler (never included in ARM cross-builds).
 */

#ifndef MOCK_STM32G474XX_H
#define MOCK_STM32G474XX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* STM32G474RE SRAM & CCM-SRAM Memory Map (verified vs stm32g474xx.h lines 1133-1152) */
#define SRAM1_BASE          (0x20000000UL)
#define SRAM2_BASE          (0x20014000UL)
#define SRAM2_SIZE          (0x00004000UL)
#define CCMSRAM_BASE        (0x10000000UL)
#define CCMSRAM_SIZE        (0x00008000UL)

/* STM32G474RE GPIO_TypeDef (verified vs stm32g474xx.h & RM0440 Ch. 9) */
typedef struct {
    volatile uint32_t MODER;   /* 0x00 */
    volatile uint32_t OTYPER;  /* 0x04 */
    volatile uint32_t OSPEEDR; /* 0x08 */
    volatile uint32_t PUPDR;   /* 0x0C */
    volatile uint32_t IDR;     /* 0x10 */
    volatile uint32_t ODR;     /* 0x14 */
    volatile uint32_t BSRR;    /* 0x18 */
    volatile uint32_t LCKR;    /* 0x1C */
    volatile uint32_t AFR[2];  /* 0x20, 0x24 */
    volatile uint32_t BRR;     /* 0x28 */
} GPIO_TypeDef;

/* Minimal RCC_TypeDef for GPIO_EnableClock() testing */
typedef struct {
    volatile uint32_t AHB2ENR;
} RCC_TypeDef;

/* Fake backing peripheral instances defined in test_gpio_mock.c */
extern GPIO_TypeDef g_mock_gpioa;
extern GPIO_TypeDef g_mock_gpiob;
extern GPIO_TypeDef g_mock_gpioc;
extern GPIO_TypeDef g_mock_gpiod;
extern GPIO_TypeDef g_mock_gpioe;
extern GPIO_TypeDef g_mock_gpiof;
extern GPIO_TypeDef g_mock_gpiog;
extern RCC_TypeDef  g_mock_rcc;

#define GPIOA (&g_mock_gpioa)
#define GPIOB (&g_mock_gpiob)
#define GPIOC (&g_mock_gpioc)
#define GPIOD (&g_mock_gpiod)
#define GPIOE (&g_mock_gpioe)
#define GPIOF (&g_mock_gpiof)
#define GPIOG (&g_mock_gpiog)
#define RCC   (&g_mock_rcc)

/* RCC->AHB2ENR GPIO clock enable bits (verified vs stm32g474xx.h lines 12338-12358) */
#define RCC_AHB2ENR_GPIOAEN (1UL << 0U)
#define RCC_AHB2ENR_GPIOBEN (1UL << 1U)
#define RCC_AHB2ENR_GPIOCEN (1UL << 2U)
#define RCC_AHB2ENR_GPIODEN (1UL << 3U)
#define RCC_AHB2ENR_GPIOEEN (1UL << 4U)
#define RCC_AHB2ENR_GPIOFEN (1UL << 5U)
#define RCC_AHB2ENR_GPIOGEN (1UL << 6U)

/* GPIO->LCKR lock key bit (verified vs stm32g474xx.h lines 6675-6677) */
#define GPIO_LCKR_LCKK_Pos  (16U)
#define GPIO_LCKR_LCKK_Msk  (1UL << GPIO_LCKR_LCKK_Pos)
#define GPIO_LCKR_LCKK      GPIO_LCKR_LCKK_Msk

/* Cortex-M4 SCB CFSR / HFSR masks (verified vs core_cm4.h lines 616-693) */
#define SCB_CFSR_MEMFAULTSR_Msk (0xFFUL << 0U)
#define SCB_CFSR_BUSFAULTSR_Msk (0xFFUL << 8U)
#define SCB_CFSR_USGFAULTSR_Msk (0xFFFFUL << 16U)
#define SCB_CFSR_MMARVALID_Msk  (1UL << 7U)
#define SCB_CFSR_BFARVALID_Msk  (1UL << 15U)
#define SCB_CFSR_UNDEFINSTR_Msk (1UL << 16U)
#define SCB_CFSR_INVSTATE_Msk   (1UL << 17U)
#define SCB_CFSR_UNALIGNED_Msk  (1UL << 24U)
#define SCB_CFSR_DIVBYZERO_Msk  (1UL << 25U)
#define SCB_HFSR_VECTTBL_Msk    (1UL << 1U)
#define SCB_HFSR_FORCED_Msk     (1UL << 30U)

#ifdef __cplusplus
}
#endif

#endif /* MOCK_STM32G474XX_H */
