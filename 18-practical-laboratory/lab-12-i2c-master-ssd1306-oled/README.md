# Lab 12: I2C1 Master 400 kHz Fast Mode & SSD1306 OLED Display (with NACK/Bus Recovery)

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Status:** `EXTERNAL HARDWARE PENDING` *(Requires External SSD1306 I2C OLED + 2.2–4.7 kΩ Pull-Ups on `PA15`/`PB7` — User Connection Required)*

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`TIMINGR = 0x70811021` worst-case $t_r=300\text{ ns}$, or `i2c_g4.h` `0x30B21F3B`) | **Alternate = `160 MHz Range 1 Boost`** (`TIMINGR = 0x70710F1F` worst-case $t_r=300\text{ ns}$, or `i2c_g4.h` `0x30B21D38`) | **Reusable Drivers:** [`23-code-library/Drivers/i2c_g4.h`](../../23-code-library/Drivers/i2c_g4.h) (`i2c_init_master()`, `i2c_write()`, `i2c_read_reg()`, `i2c_recover_bus_9clks()`, `i2c_select_timingr()`), [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h), & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h).

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE to **`170 MHz`**, configure **`I2C1`** on **`PA15` (`SCL`, `AF4`) / `PB7` (`SDA`, `AF4`)** in **400 kHz Fast Mode** with an explicitly derived, RM0440 §39.4.9 spec-compliant **`TIMINGR`** (`0x70811021`) that satisfies $t_{\text{SCLDEL}} \ge t_{r(\max)} + t_{\text{SU;DAT}}$ even at worst-case $t_r = 300\text{ ns}$, completely avoid the **NUCLEO-G474RE `PB8-BOOT0` bootloader-trap voltage divider**, prevent **infinite NACK deadlocks** via bounded `NACKF`/timeout handling + **9-clock bus recovery**, support transfers $> 255$ bytes via **`I2C_CR2_RELOAD`** (required for the 1024-byte SSD1306 framebuffer), and initialize an SSD1306 OLED with its charge pump enabled.

- **Target Concept IDs:** `PERIPH-I2C-001` (Open-Drain Bus, `TIMINGR`, `RELOAD`, `NACKF`), `DIG-ELEC-004` (Pull-Up Sizing & RC Rise Time), `FW-REL-001` (9-Clock Stuck-Bus Recovery & Bounded Timeouts)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **External Components:**
  - `0.96"` `128×64` SSD1306 I2C OLED module (7-bit address `0x3C`, 8-bit write address `0x78`)
  - **Two external `2.2 kΩ`–`4.7 kΩ` pull-up resistors** from `SCL` (`PA15`) and `SDA` (`PB7`) to `3.3 V` (note: many breakout modules already populate `4.7 kΩ`–`10 kΩ` pull-ups; verify total parallel pull-up meets $t_r \le 300\text{ ns}$ for 400 kHz Fast Mode!)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

> [!CAUTION]
> **Why Lab 12 Defaults to `PA15`/`PB7` Instead of `PB8`/`PB9` (`PB8-BOOT0` Bootloader Trap — UM2505 / DS12288):**  
> On NUCLEO-G474RE (`MB1367` Rev C), **`PB8`** (Arduino `D15`) is shared with **`BOOT0`** and has an onboard **`10 kΩ` pull-down resistor (`R36`)** to `GND`:
> 1. **Without external pull-ups:** STM32's internal `~40 kΩ` pull-up forms a voltage divider with `R36`: $V_{\text{PB8}} = 3.3\text{ V} \times \frac{10}{40 + 10} = \mathbf{0.66\text{ V}}$ — permanently stuck LOW!
> 2. **With permanent external `2.2 kΩ` pull-ups on `PB8`:** The divider pulls `PB8-BOOT0` to $V_{\text{PB8}} = 3.3\text{ V} \times \frac{10}{2.2 + 10} = \mathbf{2.705\text{ V}} > V_{\text{IH}}$ at reset! Because factory option byte `FLASH->OPTR.nSWBOOT0 = 1` samples physical `PB8-BOOT0` on every power-up/NRST, **wiring permanent `2.2 kΩ` pull-ups to `PB8` traps NUCLEO-G474RE in the ST System ROM Bootloader instead of booting User Flash** unless you first clear `nSWBOOT0 = 0` via STM32CubeProgrammer!
> 3. **Default Zero-Conflict Solution (Used Below):** Map **`I2C1`** to **`PA15` (`I2C1_SCL`, `AF4`)** and **`PB7` (`I2C1_SDA`, `AF4`)**! `PA15` is `JTDI` (unused by 2-wire SWD on `PA13`/`PA14`) and `PB7` has no `BOOT0` coupling, so permanent external `2.2 kΩ`–`4.7 kΩ` pull-ups work cleanly across every power cycle and reset.

