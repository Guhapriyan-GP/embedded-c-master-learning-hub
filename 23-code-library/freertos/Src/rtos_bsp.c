/**
 * @file    rtos_bsp.c
 * @brief   Self-contained NUCLEO-G474RE Board Support Package for FreeRTOS
 *          verification firmware. All register names and bit positions are
 *          verified against CMSIS stm32g474xx.h and RM0440.
 */

#include "rtos_bsp.h"
#include "stm32g474xx.h"
#include <stdarg.h>
#include <stdio.h>

#define BSP_READY_TIMEOUT   (1000000UL)
#define BSP_PRINTF_BUF_SIZE (192U)

static void bsp_stabilize_1us_at_85mhz(void)
{
    for (uint32_t i = 0U; i < 120U; ++i) {
        __asm__ volatile("nop");
    }
}

bool BSP_Clock_Init_170MHz(void)
{
    uint32_t timeout;

    /* 1. Enable PWR peripheral clock and synchronize bus */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;

    /* 2. Select Voltage Scaling Range 1 (VOS = 0b01) and wait for VOSF == 0 */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    timeout = BSP_READY_TIMEOUT;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 3. Enable Range 1 Boost Mode (R1MODE = 0 in PWR->CR5) for SYSCLK > 150 MHz */
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* 4. Configure Flash to 4 Wait States + Prefetch + Instruction/Data Caches */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS |
                 FLASH_ACR_PRFTEN |
                 FLASH_ACR_ICEN |
                 FLASH_ACR_DCEN;
    timeout = BSP_READY_TIMEOUT;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 5. Ensure HSI16 is enabled and ready */
    RCC->CR |= RCC_CR_HSION;
    timeout = BSP_READY_TIMEOUT;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 6. If PLL is currently SYSCLK, switch back to HSI16 before reconfiguring */
    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL) {
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
        timeout = BSP_READY_TIMEOUT;
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
            if (--timeout == 0U) {
                return false;
            }
        }
    }

    /* 7. Disable Main PLL and wait for PLLRDY == 0 */
    RCC->CR &= ~RCC_CR_PLLON;
    timeout = BSP_READY_TIMEOUT;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 8. Configure Main PLL:
     *    HSI16 (16 MHz) / M=4 -> 4 MHz -> * N=85 -> 340 MHz VCO
     *    R=/2 -> 170 MHz SYSCLK, Q=/8 -> 42.5 MHz, P=/6 -> 56.67 MHz */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI |
                   ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos) |
                   (85U << RCC_PLLCFGR_PLLN_Pos) |
                   (0U << RCC_PLLCFGR_PLLR_Pos) |
                   RCC_PLLCFGR_PLLREN |
                   (3U << RCC_PLLCFGR_PLLQ_Pos) |
                   RCC_PLLCFGR_PLLQEN |
                   (6U << RCC_PLLCFGR_PLLPDIV_Pos) |
                   RCC_PLLCFGR_PLLPEN;

    /* 9. Enable Main PLL and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    timeout = BSP_READY_TIMEOUT;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* 10. RM0440 §6.1.5 mandatory two-step switch for > 150 MHz:
     *     Intermediate HPRE = /2 (85 MHz), switch SYSCLK -> PLL, wait >= 1 us, then HPRE = /1 */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2 |
                RCC_CFGR_PPRE1_DIV1 |
                RCC_CFGR_PPRE2_DIV1;

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    timeout = BSP_READY_TIMEOUT;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
        if (--timeout == 0U) {
            return false;
        }
    }

    bsp_stabilize_1us_at_85mhz();
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE) | RCC_CFGR_HPRE_DIV1;

    SystemCoreClockUpdate();
    return true;
}

uint32_t BSP_GetAndClearResetFlags(void)
{
    uint32_t csr = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;
    /* Per RM0440 §7.4.29, clear RMVF back to 0 so future reset flags are not masked */
    (void)RCC->CSR;
    RCC->CSR &= ~RCC_CSR_RMVF;
    return csr;
}

void BSP_LED_Init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* PA5 -> General Purpose Output Push-Pull, Low Speed, No Pull */
    GPIOA->MODER   = (GPIOA->MODER & ~(3UL << 10U)) | (1UL << 10U);
    GPIOA->OTYPER &= ~(1UL << 5U);
    GPIOA->OSPEEDR &= ~(3UL << 10U);
    GPIOA->PUPDR   &= ~(3UL << 10U);
    GPIOA->BSRR     = GPIO_BSRR_BR5;
}

void BSP_LED_On(void)
{
    GPIOA->BSRR = GPIO_BSRR_BS5;
}

void BSP_LED_Off(void)
{
    GPIOA->BSRR = GPIO_BSRR_BR5;
}

void BSP_LED_Toggle(void)
{
    if ((GPIOA->ODR & GPIO_ODR_OD5) != 0U) {
        GPIOA->BSRR = GPIO_BSRR_BR5;
    } else {
        GPIOA->BSRR = GPIO_BSRR_BS5;
    }
}

