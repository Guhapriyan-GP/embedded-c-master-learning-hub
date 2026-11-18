# Lab 08: USART2 Bare-Metal Driver, SPSC Ring Buffer RX ISR & Retargeted `printf`

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE to **`170 MHz`**, build a production-grade bare-metal **`USART2`** driver (`115,200 baud`, `8N1`) with **bounded TX timeouts**, a **non-blocking Single-Producer Single-Consumer (SPSC) lock-free ring buffer RX ISR** (`USART2_IRQHandler`) with overrun/framing error recovery, retargeted `printf()` via `_write()`, and a **3-way comparison** (Direct Register vs STM32Cube LL vs ST HAL).

- **Target Concept IDs:** `PERIPH-UART-001` (USART Framing & BRR Math), `FW-PAT-001` (SPSC Lock-Free Ring Buffer), `EMB-BAR-001` (`__DMB()`/`__DSB()` Ordering), `TOOL-NEWLIB-001` (`_write` Syscall Retargeting)
- **Scope Layer:** `[STM32G474RE Exact]` / `[General C]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **Host Interface:** USB cable carrying the onboard ST-LINK/V3E Virtual COM Port (VCP) wired directly to `PA2` (`USART2_TX`) and `PA3` (`USART2_RX`) per **UM2505**

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+) with `--specs=nano.specs --specs=nosys.specs` (add `-u _printf_float` only if printing floats)
- Serial terminal on host PC (`PuTTY`, `Tera Term`, `minicom`, or VS Code Serial Monitor) configured for **`115200 baud, 8-N-1, No Flow Control`**

---

## 4. Wiring & Pin Configuration

| MCU Pin | ST-LINK VCP Net | `MODER` | `AFR[0]` | `OSPEEDR` | `PUPDR` | Notes |
|---|---|---|---|---|---|---|
| `PA2` | `USART2_TX` (to Host RX) | `10b` (AF) | `AF7` (`[11:8]`) | `10b` (High) | `00b` (No Pull) | Driven push-pull by USART2 |
| `PA3` | `USART2_RX` (from Host TX) | `10b` (AF) | `AF7` (`[15:12]`) | `10b` (High) | `01b` (Pull-Up) | Pull-up prevents floating break noise if cable disconnects |

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 37 (Rev 7) / Chapter 40 (Rev 9)** (*USART*), **UM2505 §6.8** (*Virtual COM port*).

### 5.1 Baud Rate Divisor (`BRR`) @ 170 MHz (`OVER8 = 0`, 16× Oversampling)

$$\text{USARTDIV} = \frac{f_{\text{PCLK1}}}{\text{Baud}} = \frac{170\,000\,000}{115\,200} = 1475.694\dots \approx 1476 \quad (\text{0x05C4})$$

With integer rounding:
$$\text{BRR} = \left\lfloor \frac{170\,000\,000 + \frac{115\,200}{2}}{115\,200} \right\rfloor = 1476$$

Actual baud rate: $\frac{170\,000\,000}{1476} = 115\,176.15\text{ baud}$ $\implies$ Error = **`-0.021%`** (well inside the $\pm 2\%$ UART sampling margin!).

| Register | Fields Used | Value | Purpose |
|---|---|---|---|
| `USART2->BRR` | `[15:0]` | `1476U` (`0x05C4`) | Exact `115200` baud @ `PCLK1 = 170 MHz` |
| `USART2->CR1` | `UE`, `TE`, `RE`, `RXNEIE` | Enabled | Enables TX, RX, and RX-Not-Empty interrupt |
| `USART2->ISR` | `TXE`, `TC`, `RXNE`, `ORE`, `FE`, `NE`, `PE` | Status | Polled (with bounded timeout) on TX; handled in ISR on RX |
| `USART2->ICR` | `ORECF`, `FECF`, `NECF`, `PECF` | Write-1-to-Clear | Clears error flags in ISR so RX never stalls on overrun |

---

## 6. Code Implementation

### 6.1 Approach A — Direct Register Access with SPSC Ring Buffer & Bounded TX Timeout

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

extern uint32_t SystemCoreClock;

#define UART_RX_BUF_SIZE   (128U) /* MUST be power of 2 */
#define UART_RX_BUF_MASK   (UART_RX_BUF_SIZE - 1U)
#define UART_TX_TIMEOUT    (200000UL)

typedef struct {
    volatile uint8_t  buf[UART_RX_BUF_SIZE];
    volatile uint32_t head; /* Written ONLY by USART2_IRQHandler (Producer) */
    volatile uint32_t tail; /* Written ONLY by main() context    (Consumer) */
    volatile uint32_t rx_overruns;
    volatile uint32_t rx_hw_errors;
} uart_spsc_rb_t;

static uart_spsc_rb_t g_rx_rb = {0};

static void Clock_Init_170MHz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    volatile uint32_t tmpreg = RCC->APB1ENR1;
    (void)tmpreg;

    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) {}

    PWR->CR5 &= ~PWR_CR5_R1MODE;

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) |
                 FLASH_ACR_LATENCY_4WS             |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) {}

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {}

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI              |
                   ((4U - 1U) << RCC_PLLCFGR_PLLM_Pos) |
                   (85U       << RCC_PLLCFGR_PLLN_Pos) |
                   (0U        << RCC_PLLCFGR_PLLR_Pos) |
                   RCC_PLLCFGR_PLLREN;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {}

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    for (volatile uint32_t i = 0U; i < 120U; i++) {
        __NOP();
    }

    RCC->CFGR &= ~RCC_CFGR_HPRE;
    SystemCoreClock = 170000000UL;
}

void USART2_IRQHandler(void) {
    uint32_t isr = USART2->ISR;

    /* 1. Recover cleanly from hardware RX errors (ORE/FE/NE/PE) */
    if ((isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) != 0U) {
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
        g_rx_rb.rx_hw_errors++;
    }

    /* 2. Drain RX data register into lock-free SPSC ring buffer */
    if ((isr & USART_ISR_RXNE_RXFNE) != 0U) {
        uint8_t byte = (uint8_t)(USART2->RDR & 0xFFU);
        uint32_t next_head = (g_rx_rb.head + 1U) & UART_RX_BUF_MASK;
        if (next_head != g_rx_rb.tail) {
            g_rx_rb.buf[g_rx_rb.head] = byte;
            __DMB(); /* Ensure data write completes before publishing updated head */
            g_rx_rb.head = next_head;
        } else {
            g_rx_rb.rx_overruns++; /* Ring buffer full: drop newest byte safely */
        }
    }

    __DSB();
}

static bool USART2_RxPop(uint8_t *out_byte) {
    uint32_t tail = g_rx_rb.tail;
    if (tail == g_rx_rb.head) {
        return false; /* Empty */
    }
    __DMB(); /* Ensure head read completes before reading buffer slot */
    *out_byte = g_rx_rb.buf[tail];
    g_rx_rb.tail = (tail + 1U) & UART_RX_BUF_MASK;
    return true;
}

static bool USART2_TxByte(uint8_t byte) {
    uint32_t timeout = UART_TX_TIMEOUT;
    while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0U) {
        if (--timeout == 0U) {
            return false; /* Bounded timeout prevents infinite lockup */
        }
    }
    USART2->TDR = byte;
    return true;
}

static void USART2_Init(uint32_t baud) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* PA2 (TX), PA3 (RX) -> AF7, High Speed, Pull-Up on RX (PA3) */
    GPIOA->MODER   = (GPIOA->MODER   & ~((3U << 4U) | (3U << 6U))) | ((2U << 4U) | (2U << 6U));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3U << 4U) | (3U << 6U))) | ((2U << 4U) | (2U << 6U));
    GPIOA->PUPDR   = (GPIOA->PUPDR   & ~((3U << 4U) | (3U << 6U))) | (1U << 6U);
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~((0xFU << 8U) | (0xFU << 12U))) | ((7U << 8U) | (7U << 12U));

    /* Disable USART before programming BRR */
    USART2->CR1 = 0U;
    USART2->BRR = (170000000UL + (baud / 2UL)) / baud; /* 1476 @ 115200 baud */
    USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;

    /* Enable TX, RX, RXNE interrupt, and USART peripheral */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE | USART_CR1_UE;

    NVIC_SetPriorityGrouping(0U);
    NVIC_SetPriority(USART2_IRQn, 5U);
    NVIC_ClearPendingIRQ(USART2_IRQn);
    NVIC_EnableIRQ(USART2_IRQn);
}

/* Newlib syscall hook for printf() (link with --specs=nano.specs --specs=nosys.specs) */
int _write(int file, char *ptr, int len) {
    (void)file;
    int sent = 0;
    for (int i = 0; i < len; i++) {
        if (!USART2_TxByte((uint8_t)ptr[i])) {
            break;
        }
        sent++;
    }
    return sent;
}

int main(void) {
    /* CRITICAL: 170 MHz clock MUST be active before computing BRR for 170 MHz! */
    Clock_Init_170MHz();
    USART2_Init(115200UL);

    printf("[STM32G474RE] USART2 Ready @ 115200 8N1 (170 MHz PCLK1). Type to echo:\r\n");

    uint32_t counter = 0U;
    while (1) {
        /* Non-blocking RX drain: echo any bytes typed by host user */
        uint8_t ch = 0U;
        while (USART2_RxPop(&ch)) {
            printf("RX Echo: '%c' (0x%02X)\r\n", (ch >= 32U && ch < 127U) ? (char)ch : '.', ch);
        }

        if ((counter % 5000000UL) == 0UL) {
            printf("[Heartbeat #%lu] rx_ovr=%lu rx_err=%lu\r\n",
                   (unsigned long)(counter / 5000000UL),
                   (unsigned long)g_rx_rb.rx_overruns,
                   (unsigned long)g_rx_rb.rx_hw_errors);
        }
        counter++;
    }
}
```

