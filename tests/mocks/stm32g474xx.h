/**
 * @file    stm32g474xx.h
 * @brief   Host-test CMSIS mock header for STM32G474RE register-level unit tests.
 *
 * All struct layouts, memory map bounds, IRQ numbers, and register bit masks
 * are verified 1:1 against:
 *   - `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h`
 *   - `23-code-library/Drivers/CMSIS/Core/Include/core_cm4.h`
 *
 * Used strictly on the host compiler (never included in ARM cross-builds).
 */

#ifndef MOCK_STM32G474XX_H
#define MOCK_STM32G474XX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __IO
#define __IO volatile
#endif

/* ============================================================================
 * STM32G474RE SRAM & CCM-SRAM Memory Map (verified vs stm32g474xx.h)
 * ========================================================================== */
#define SRAM1_BASE          (0x20000000UL)
#define SRAM2_BASE          (0x20014000UL)
#define SRAM2_SIZE          (0x00004000UL)
#define CCMSRAM_BASE        (0x10000000UL)
#define CCMSRAM_SIZE        (0x00008000UL)

/* ============================================================================
 * STM32G474RE IRQ Numbers (verified vs stm32g474xx.h enum IRQn_Type)
 * ========================================================================== */
typedef enum {
    EXTI0_IRQn         = 6,
    EXTI1_IRQn         = 7,
    EXTI2_IRQn         = 8,
    EXTI3_IRQn         = 9,
    EXTI4_IRQn         = 10,
    DMA1_Channel1_IRQn = 11,
    DMA1_Channel2_IRQn = 12,
    DMA1_Channel3_IRQn = 13,
    DMA1_Channel4_IRQn = 14,
    DMA1_Channel5_IRQn = 15,
    DMA1_Channel6_IRQn = 16,
    DMA1_Channel7_IRQn = 17,
    EXTI9_5_IRQn       = 23,
    TIM2_IRQn          = 28,
    TIM3_IRQn          = 29,
    USART1_IRQn        = 37,
    USART2_IRQn        = 38,
    EXTI15_10_IRQn     = 40,
    TIM6_DAC_IRQn      = 54,
    TIM7_DAC_IRQn      = 55,
    DMA2_Channel1_IRQn = 56,
    DMA2_Channel2_IRQn = 57,
    DMA2_Channel3_IRQn = 58,
    DMA2_Channel4_IRQn = 59,
    DMA2_Channel5_IRQn = 60,
    LPUART1_IRQn       = 91,
    DMA1_Channel8_IRQn = 96,
    DMA2_Channel6_IRQn = 97,
    DMA2_Channel7_IRQn = 98,
    DMA2_Channel8_IRQn = 99
} IRQn_Type;

/* ============================================================================
 * Peripheral Register Struct Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */

/* GPIO_TypeDef (RM0440 Ch. 9) */
typedef struct {
    __IO uint32_t MODER;   /* 0x00 */
    __IO uint32_t OTYPER;  /* 0x04 */
    __IO uint32_t OSPEEDR; /* 0x08 */
    __IO uint32_t PUPDR;   /* 0x0C */
    __IO uint32_t IDR;     /* 0x10 */
    __IO uint32_t ODR;     /* 0x14 */
    __IO uint32_t BSRR;    /* 0x18 */
    __IO uint32_t LCKR;    /* 0x1C */
    __IO uint32_t AFR[2];  /* 0x20, 0x24 */
    __IO uint32_t BRR;     /* 0x28 */
} GPIO_TypeDef;

/* RCC_TypeDef (minimal subset for peripheral drivers) */
typedef struct {
    __IO uint32_t CFGR;
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
    __IO uint32_t APB1ENR1;
    __IO uint32_t APB1ENR2;
    __IO uint32_t APB2ENR;
    __IO uint32_t CSR;
} RCC_TypeDef;

/* SYSCFG_TypeDef (RM0440 Ch. 10) */
typedef struct {
    __IO uint32_t MEMRMP;
    __IO uint32_t CFGR1;
    __IO uint32_t EXTICR[4];
} SYSCFG_TypeDef;