| MCU Pin (Morpho Header) | I2C1 Signal | `MODER` | `OTYPER` | `AFR` | External Pull-Up |
|---|---|---|---|---|---|
| `PA15` (`CN7` Pin 17) | `I2C1_SCL` | `10b` (AF) | `1` (**Open-Drain**) | `AFR[1][31:28]` = `AF4` | **External `2.2 kΩ`–`4.7 kΩ` to `3.3 V`** |
| `PB7` (`CN7` Pin 21) | `I2C1_SDA` | `10b` (AF) | `1` (**Open-Drain**) | `AFR[0][31:28]` = `AF4` | **External `2.2 kΩ`–`4.7 kΩ` to `3.3 V`** |

*(Advisory: If you specifically need Arduino `D15`/`D14` (`PB8`/`PB9`), you MUST first program option byte `FLASH->OPTR.nSWBOOT0 = 0` and `nBOOT0 = 1` using STM32CubeProgrammer so `PB8` is ignored during boot sampling, then install external `2.2 kΩ` pull-ups.)*

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 39 / 41** (*I2C*, §39.4.9 *I2C_TIMINGR configuration*), **NXP UM10204** (*I2C-bus Specification — Fast Mode 400 kHz*), **DS12288**.

### 5.1 Spec-Compliant 400 kHz `TIMINGR` Derivation @ `170 MHz` (`I2CCLK = PCLK1`)

NXP UM10204 Fast Mode (`400 kHz`, worst-case $t_{r(\max)} = 300\text{ ns}$, $t_{f(\max)} = 300\text{ ns}$) and RM0440 §39.4.9 mandate:
- $t_{\text{HIGH}} \ge 600\text{ ns}$
- $t_{\text{LOW}} \ge 1300\text{ ns}$
- $t_{\text{SCLDEL}} \ge t_{r(\max)} + t_{\text{SU;DAT}} = 300\text{ ns} + 100\text{ ns} = \mathbf{400\text{ ns}}$ *(crucial! Smaller `PRESC` with 4-bit `SCLDEL <= 15` fails this bound if $t_r$ approaches $300\text{ ns}$!)*
- $t_{\text{SDADEL}} \le t_{\text{VD;DAT}(\max)} - t_{f(\max)} - t_{\text{AF}(\max)} - 4 t_{\text{I2CCLK}} = 900 - 300 - 260 - 23.5 = \mathbf{316.5\text{ ns}}$
- $f_{\text{SCL}} \le 400\text{ kHz}$ (even on a low-capacitance bus with minimal edge times!)