void BSP_Button_Init(uint32_t nvic_priority)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    /* PC13 -> Input, No internal pull (NUCLEO-G474RE B1 has external pull-down, Active-HIGH) */
    GPIOC->MODER &= ~(3UL << 26U);
    GPIOC->PUPDR &= ~(3UL << 26U);

    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~SYSCFG_EXTICR4_EXTI13_Msk) |
                        SYSCFG_EXTICR4_EXTI13_PC;
    EXTI->RTSR1 |=  EXTI_RTSR1_RT13;
    EXTI->FTSR1 &= ~EXTI_FTSR1_FT13;
    EXTI->PR1    =  EXTI_PR1_PIF13;
    EXTI->IMR1  |=  EXTI_IMR1_IM13;

    NVIC_SetPriority(EXTI15_10_IRQn, nvic_priority);
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

bool BSP_Button_Read(void)
{
    return ((GPIOC->IDR & GPIO_IDR_ID13) != 0U);
}

void BSP_USART2_Init_115200(void)
{
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    (void)RCC->APB1ENR1;

    /* PA2 (USART2_TX) & PA3 (USART2_RX) -> Alternate Function AF7 */
    GPIOA->MODER   = (GPIOA->MODER   & ~((3UL << 4U) | (3UL << 6U))) | ((2UL << 4U) | (2UL << 6U));
    GPIOA->OTYPER &= ~((1UL << 2U) | (1UL << 3U));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3UL << 4U) | (3UL << 6U))) | ((2UL << 4U) | (2UL << 6U));
    GPIOA->PUPDR   = (GPIOA->PUPDR   & ~((3UL << 4U) | (3UL << 6U))) | ((1UL << 4U) | (1UL << 6U));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~((0xFUL << 8U) | (0xFUL << 12U))) |
                     ((7UL << 8U) | (7UL << 12U));

    /* Disable USART2 before configuration */
    USART2->CR1 = 0U;
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;

    /* 170 MHz / 115200 = 1475.69 -> BRR = 1476 (error = -0.02%) */
    USART2->BRR = (SystemCoreClock + 57600UL) / 115200UL;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void BSP_USART2_WriteChar(char c)
{
    while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0U) {
        /* wait for TX data register empty */
    }
    USART2->TDR = (uint8_t)c;
}

void BSP_USART2_WriteStr(const char *str)
{
    if (str == NULL) {
        return;
    }
    while (*str != '\0') {
        BSP_USART2_WriteChar(*str++);
    }
}

void BSP_USART2_Printf(const char *fmt, ...)
{
    char buf[BSP_PRINTF_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0) {
        size_t count = ((size_t)len < (sizeof(buf) - 1U)) ? (size_t)len : (sizeof(buf) - 1U);
        for (size_t i = 0U; i < count; ++i) {
            BSP_USART2_WriteChar(buf[i]);
        }
    }
}

void BSP_TIM2_Init_10Hz(uint32_t nvic_priority)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    (void)RCC->APB1ENR1;

    /* Freeze TIM2 counter while CPU is halted in debugger */
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_TIM2_STOP;

    TIM2->CR1  = 0U;
    /* PCLK1 = 170 MHz -> PSC = 16999 => 10 kHz tick; ARR = 999 => 10 Hz (100 ms) */
    TIM2->PSC  = 16999U;
    TIM2->ARR  = 999U;
    TIM2->CNT  = 0U;
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = 0U;
    TIM2->DIER = TIM_DIER_UIE;

    NVIC_SetPriority(TIM2_IRQn, nvic_priority);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void BSP_TIM2_Start(void)
{
    TIM2->SR   = 0U;
    TIM2->CR1 |= TIM_CR1_CEN;
}

bool BSP_IWDG_Init_3000ms(void)
{
    uint32_t timeout;

    /* Freeze IWDG during debugger halt so stepping in GDB does not reset MCU */
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;

    /* Ensure LSI (~32 kHz) is running */
    RCC->CSR |= RCC_CSR_LSION;
    timeout = BSP_READY_TIMEOUT;
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /* RM0440 §35.3.2 exact sequence:
     * 1. Enable IWDG (0xCCCC), 2. Unlock (0x5555), 3. Set PR & RLR,
     * 4. Wait SR == 0 (PVU/RVU clear), 5. Reload (0xAAAA).
     * PR = 0x03 -> /32 => 32 kHz / 32 = 1 kHz; RLR = 3000 => ~3.0 s timeout. */
    IWDG->KR  = 0x0000CCCCUL;
    IWDG->KR  = 0x00005555UL;
    IWDG->PR  = 0x00000003UL;
    IWDG->RLR = 3000UL;

    timeout = BSP_READY_TIMEOUT;
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    IWDG->KR = 0x0000AAAAUL;
    return true;
}

void BSP_IWDG_Refresh(void)
{
    IWDG->KR = 0x0000AAAAUL;
}

void BSP_DebugBreakIfAttached(void)
{
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0UL) {
        __BKPT(0);
    }
}