/* EXTI_TypeDef (RM0440 Ch. 15) */
typedef struct {
    __IO uint32_t IMR1;
    __IO uint32_t EMR1;
    __IO uint32_t RTSR1;
    __IO uint32_t FTSR1;
    __IO uint32_t SWIER1;
    __IO uint32_t PR1;
} EXTI_TypeDef;

/* TIM_TypeDef (RM0440 Ch. 29/31) */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMCR;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    __IO uint32_t CCMR1;
    __IO uint32_t CCMR2;
    __IO uint32_t CCER;
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
    __IO uint32_t CCR1;
    __IO uint32_t CCR2;
    __IO uint32_t CCR3;
    __IO uint32_t CCR4;
} TIM_TypeDef;

/* USART_TypeDef (RM0440 Ch. 40) */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t CR3;
    __IO uint32_t BRR;
    __IO uint32_t GTPR;
    __IO uint32_t RTOR;
    __IO uint32_t RQR;
    __IO uint32_t ISR;
    __IO uint32_t ICR;
    __IO uint32_t RDR;
    __IO uint32_t TDR;
} USART_TypeDef;

/* SPI_TypeDef (RM0440 Ch. 42) */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SR;
    __IO uint32_t DR;
} SPI_TypeDef;

/* I2C_TypeDef (RM0440 Ch. 39) */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t OAR1;
    __IO uint32_t OAR2;
    __IO uint32_t TIMINGR;
    __IO uint32_t TIMEOUTR;
    __IO uint32_t ISR;
    __IO uint32_t ICR;
    __IO uint32_t PECR;
    __IO uint32_t RXDR;
    __IO uint32_t TXDR;
} I2C_TypeDef;

/* ADC_TypeDef & ADC_Common_TypeDef (RM0440 Ch. 21) */
typedef struct {
    __IO uint32_t ISR;
    __IO uint32_t IER;
    __IO uint32_t CR;
    __IO uint32_t CFGR;
    __IO uint32_t CFGR2;
    __IO uint32_t SMPR1;
    __IO uint32_t SMPR2;
    __IO uint32_t SQR1;
    __IO uint32_t DR;
} ADC_TypeDef;

typedef struct {
    __IO uint32_t CSR;
    __IO uint32_t CCR;
    __IO uint32_t CDR;
} ADC_Common_TypeDef;

/* DMA_TypeDef, DMA_Channel_TypeDef, DMAMUX_Channel_TypeDef (RM0440 Ch. 11/12) */
typedef struct {
    __IO uint32_t ISR;
    __IO uint32_t IFCR;
} DMA_TypeDef;

typedef struct {
    __IO uint32_t CCR;
    __IO uint32_t CNDTR;
    __IO uint32_t CPAR;
    __IO uint32_t CMAR;
} DMA_Channel_TypeDef;

typedef struct {
    __IO uint32_t CCR;
} DMAMUX_Channel_TypeDef;

/* IWDG_TypeDef & DBGMCU_TypeDef (RM0440 Ch. 35/47) */
typedef struct {
    __IO uint32_t KR;
    __IO uint32_t PR;
    __IO uint32_t RLR;
    __IO uint32_t SR;
    __IO uint32_t WINR;
} IWDG_TypeDef;

typedef struct {
    __IO uint32_t IDCODE;
    __IO uint32_t CR;
    __IO uint32_t APB1FZR1;
    __IO uint32_t APB1FZR2;
    __IO uint32_t APB2FZ;
} DBGMCU_TypeDef;

/* ============================================================================
 * Mock NVIC & Barrier Instrumentation State
 * ========================================================================== */
#define MOCK_NVIC_MAX_IRQ (128U)

typedef struct {
    uint32_t priority[MOCK_NVIC_MAX_IRQ];
    uint8_t  enabled[MOCK_NVIC_MAX_IRQ];
    uint32_t clear_pending_cnt[MOCK_NVIC_MAX_IRQ];
    uint32_t dsb_count;
} mock_core_state_t;