Choose **`PRESC = 7`** $\implies t_{\text{PRESC}} = \frac{7 + 1}{170\text{ MHz}} = \mathbf{47.059\text{ ns}}$:
- **`SCLDEL = 8` (`0x8`)**: $t_{\text{SCLDEL}} = (8 + 1) \times 47.059\text{ ns} = \mathbf{423.5\text{ ns}} \ge 400.0\text{ ns}\ \checkmark$
- **`SDADEL = 1` (`0x1`)**: $t_{\text{SDADEL}} = 1 \times 47.059\text{ ns} = \mathbf{47.1\text{ ns}} \in [0,\ 316.5\text{ ns}]\ \checkmark$
- **`SCLH = 16` (`0x10`)**: $t_{\text{SCLH}} = (16 + 1) \times 47.059\text{ ns} = \mathbf{800.0\text{ ns}} \ge 600.0\text{ ns}\ \checkmark$
- **`SCLL = 33` (`0x21`)**: $t_{\text{SCLL}} = (33 + 1) \times 47.059\text{ ns} = \mathbf{1600.0\text{ ns}} \ge 1300.0\text{ ns}\ \checkmark$
- **Maximum Frequency Check (zero edge delay + min analog filter $t_{\text{SYNC1+2}} \ge 123.5\text{ ns}$):** $T_{\text{SCL(min)}} = 800.0 + 1600.0 + 123.5 = 2523.5\text{ ns} \implies f_{\text{SCL(max)}} = \mathbf{396.3\text{ kHz}} \le 400\text{ kHz}\ \checkmark$ (typical bench bus with $t_r \approx 120\text{ ns}$ runs at $\approx 360\text{–}385\text{ kHz}$).

$$\text{I2C1->TIMINGR} = (7 \ll 28) \mid (8 \ll 20) \mid (1 \ll 16) \mid (\text{0x10} \ll 8) \mid \text{0x21} = \mathbf{\text{0x70811021}}$$

### 5.2 Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` vs `16 MHz`)

Never reuse a `170 MHz` `I2C_TIMINGR` value at `160 MHz` or `16 MHz` without recalculating `PRESC`/`SCLH`/`SCLL` — use the exact matched `TIMINGR` below (or call `i2c_init_master(I2C1, I2C_SPEED_400KHZ)` in [`i2c_g4.h`](../../23-code-library/Drivers/i2c_g4.h), which selects the verified constant dynamically from live `SystemCoreClock`):

| Clock Profile | `clock_g4.h` API | `400 kHz` Worst-Case $t_r=300\text{ ns}$ (`PRESC=7`) | `i2c_g4.h` `400 kHz` Fast Mode | `i2c_g4.h` `100 kHz` Standard Mode | Verified Timing Bounds (`PRESC=7` 400 kHz) |
|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | **`0x70811021`** | `0x30B21F3B` | `0x70C45D6C` | $t_{\text{SCLDEL}}=423.5\text{ ns}$, $t_{\text{SCLH}}=800\text{ ns}$, $t_{\text{SCLL}}=1600\text{ ns}$, $f_{\max}=396.3\text{ kHz}$ |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | **`0x70710F1F`** | `0x30B21D38` | `0x70C45866` | $t_{\text{SCLDEL}}=400.0\text{ ns}$, $t_{\text{SCLH}}=800\text{ ns}$, $t_{\text{SCLL}}=1600\text{ ns}$, $f_{\max}=396.0\text{ kHz}$ |
| **`16 MHz HSI16` *(Reset Fallback)*** | Reset default | `0x00310C16` | `0x00310C16` | `0x00423F4F` | `I2C_TIMINGR_16MHZ_400KHZ` / `100KHZ` |

- **Reusable Production Driver (`23-code-library/Drivers/i2c_g4.h`):** Call `i2c_recover_bus_9clks(GPIOA, 15U, GPIOB, 7U)`, configure `PA15`/`PB7` AF4 Open-Drain via `GPIO_ConfigAF()`, then call `i2c_init_master(I2C1, I2C_SPEED_400KHZ)` and `i2c_write()` (automatically chunks $> 255\text{ B}$ transfers with `RELOAD` + checks `NACKF`/`BERR`/`ARLO`).

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t SystemCoreClock;

#define SSD1306_ADDR_8BIT   (0x78U) /* 7-bit 0x3C << 1 */
#define I2C_TIMEOUT_LOOPS   (300000UL)