### 6.2 Approach B — STM32Cube LL Comparison

```c
/* stm32g4xx_ll_usart.h: thin zero-overhead wrappers over BRR/CR1/ISR/TDR */
LL_USART_Disable(USART2);
LL_USART_SetBaudRate(USART2, 170000000UL, LL_USART_PRESCALER_DIV1,
                     LL_USART_OVERSAMPLING_16, 115200UL);
LL_USART_ConfigCharacter(USART2, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);
LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX);
LL_USART_EnableIT_RXNE_RXFNE(USART2);
LL_USART_Enable(USART2);
```

### 6.3 Approach C — STM32Cube HAL Comparison

```c
/* stm32g4xx_hal_uart.h: stateful handle + DMA/IT state machine + callbacks */
UART_HandleTypeDef huart2 = {
    .Instance = USART2,
    .Init = { .BaudRate = 115200U, .WordLength = UART_WORDLENGTH_8B,
              .StopBits = UART_STOPBITS_1, .Parity = UART_PARITY_NONE,
              .Mode = UART_MODE_TX_RX, .HwFlowCtl = UART_HWCONTROL_NONE }
};
HAL_UART_Init(&huart2);
HAL_UART_Transmit(&huart2, (uint8_t *)"Hello\r\n", 7U, 100U); /* Built-in 100 ms timeout */
```