extern mock_core_state_t g_mock_core;
extern uint32_t          SystemCoreClock;
extern const uint8_t     APBPrescTable[8];

static inline void __DSB(void) {
    g_mock_core.dsb_count++;
}

static inline void NVIC_SetPriority(IRQn_Type irqn, uint32_t priority) {
    if (((int32_t)irqn >= 0) && ((uint32_t)irqn < MOCK_NVIC_MAX_IRQ)) {
        g_mock_core.priority[(uint32_t)irqn] = priority;
    }
}

static inline void NVIC_ClearPendingIRQ(IRQn_Type irqn) {
    if (((int32_t)irqn >= 0) && ((uint32_t)irqn < MOCK_NVIC_MAX_IRQ)) {
        g_mock_core.clear_pending_cnt[(uint32_t)irqn]++;
    }
}

static inline void NVIC_EnableIRQ(IRQn_Type irqn) {
    if (((int32_t)irqn >= 0) && ((uint32_t)irqn < MOCK_NVIC_MAX_IRQ)) {
        g_mock_core.enabled[(uint32_t)irqn] = 1U;
    }
}

/* ============================================================================
 * Fake Backing Peripheral Instances
 * ========================================================================== */
extern GPIO_TypeDef           g_mock_gpioa;
extern GPIO_TypeDef           g_mock_gpiob;
extern GPIO_TypeDef           g_mock_gpioc;
extern GPIO_TypeDef           g_mock_gpiod;
extern GPIO_TypeDef           g_mock_gpioe;
extern GPIO_TypeDef           g_mock_gpiof;
extern GPIO_TypeDef           g_mock_gpiog;
extern RCC_TypeDef            g_mock_rcc;
extern SYSCFG_TypeDef         g_mock_syscfg;
extern EXTI_TypeDef           g_mock_exti;
extern TIM_TypeDef            g_mock_tim2;
extern TIM_TypeDef            g_mock_tim3;
extern TIM_TypeDef            g_mock_tim6;
extern TIM_TypeDef            g_mock_tim7;
extern USART_TypeDef          g_mock_usart1;
extern USART_TypeDef          g_mock_usart2;
extern USART_TypeDef          g_mock_lpuart1;
extern SPI_TypeDef            g_mock_spi1;
extern SPI_TypeDef            g_mock_spi2;
extern SPI_TypeDef            g_mock_spi3;
extern I2C_TypeDef            g_mock_i2c1;
extern I2C_TypeDef            g_mock_i2c2;
extern I2C_TypeDef            g_mock_i2c3;
extern ADC_TypeDef            g_mock_adc1;
extern ADC_TypeDef            g_mock_adc2;
extern ADC_Common_TypeDef     g_mock_adc12_common;
extern DMA_TypeDef            g_mock_dma1;
extern DMA_TypeDef            g_mock_dma2;
extern DMA_Channel_TypeDef    g_mock_dma1_ch[8];
extern DMA_Channel_TypeDef    g_mock_dma2_ch[8];
extern DMAMUX_Channel_TypeDef g_mock_dmamux1_ch[16];
extern IWDG_TypeDef           g_mock_iwdg;
extern DBGMCU_TypeDef         g_mock_dbgmcu;