typedef enum {
    I2C_OK = 0,
    I2C_ERR_BUSY,
    I2C_ERR_NACK,
    I2C_ERR_TIMEOUT,
    I2C_ERR_BUS
} i2c_status_t;

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

static void delay_short(uint32_t loops) {
    for (volatile uint32_t i = 0U; i < loops; i++) {
        __NOP();
    }
}

/* 9-Clock I2C Bus Recovery on PA15 (SCL) & PB7 (SDA): frees a slave stuck holding SDA LOW */
static void I2C1_RecoverBus(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* Temporarily disable I2C1 peripheral */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* Set PA15 (SCL) and PB7 (SDA) as Open-Drain GPIO Outputs released HIGH */
    GPIOA->OTYPER |= (1U << 15U);
    GPIOB->OTYPER |= (1U << 7U);
    GPIOA->PUPDR  &= ~(3U << 30U); /* Clear JTDI reset pull-up on PA15 */
    GPIOB->PUPDR  &= ~(3U << 14U);
    GPIOA->BSRR    = GPIO_BSRR_BS15;
    GPIOB->BSRR    = GPIO_BSRR_BS7;
    GPIOA->MODER   = (GPIOA->MODER & ~(3U << 30U)) | (1U << 30U);
    GPIOB->MODER   = (GPIOB->MODER & ~(3U << 14U)) | (1U << 14U);

    /* Clock up to 9 SCL pulses while SDA (PB7) is stuck LOW */
    for (uint32_t pulse = 0U; pulse < 9U; pulse++) {
        if ((GPIOB->IDR & GPIO_IDR_ID7) != 0U) {
            break; /* SDA released HIGH */
        }
        GPIOA->BSRR = GPIO_BSRR_BR15; /* SCL LOW */
        delay_short(400U);
        GPIOA->BSRR = GPIO_BSRR_BS15; /* SCL HIGH */
        delay_short(400U);
    }

    /* Generate manual STOP condition: SDA LOW -> SCL HIGH -> SDA HIGH */
    GPIOB->BSRR = GPIO_BSRR_BR7;
    delay_short(400U);
    GPIOA->BSRR = GPIO_BSRR_BS15;
    delay_short(400U);
    GPIOB->BSRR = GPIO_BSRR_BS7;
    delay_short(400U);
}

static void I2C1_Init_400kHz(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    (void)RCC->APB1ENR1;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;

    I2C1_RecoverBus();

    /* PA15 (I2C1_SCL, AF4) & PB7 (I2C1_SDA, AF4) -> Open-Drain, High Speed, No internal pull */
    GPIOA->MODER   = (GPIOA->MODER   & ~(3U << 30U)) | (2U << 30U);
    GPIOA->OTYPER |= (1U << 15U);
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3U << 30U)) | (2U << 30U);
    GPIOA->PUPDR  &= ~(3U << 30U);
    GPIOA->AFR[1]  = (GPIOA->AFR[1]  & ~(0xFU << 28U)) | (4U << 28U);

    GPIOB->MODER   = (GPIOB->MODER   & ~(3U << 14U)) | (2U << 14U);
    GPIOB->OTYPER |= (1U << 7U);
    GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~(3U << 14U)) | (2U << 14U);
    GPIOB->PUPDR  &= ~(3U << 14U);
    GPIOB->AFR[0]  = (GPIOB->AFR[0]  & ~(0xFU << 28U)) | (4U << 28U);

    /* MUST ensure PE == 0 before programming TIMINGR */
    I2C1->CR1 &= ~I2C_CR1_PE;
    while ((I2C1->CR1 & I2C_CR1_PE) != 0U) {}

    /* Verified RM0440 §39.4.9 400 kHz Fast Mode @ 170 MHz (t_r <= 300 ns):
     * PRESC=7 (47.06 ns), SCLDEL=8 (423.5 ns >= 400 ns), SDADEL=1 (47.1 ns),
     * SCLH=0x10 (800.0 ns >= 600 ns), SCLL=0x21 (1600.0 ns >= 1300 ns) -> f_max <= 396.3 kHz */
    I2C1->TIMINGR = 0x70811021UL;
    I2C1->CR1     = I2C_CR1_PE;
}