### 6.4 3-Way Comparison Summary

- **Direct Register + Ring Buffer:** Ideal for interrupt-driven telemetry, CLI shells, and zero-heap safety firmware where every byte must be received without missing characters.
- **STM32Cube LL:** Same performance as Direct Register, with automated `BRR` prescaler math and readable error-flag macros.
- **STM32Cube HAL:** Convenient blocking `HAL_UART_Transmit(..., timeout)` and `HAL_UARTEx_ReceiveToIdle_DMA()` for rapid application prototyping, at the cost of ~2–4 KB extra Flash and handle-locking rules.

---

## 7. Expected Output

At `115200 8N1` on the ST-LINK Virtual COM port:
```text
[STM32G474RE] USART2 Ready @ 115200 8N1 (170 MHz PCLK1). Type to echo:
[Heartbeat #0] rx_ovr=0 rx_err=0
RX Echo: 'A' (0x41)
[Heartbeat #1] rx_ovr=0 rx_err=0
```

---

## 8. Debugging Procedure

1. Inspect `USART2->BRR` in GDB — must read `0x05C4` (`1476`).
2. Confirm `RCC->CFGR & RCC_CFGR_SWS == RCC_CFGR_SWS_PLL` (`170 MHz`).
3. Send a burst of 64 characters from the host terminal and inspect `g_rx_rb.head`, `g_rx_rb.tail`, `g_rx_rb.rx_overruns == 0`, and `g_rx_rb.rx_hw_errors == 0`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Terminal shows total garbage characters (`~10,840 baud`) | Forgot `Clock_Init_170MHz()`; `BRR = 1476` @ `16 MHz` gives `10,840 baud`! | Call `Clock_Init_170MHz()` before `USART2_Init()` |
| Linker error `undefined reference to _sbrk` / `_write` | Missing `--specs=nosys.specs` or `_write()` stub | Add `--specs=nano.specs --specs=nosys.specs` to `LDFLAGS` |
| RX stops working after fast burst | Overrun flag `ORE` set and never cleared in `USART2->ICR` | Clear `USART_ICR_ORECF` inside `USART2_IRQHandler` |
| `printf` output never appears until buffer fills | Newlib `stdout` is line-buffered | Terminate messages with `\r\n` or call `setvbuf(stdout, NULL, _IONBF, 0);` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `USART2->BRR == 1476U` at `PCLK1 = 170 MHz` (`-0.021%` error).
- [ ] TX loop terminates safely if `TXE` fails (bounded timeout).
- [ ] Typing characters in serial terminal triggers `USART2_IRQHandler` and echoes with `rx_overruns == 0` and `rx_hw_errors == 0`.

---

## 11. Extension Exercises

1. **Non-Blocking TX Ring Buffer (`TXEIE`):** Add a second SPSC ring buffer for transmission so `_write()` pushes bytes into SRAM and returns immediately while `USART2_IRQHandler` drains them via `USART_CR1_TXEIE_TXFNFIE`.
2. **DMA Circular RX + `IDLE` Line Detection:** Connect `DMA1_Channel2` via DMAMUX ID **`26`** (`USART2_RX`) and enable `USART_CR1_IDLEIE` to receive variable-length binary protocol packets with near-zero CPU load.
3. **Zero-Allocation CLI Parser:** Accumulate characters until `\r` or `\n` and dispatch commands (`LED ON`, `LED OFF`, `STAT`).