#define GPIOA           (&g_mock_gpioa)
#define GPIOB           (&g_mock_gpiob)
#define GPIOC           (&g_mock_gpioc)
#define GPIOD           (&g_mock_gpiod)
#define GPIOE           (&g_mock_gpioe)
#define GPIOF           (&g_mock_gpiof)
#define GPIOG           (&g_mock_gpiog)
#define RCC             (&g_mock_rcc)
#define SYSCFG          (&g_mock_syscfg)
#define EXTI            (&g_mock_exti)
#define TIM2            (&g_mock_tim2)
#define TIM3            (&g_mock_tim3)
#define TIM6            (&g_mock_tim6)
#define TIM7            (&g_mock_tim7)
#define USART1          (&g_mock_usart1)
#define USART2          (&g_mock_usart2)
#define LPUART1         (&g_mock_lpuart1)
#define SPI1            (&g_mock_spi1)
#define SPI2            (&g_mock_spi2)
#define SPI3            (&g_mock_spi3)
#define I2C1            (&g_mock_i2c1)
#define I2C2            (&g_mock_i2c2)
#define I2C3            (&g_mock_i2c3)
#define ADC1            (&g_mock_adc1)
#define ADC2            (&g_mock_adc2)
#define ADC12_COMMON    (&g_mock_adc12_common)
#define DMA1            (&g_mock_dma1)
#define DMA2            (&g_mock_dma2)
#define DMA1_Channel1   (&g_mock_dma1_ch[0])
#define DMA1_Channel2   (&g_mock_dma1_ch[1])
#define DMA1_Channel3   (&g_mock_dma1_ch[2])
#define DMA1_Channel4   (&g_mock_dma1_ch[3])
#define DMA1_Channel5   (&g_mock_dma1_ch[4])
#define DMA1_Channel6   (&g_mock_dma1_ch[5])
#define DMA1_Channel7   (&g_mock_dma1_ch[6])
#define DMA1_Channel8   (&g_mock_dma1_ch[7])
#define DMA2_Channel1   (&g_mock_dma2_ch[0])
#define DMA2_Channel2   (&g_mock_dma2_ch[1])
#define DMA2_Channel3   (&g_mock_dma2_ch[2])
#define DMA2_Channel4   (&g_mock_dma2_ch[3])
#define DMA2_Channel5   (&g_mock_dma2_ch[4])
#define DMA2_Channel6   (&g_mock_dma2_ch[5])
#define DMA2_Channel7   (&g_mock_dma2_ch[6])
#define DMA2_Channel8   (&g_mock_dma2_ch[7])
#define DMAMUX1_Channel0  (&g_mock_dmamux1_ch[0])
#define DMAMUX1_Channel1  (&g_mock_dmamux1_ch[1])
#define DMAMUX1_Channel2  (&g_mock_dmamux1_ch[2])
#define DMAMUX1_Channel3  (&g_mock_dmamux1_ch[3])
#define DMAMUX1_Channel4  (&g_mock_dmamux1_ch[4])
#define DMAMUX1_Channel5  (&g_mock_dmamux1_ch[5])
#define DMAMUX1_Channel6  (&g_mock_dmamux1_ch[6])
#define DMAMUX1_Channel7  (&g_mock_dmamux1_ch[7])
#define DMAMUX1_Channel8  (&g_mock_dmamux1_ch[8])
#define DMAMUX1_Channel9  (&g_mock_dmamux1_ch[9])
#define DMAMUX1_Channel10 (&g_mock_dmamux1_ch[10])
#define DMAMUX1_Channel11 (&g_mock_dmamux1_ch[11])
#define DMAMUX1_Channel12 (&g_mock_dmamux1_ch[12])
#define DMAMUX1_Channel13 (&g_mock_dmamux1_ch[13])
#define DMAMUX1_Channel14 (&g_mock_dmamux1_ch[14])
#define DMAMUX1_Channel15 (&g_mock_dmamux1_ch[15])
#define IWDG            (&g_mock_iwdg)
#define DBGMCU          (&g_mock_dbgmcu)