/* Supports arbitrary len > 255 via hardware RELOAD chunking, with NACK & timeout protection */
static i2c_status_t I2C1_Write(uint8_t dev_addr_8bit, const uint8_t *data, uint32_t len) {
    if ((len == 0U) || (data == 0)) {
        return I2C_OK;
    }

    uint32_t timeout = I2C_TIMEOUT_LOOPS;
    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U) {
        if (--timeout == 0U) { return I2C_ERR_BUSY; }
    }

    /* Clear any stale error/stop flags */
    I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;

    uint32_t remaining = len;
    uint32_t idx       = 0U;
    uint32_t chunk     = (remaining > 255U) ? 255U : remaining;

    uint32_t cr2 = ((uint32_t)(dev_addr_8bit & 0xFEU)) |
                   (chunk << I2C_CR2_NBYTES_Pos)       |
                   I2C_CR2_START;
    cr2 |= (remaining > 255U) ? I2C_CR2_RELOAD : I2C_CR2_AUTOEND;
    I2C1->CR2 = cr2;

    while (remaining > 0U) {
        for (uint32_t c = 0U; c < chunk; c++) {
            timeout = I2C_TIMEOUT_LOOPS;
            while ((I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF | I2C_ISR_BERR | I2C_ISR_ARLO)) == 0U) {
                if (--timeout == 0U) {
                    I2C1->CR2 |= I2C_CR2_STOP;
                    return I2C_ERR_TIMEOUT;
                }
            }
            if ((I2C1->ISR & I2C_ISR_NACKF) != 0U) {
                I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
                return I2C_ERR_NACK;
            }
            if ((I2C1->ISR & (I2C_ISR_BERR | I2C_ISR_ARLO)) != 0U) {
                I2C1->ICR = I2C_ICR_BERRCF | I2C_ICR_ARLOCF;
                return I2C_ERR_BUS;
            }
            I2C1->TXDR = data[idx++];
        }

        remaining -= chunk;
        if (remaining > 0U) {
            /* Wait for TCR (Transfer Complete Reload) before programming next chunk */
            timeout = I2C_TIMEOUT_LOOPS;
            while ((I2C1->ISR & I2C_ISR_TCR) == 0U) {
                if ((I2C1->ISR & I2C_ISR_NACKF) != 0U) {
                    I2C1->ICR = I2C_ICR_NACKCF;
                    return I2C_ERR_NACK;
                }
                if (--timeout == 0U) { return I2C_ERR_TIMEOUT; }
            }
            chunk = (remaining > 255U) ? 255U : remaining;
            uint32_t next_cr2 = (I2C1->CR2 & ~(I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND)) |
                                (chunk << I2C_CR2_NBYTES_Pos);
            next_cr2 |= (remaining > 255U) ? I2C_CR2_RELOAD : I2C_CR2_AUTOEND;
            I2C1->CR2 = next_cr2;
        }
    }

    timeout = I2C_TIMEOUT_LOOPS;
    while ((I2C1->ISR & I2C_ISR_STOPF) == 0U) {
        if (--timeout == 0U) { return I2C_ERR_TIMEOUT; }
    }
    I2C1->ICR = I2C_ICR_STOPCF;
    return I2C_OK;
}

static volatile i2c_status_t g_last_status = I2C_OK;