/* ============================================================================
 * RCC Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define RCC_CFGR_PPRE1_Pos        (8U)
#define RCC_CFGR_PPRE1_Msk        (0x7UL << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PPRE1            RCC_CFGR_PPRE1_Msk
#define RCC_CFGR_PPRE2_Pos        (11U)
#define RCC_CFGR_PPRE2_Msk        (0x7UL << RCC_CFGR_PPRE2_Pos)
#define RCC_CFGR_PPRE2            RCC_CFGR_PPRE2_Msk

#define RCC_AHB1ENR_DMA1EN        (1UL << 0U)
#define RCC_AHB1ENR_DMA2EN        (1UL << 1U)
#define RCC_AHB1ENR_DMAMUX1EN     (1UL << 2U)

#define RCC_AHB2ENR_GPIOAEN       (1UL << 0U)
#define RCC_AHB2ENR_GPIOBEN       (1UL << 1U)
#define RCC_AHB2ENR_GPIOCEN       (1UL << 2U)
#define RCC_AHB2ENR_GPIODEN       (1UL << 3U)
#define RCC_AHB2ENR_GPIOEEN       (1UL << 4U)
#define RCC_AHB2ENR_GPIOFEN       (1UL << 5U)
#define RCC_AHB2ENR_GPIOGEN       (1UL << 6U)
#define RCC_AHB2ENR_ADC12EN       (1UL << 13U)

#define RCC_APB1ENR1_TIM2EN       (1UL << 0U)
#define RCC_APB1ENR1_TIM3EN       (1UL << 1U)
#define RCC_APB1ENR1_TIM6EN       (1UL << 4U)
#define RCC_APB1ENR1_TIM7EN       (1UL << 5U)
#define RCC_APB1ENR1_SPI2EN       (1UL << 14U)
#define RCC_APB1ENR1_SPI3EN       (1UL << 15U)
#define RCC_APB1ENR1_USART2EN     (1UL << 17U)
#define RCC_APB1ENR1_I2C1EN       (1UL << 21U)
#define RCC_APB1ENR1_I2C2EN       (1UL << 22U)
#define RCC_APB1ENR1_I2C3EN       (1UL << 30U)

#define RCC_APB1ENR2_LPUART1EN    (1UL << 0U)

#define RCC_APB2ENR_SYSCFGEN      (1UL << 0U)
#define RCC_APB2ENR_SPI1EN        (1UL << 12U)
#define RCC_APB2ENR_USART1EN      (1UL << 14U)

#define RCC_CSR_LSION             (1UL << 0U)
#define RCC_CSR_LSIRDY            (1UL << 1U)
#define RCC_CSR_RMVF              (1UL << 23U)
#define RCC_CSR_IWDGRSTF          (1UL << 29U)

/* ============================================================================
 * GPIO Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define GPIO_LCKR_LCKK_Pos        (16U)
#define GPIO_LCKR_LCKK_Msk        (1UL << GPIO_LCKR_LCKK_Pos)
#define GPIO_LCKR_LCKK            GPIO_LCKR_LCKK_Msk

/* ============================================================================
 * TIM Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define TIM_CR1_CEN               (1UL << 0U)
#define TIM_CR1_ARPE              (1UL << 7U)
#define TIM_DIER_UIE              (1UL << 0U)
#define TIM_SR_UIF                (1UL << 0U)
#define TIM_EGR_UG                (1UL << 0U)

#define TIM_CCMR1_CC1S            (0x3UL << 0U)
#define TIM_CCMR1_OC1PE           (1UL << 3U)
#define TIM_CCMR1_OC1M_Pos        (4U)
#define TIM_CCMR1_OC1M            (0x1007UL << TIM_CCMR1_OC1M_Pos)
#define TIM_CCMR1_CC2S            (0x3UL << 8U)
#define TIM_CCMR1_OC2PE           (1UL << 11U)
#define TIM_CCMR1_OC2M_Pos        (12U)
#define TIM_CCMR1_OC2M            (0x1007UL << TIM_CCMR1_OC2M_Pos)

#define TIM_CCMR2_CC3S            (0x3UL << 0U)
#define TIM_CCMR2_OC3PE           (1UL << 3U)
#define TIM_CCMR2_OC3M_Pos        (4U)
#define TIM_CCMR2_OC3M            (0x1007UL << TIM_CCMR2_OC3M_Pos)
#define TIM_CCMR2_CC4S            (0x3UL << 8U)
#define TIM_CCMR2_OC4PE           (1UL << 11U)
#define TIM_CCMR2_OC4M_Pos        (12U)
#define TIM_CCMR2_OC4M            (0x1007UL << TIM_CCMR2_OC4M_Pos)

#define TIM_CCER_CC1E             (1UL << 0U)
#define TIM_CCER_CC1P             (1UL << 1U)
#define TIM_CCER_CC2E             (1UL << 4U)
#define TIM_CCER_CC2P             (1UL << 5U)
#define TIM_CCER_CC3E             (1UL << 8U)
#define TIM_CCER_CC3P             (1UL << 9U)
#define TIM_CCER_CC4E             (1UL << 12U)
#define TIM_CCER_CC4P             (1UL << 13U)

/* ============================================================================
 * USART Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define USART_CR1_UE              (1UL << 0U)
#define USART_CR1_RE              (1UL << 2U)
#define USART_CR1_TE              (1UL << 3U)
#define USART_CR1_RXNEIE_RXFNEIE  (1UL << 5U)

#define USART_ISR_PE              (1UL << 0U)
#define USART_ISR_FE              (1UL << 1U)
#define USART_ISR_NE              (1UL << 2U)
#define USART_ISR_ORE             (1UL << 3U)
#define USART_ISR_RXNE_RXFNE      (1UL << 5U)
#define USART_ISR_TC              (1UL << 6U)
#define USART_ISR_TXE_TXFNF       (1UL << 7U)

#define USART_ICR_PECF            (1UL << 0U)
#define USART_ICR_FECF            (1UL << 1U)
#define USART_ICR_NECF            (1UL << 2U)
#define USART_ICR_ORECF           (1UL << 3U)

/* ============================================================================
 * SPI Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define SPI_CR1_CPHA              (1UL << 0U)
#define SPI_CR1_CPOL              (1UL << 1U)
#define SPI_CR1_MSTR              (1UL << 2U)
#define SPI_CR1_BR_Pos            (3U)
#define SPI_CR1_BR                (0x7UL << SPI_CR1_BR_Pos)
#define SPI_CR1_SPE               (1UL << 6U)
#define SPI_CR1_SSI               (1UL << 8U)
#define SPI_CR1_SSM               (1UL << 9U)

#define SPI_CR2_DS_Pos            (8U)
#define SPI_CR2_DS                (0xFUL << SPI_CR2_DS_Pos)
#define SPI_CR2_FRXTH             (1UL << 12U)

#define SPI_SR_RXNE               (1UL << 0U)
#define SPI_SR_TXE                (1UL << 1U)
#define SPI_SR_BSY                (1UL << 7U)
#define SPI_SR_FTLVL_Pos          (11U)
#define SPI_SR_FTLVL              (0x3UL << SPI_SR_FTLVL_Pos)

/* ============================================================================
 * I2C Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define I2C_CR1_PE                (1UL << 0U)

#define I2C_CR2_SADD_Pos          (0U)
#define I2C_CR2_SADD_Msk          (0x3FFUL << I2C_CR2_SADD_Pos)
#define I2C_CR2_RD_WRN            (1UL << 10U)
#define I2C_CR2_START             (1UL << 13U)
#define I2C_CR2_NBYTES_Pos        (16U)
#define I2C_CR2_NBYTES_Msk        (0xFFUL << I2C_CR2_NBYTES_Pos)
#define I2C_CR2_RELOAD            (1UL << 24U)
#define I2C_CR2_AUTOEND           (1UL << 25U)

#define I2C_ISR_TXE               (1UL << 0U)
#define I2C_ISR_TXIS              (1UL << 1U)
#define I2C_ISR_RXNE              (1UL << 2U)
#define I2C_ISR_NACKF             (1UL << 4U)
#define I2C_ISR_STOPF             (1UL << 5U)
#define I2C_ISR_TC                (1UL << 6U)
#define I2C_ISR_TCR               (1UL << 7U)
#define I2C_ISR_BERR              (1UL << 8U)
#define I2C_ISR_ARLO              (1UL << 9U)
#define I2C_ISR_BUSY              (1UL << 15U)

#define I2C_ICR_NACKCF            (1UL << 4U)
#define I2C_ICR_STOPCF            (1UL << 5U)
#define I2C_ICR_BERRCF            (1UL << 8U)
#define I2C_ICR_ARLOCF            (1UL << 9U)

/* ============================================================================
 * ADC Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define ADC_ISR_ADRDY             (1UL << 0U)
#define ADC_ISR_EOC               (1UL << 2U)
#define ADC_ISR_EOS               (1UL << 3U)

#define ADC_CR_ADEN               (1UL << 0U)
#define ADC_CR_ADDIS              (1UL << 1U)
#define ADC_CR_ADSTART            (1UL << 2U)
#define ADC_CR_ADVREGEN           (1UL << 28U)
#define ADC_CR_DEEPPWD            (1UL << 29U)
#define ADC_CR_ADCALDIF           (1UL << 30U)
#define ADC_CR_ADCAL              (1UL << 31U)

#define ADC_CFGR_DMAEN            (1UL << 0U)
#define ADC_CFGR_DMACFG           (1UL << 1U)
#define ADC_CFGR_RES              (0x3UL << 3U)
#define ADC_CFGR_EXTEN            (0x3UL << 10U)
#define ADC_CFGR_CONT             (1UL << 13U)
#define ADC_CFGR_ALIGN            (1UL << 15U)

#define ADC_SQR1_SQ1_Pos          (6U)
#define ADC_CCR_CKMODE_Pos        (16U)
#define ADC_CCR_CKMODE            (0x3UL << ADC_CCR_CKMODE_Pos)
#define ADC_CCR_VREFEN            (1UL << 22U)
#define ADC_CCR_VSENSESEL         (1UL << 23U)

/* ============================================================================
 * DMA / DMAMUX Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define DMA_CCR_EN                (1UL << 0U)
#define DMA_CCR_TCIE              (1UL << 1U)
#define DMA_CCR_HTIE              (1UL << 2U)
#define DMA_CCR_TEIE              (1UL << 3U)
#define DMA_CCR_CIRC              (1UL << 5U)
#define DMA_CCR_PINC              (1UL << 6U)
#define DMA_CCR_MINC              (1UL << 7U)
#define DMA_CCR_PSIZE_Pos         (8U)
#define DMA_CCR_MSIZE_Pos         (10U)
#define DMA_CCR_MEM2MEM           (1UL << 14U)

#define DMAMUX_CxCR_DMAREQ_ID_Pos (0U)
#define DMAMUX_CxCR_DMAREQ_ID_Msk (0x7FUL << DMAMUX_CxCR_DMAREQ_ID_Pos)

/* ============================================================================
 * IWDG & DBGMCU Bit Definitions (verified vs stm32g474xx.h)
 * ========================================================================== */
#define IWDG_SR_PVU               (1UL << 0U)
#define IWDG_SR_RVU               (1UL << 1U)
#define DBGMCU_APB1FZR1_DBG_IWDG_STOP (1UL << 12U)

/* ============================================================================
 * Cortex-M4 SCB CFSR / HFSR Masks (verified vs core_cm4.h)
 * ========================================================================== */
#define SCB_CFSR_MEMFAULTSR_Msk   (0xFFUL << 0U)
#define SCB_CFSR_BUSFAULTSR_Msk   (0xFFUL << 8U)
#define SCB_CFSR_USGFAULTSR_Msk   (0xFFFFUL << 16U)
#define SCB_CFSR_MMARVALID_Msk    (1UL << 7U)
#define SCB_CFSR_BFARVALID_Msk    (1UL << 15U)
#define SCB_CFSR_UNDEFINSTR_Msk   (1UL << 16U)
#define SCB_CFSR_INVSTATE_Msk     (1UL << 17U)
#define SCB_CFSR_UNALIGNED_Msk    (1UL << 24U)
#define SCB_CFSR_DIVBYZERO_Msk    (1UL << 25U)
#define SCB_HFSR_VECTTBL_Msk      (1UL << 1U)
#define SCB_HFSR_FORCED_Msk       (1UL << 30U)

#ifdef __cplusplus
}
#endif

#endif /* MOCK_STM32G474XX_H */