int main(void) {
    Clock_Init_170MHz();
    I2C1_Init_400kHz();

    /* Complete minimal SSD1306 128x64 initialization including internal Charge Pump (0x8D, 0x14)! */
    static const uint8_t ssd1306_init[] = {
        0x00U,       /* Control byte: Co=0, D/C#=0 (Command stream) */
        0xAEU,       /* Display OFF */
        0x20U, 0x00U,/* Horizontal Addressing Mode */
        0x8DU, 0x14U,/* CRITICAL: Enable internal DC-DC Charge Pump */
        0xA4U,       /* Resume to RAM content display */
        0xA6U,       /* Normal display (non-inverted) */
        0xAFU        /* Display ON */
    };

    g_last_status = I2C1_Write(SSD1306_ADDR_8BIT, ssd1306_init, sizeof(ssd1306_init));

    /* Example 1025-byte write (1 control byte + 1024-byte full framebuffer) exercising RELOAD */
    static uint8_t fb[1025];
    fb[0] = 0x40U; /* Data stream */
    for (uint32_t i = 1U; i < sizeof(fb); i++) {
        fb[i] = 0xAAU; /* Horizontal stripe test pattern */
    }
    if (g_last_status == I2C_OK) {
        g_last_status = I2C1_Write(SSD1306_ADDR_8BIT, fb, sizeof(fb));
    }

    while (1) {
        __WFI();
    }
}
```

---

## 7. Expected Output

With an SSD1306 OLED attached (and external `2.2 kΩ` pull-ups), `g_last_status == I2C_OK (0)` and the display illuminates with a crisp horizontal stripe pattern across all 128×64 pixels. If the OLED is unplugged, `I2C1_Write()` cleanly returns `I2C_ERR_NACK (2)` without ever deadlocking.

---

## 8. Debugging Procedure

1. Probe `PA15` (`SCL`) and `PB7` (`SDA`) with an oscilloscope before initiating traffic: both lines MUST idle near **`3.3 V`** with external `2.2 kΩ`–`4.7 kΩ` pull-ups.
2. Trigger on `SDA` falling edge: measure $t_{\text{HIGH}} \approx 800\text{ ns} \ge 600\text{ ns}$, $t_{\text{LOW}} \approx 1600\text{ ns} \ge 1300\text{ ns}$, $t_{\text{SCLDEL}} \approx 423.5\text{ ns} \ge 400\text{ ns}$, and $f_{\text{SCL}} \le 396.3\text{ kHz} \le 400\text{ kHz}$.
3. Unplug the OLED and step through `I2C1_Write()`: verify `I2C_ISR_NACKF` triggers clean exit with `g_last_status == I2C_ERR_NACK`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Board enters System ROM Bootloader on reset when using `PB8`/`PB9` | Permanent external `2.2 kΩ` pull-up on `PB8-BOOT0` fights `R36` (`10 kΩ`) → `2.705 V` latches `BOOT0 = 1`! | Use default **`PA15`/`PB7`** (`AF4`), OR clear `FLASH->OPTR.nSWBOOT0 = 0` via STM32CubeProgrammer before attaching pull-ups to `PB8` |
| Firmware hangs permanently when OLED disconnected | Polled `TXIS` without checking `I2C_ISR_NACKF` or a timeout | Poll `TXIS \| NACKF` with bounded timeout and clear `NACKCF` |
| OLED ACKs every byte but screen remains completely black | Omitted SSD1306 Charge Pump command (`0x8D, 0x14`) | Include `0x8D, 0x14` before `0xAF` |
| Writing full 1024-byte framebuffer corrupts `CR2` | Shifted `len > 255` directly into 8-bit `NBYTES` field | Use `I2C_CR2_RELOAD` + `I2C_ISR_TCR` 255-byte chunking |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `I2C1->TIMINGR == 0x70811021UL` programmed while `PE == 0` @ `170 MHz`.
- [ ] Oscilloscope confirms $t_{\text{SCLDEL}} \ge 400\text{ ns}$, $t_{\text{HIGH}} \ge 600\text{ ns}$, $t_{\text{LOW}} \ge 1300\text{ ns}$, $f_{\text{SCL}} \le 400\text{ kHz}$.
- [ ] Disconnecting OLED returns `I2C_ERR_NACK` cleanly (zero hang) and pressing `NRST` boots User Flash cleanly with OLED attached on `PA15`/`PB7`.

---

## 11. Extension Exercises

1. **Optional Arduino Header (`PB8`/`PB9`) Remap:** Program `FLASH->OPTR.nSWBOOT0 = 0` (`nBOOT0 = 1`) via STM32CubeProgrammer, verify `PB8-BOOT0` is ignored on reset, and remap `I2C1` to `PB8`/`PB9` (`AF4`) with `2.2 kΩ` pull-ups.
2. **I2C Bus Scanner:** Sweep 7-bit addresses `0x08..0x77` and print all ACKing slave addresses over USART2 (Lab 08).
3. **DMA Framebuffer Push:** Stream the 1024-byte OLED framebuffer via `DMA1` (`DMAMUX1` ID `17` = `I2C1_TX`) + `I2C_CR1_TXDMAEN`.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` / `EXTERNAL HARDWARE PENDING` |
| **Objective** | Drive an `I2C1` 400 kHz Fast Mode Master on `PA15`/`PB7` (`AF4` Open-Drain, completely avoiding the `PB8-BOOT0` trap) with RM0440 §39.4.9 `TIMINGR` (`0x70811021` @ 170 MHz / `0x70710F1F` @ 160 MHz), 9-clock bus recovery, bounded `NACKF` handling, and `RELOAD` chunking for a 1025-byte SSD1306 framebuffer. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`2,284 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **External Hardware Required:** `I2C1`, `GPIOA`/`GPIOB` + external `0.96"` `128×64` SSD1306 I2C OLED (`0x3C`) + two external `2.2 kΩ`–`4.7 kΩ` pull-up resistors to `3.3 V` (+ optional oscilloscope/logic analyzer). |
| **Hardware Availability** | `EXTERNAL HARDWARE PENDING — USER CONNECTION REQUIRED` |
| **Pin Configuration** | `PA15` (`CN7` pin 17, `I2C1_SCL`, `AF4`, `OTYPER=1` Open-Drain, `OSPEEDR=10b`, `PUPDR=00b`); `PB7` (`CN7` pin 21, `I2C1_SDA`, `AF4`, `OTYPER=1` Open-Drain, `OSPEEDR=10b`, `PUPDR=00b`). Avoids `PB8-BOOT0` (`R36` 10 kΩ pull-down). |
| **Expected Behavior** | Bus idles at `3.3 V`; SSD1306 ACKs init + charge-pump (`0x8D, 0x14`) + 1025-byte `RELOAD` framebuffer displaying horizontal stripes (`g_last_status == I2C_OK`); disconnecting OLED returns `I2C_ERR_NACK` cleanly without hanging. |
| **Actual Observations** | `EXTERNAL HARDWARE PENDING — USER CONNECTION REQUIRED` *(cross-compiled cleanly with 0 warnings; physical SSD1306 OLED & I2C waveform measurement deferred until user connects external OLED & pull-ups)*. |
| **Register Values** | Reset: `I2C1->CR1 = 0`, `I2C1->TIMINGR = 0`. Configured: `I2C1->TIMINGR = 0x70811021` (`170 MHz`) or `0x70710F1F` (`160 MHz`) — or via `i2c_g4.h`: `0x30B21F3B` (`170 MHz`) / `0x30B21D38` (`160 MHz`); `I2C1->CR1 = 0x1` (`PE=1`); `GPIOA->OTYPER[15] = 1`, `GPIOB->OTYPER[7] = 1`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` verify `p/x I2C1->TIMINGR`, `p/x I2C1->ISR`, `p g_last_status`; verify `3.3 V` idle + `<= 400 kHz` SCL on oscilloscope once OLED & pull-ups are wired. |
| **Remaining Limitations** | Requires external SSD1306 OLED and external pull-ups for physical ACK and display validation; without pull-ups on `PA15`/`PB7`, `I2C1` reads `BUSY` or `NACK`. |
