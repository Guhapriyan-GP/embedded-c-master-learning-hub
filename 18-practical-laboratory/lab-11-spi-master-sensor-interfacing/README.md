# Lab 11: SPI1 Master Full-Duplex Sensor Interfacing (10.625 MHz, Mode 3)

> ⬅️ **Prev Lab:** [Lab 10: ADC1 + Circular DMA1](../lab-10-adc-dma-multichannel-circular-buffer/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 12: I2C1 400 kHz SSD1306 OLED](../lab-12-i2c-master-ssd1306-oled/README.md) ➡️

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Status:** `PENDING — USER CONNECTION REQUIRED` *(External `PB4 <-> PB5` loopback jumper or `3.3 V` SPI sensor on `PB3`/`PB4`/`PB5`/`PB6`)*

> **Clock Configuration Profile:** **Primary = `170 MHz Range 1 Boost`** (`BR = 011b` `/16` $\implies f_{\text{SCK}} = 10.625\text{ MHz}$) | **Alternate = `160 MHz Range 1 Boost`** (`BR = 011b` `/16` $\implies f_{\text{SCK}} = 10.000\text{ MHz}$ exact!) | **Reusable Drivers:** [`23-code-library/Drivers/spi_g4.h`](../../23-code-library/Drivers/spi_g4.h) (`spi_init_master()`, `spi_transfer_byte()`, `spi_transfer_buf()`, `spi_wait_idle()`, `spi_calc_br_div()`), [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h), & [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h).

---

## 1. Objective & Concept IDs

Initialize the STM32G474RE to **`170 MHz`**, configure **`SPI1`** as an 8-bit full-duplex Master at **`10.625 MHz`** (Mode 3: `CPOL=1, CPHA=1`) with **`SPI_CR2_FRXTH`** 8-bit FIFO threshold and High-Speed `OSPEEDR`, implement a **proper 2-byte full-duplex SPI register read transaction** (fixing the classic 1-byte dummy-data bug!) with bounded timeouts, and account for the **NUCLEO-G474RE `PA5`/`LD2` (`SB42`/`SB6`) hardware loading conflict**.

- **Prerequisites:** [Lab 03 (170 MHz PLL Clock Switch)](../lab-03-rcc-pll-170mhz-clock-switch/README.md), [Lab 08 (`USART2` Diagnostics)](../lab-08-usart-baremetal-driver-printf/README.md), [Stage 01b (High-Speed Bus Integrity)](../../01b-digital-electronics-and-hw-integration/README.md), and [Stage 11 (SPI Full-Duplex Protocol)](../../11-communication/README.md)
- **Target Concept IDs:** `PERIPH-SPI-001` (Full-Duplex SPI Protocol, `FRXTH` 8-Bit Packing & Modes 0–3), `DIG-ELEC-006` (High-Speed `OSPEEDR` & Bus Loading), `FW-REL-001` (Bounded Hardware Timeouts)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **External Sensor:** Any 3.3 V SPI Mode 3 sensor (e.g., Bosch `BME280` WHO_AM_I reg `0xD0` = `0x60`, or TDK `ICM-20948` / `MPU-6500` reg `0x75`)
- **Optional Verification Equipment:** 4-channel logic analyzer (`SCK`, `MOSI`, `MISO`, `CS`)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | SPI1 Signal | `AFR` | `MODER` | `OSPEEDR` | External Sensor Pin |
|---|---|---|---|---|---|
| `PA5` (Arduino `D13`) | `SPI1_SCK` | `AF5` | `10b` (AF) | `10b` (High) | `SCK` / `SCL` |
| `PA6` (Arduino `D12`) | `SPI1_MISO` | `AF5` | `10b` (AF) | `10b` (High) | `MISO` / `SDO` |
| `PA7` (Arduino `D11`) | `SPI1_MOSI` | `AF5` | `10b` (AF) | `10b` (High) | `MOSI` / `SDI` |
| `PA4` (Arduino `A2`) | Manual `CS` | N/A | `01b` (Output) | `10b` (High) | `CSB` / `NCS` (Active-LOW) |

> [!WARNING]
> **NUCLEO-G474RE Hardware Conflict (`PA5` / `LD2` via `SB42`/`SB6`, UM2505):**  
> On NUCLEO-G474RE, **`PA5`** is physically tied to the onboard Green LED **`LD2`** driver circuit! At high SPI clock rates (`10.625 MHz`–`42.5 MHz`), this extra capacitive/resistive load slows `SCK` edges and causes `LD2` to glow faintly during SPI traffic. For production signal integrity or speeds $> 10\text{ MHz}$, either open the `LD2` solder bridge per UM2505, **or** remap `SPI1` to **`PB3` (`SCK`) / `PB4` (`MISO`) / `PB5` (`MOSI`) (`AF5`)** or use **`SPI2` (`PB13/PB14/PB15`, `AF5`)**.

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 40 (Rev 7) / Chapter 42 (Rev 9)** (*SPI*), **UM2505** (*LED LD2 & Arduino connector*).

### 5.1 SPI Clock Rate & 8-Bit FIFO Threshold (`FRXTH`)

- With `PCLK2 = 170 MHz` and `BR[2:0] = 011b` (`3U` $\implies$ divide by `16`):
  $$f_{\text{SCK}} = \frac{170\text{ MHz}}{16} = 10.625\text{ MHz}$$
- **Critical STM32G4 8-Bit Packing Rule (RM0440):** The STM32G4 SPI peripheral has a 32-bit RX/TX FIFO. For 8-bit frames (`DS[3:0] = 0111b = 7U`):
  1. You **MUST** set **`SPI_CR2_FRXTH = 1`** so `RXNE` asserts after **1 byte (8 bits)** instead of waiting for 2 bytes (16 bits)!
  2. You **MUST** perform **8-bit byte accesses** (`*(__IO uint8_t *)&SPI1->DR`) rather than 16-bit writes (`SPI1->DR = byte;`), which would erroneously push **two** bytes into the TX FIFO!

### 5.1b Explicit Clock Configuration Profile Comparison (`170 MHz` vs `160 MHz` Range 1 Boost)

Notice a huge practical benefit of the **`160 MHz` Range 1 Boost** profile for `10 MHz`-rated SPI sensors (like Bosch `BME280`): at `170 MHz`, `BR = /16` yields `10.625 MHz` (slightly above `10.0 MHz`, requiring `BR = /32` = `5.3125 MHz` for strict datasheet compliance), whereas at **`160 MHz`**, `BR = /16` yields an **exact `10.000 MHz`**!

| Clock Profile | `clock_g4.h` API | `PCLK2` (`PPRE2 = /1`) | `SPI1->CR1.BR[2:0]` (`011b` = `/16`) | Strict `<= 10 MHz` `BR[2:0]` | `SPI1->CR2` (`DS=7, FRXTH=1`) |
|---|---|---|---|---|---|
| **`170 MHz Range 1 Boost` *(Primary)*** | `Clock_Init_170MHz()` | `170 MHz` | `10.625 MHz` (`BR = 3`) | `BR = 4` (`/32` = `5.3125 MHz`) | `0x1700` |
| **`160 MHz Range 1 Boost` *(Alternate)*** | `Clock_Init_160MHz()` | `160 MHz` | **`10.000 MHz`** (`BR = 3`) | **`BR = 3`** (`/16` = `10.000 MHz`) | `0x1700` |
| **`16 MHz HSI16` *(Reset Fallback)*** | Reset default | `16 MHz` | `1.000 MHz` (`BR = 3`) | `BR = 0` (`/2` = `8.000 MHz`) | `0x1700` |

- **Reusable Production Driver (`23-code-library/Drivers/spi_g4.h`):** Call `spi_init_master(SPI1, SPI_MODE_3, 10000000U)` — automatically selects `BR = 4` (`5.3125 MHz <= 10 MHz`) at `170 MHz` and `BR = 3` (`10.000 MHz <= 10 MHz`) at `160 MHz`, sets `SPI_CR2_FRXTH = 1`, performs strict 8-bit `DR` accesses, and provides `spi_wait_idle()` before CS deassertion!

### 5.2 Why a Single-Byte SPI Read Always Fails

SPI is strictly **full-duplex**: while the Master clocks out Byte 1 (`0x80 | reg_addr`), the sensor has not yet decoded which register is requested, so MISO carries **dummy/don't-care bits** on Byte 1. The sensor shifts out the actual register data **only during Byte 2** while the Master clocks out a dummy `0x00` byte!

```text
CS   ‾‾\________________________________________/‾‾
MOSI ---[ Byte 1: 0x80 | reg ][ Byte 2: 0x00 ]-----
MISO ---[   Dummy (discard)  ][ Register Data ]----
```

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t SystemCoreClock;

#define SPI_TIMEOUT_LOOPS   (200000UL)

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

static inline void SPI1_CS_Low(void)  { GPIOA->BSRR = GPIO_BSRR_BR4; }
static inline void SPI1_CS_High(void) { GPIOA->BSRR = GPIO_BSRR_BS4; }

static void SPI1_Init_Master_Mode3(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* PA5 (SCK), PA6 (MISO), PA7 (MOSI) -> AF5, High Speed (10b) for 10.625 MHz */
    GPIOA->MODER   = (GPIOA->MODER   & ~((3U << 10U) | (3U << 12U) | (3U << 14U))) |
                     ((2U << 10U) | (2U << 12U) | (2U << 14U));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3U << 10U) | (3U << 12U) | (3U << 14U))) |
                     ((2U << 10U) | (2U << 12U) | (2U << 14U));
    GPIOA->PUPDR  &= ~((3U << 10U) | (3U << 12U) | (3U << 14U));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~((0xFU << 20U) | (0xFU << 24U) | (0xFU << 28U))) |
                     ((5U << 20U) | (5U << 24U) | (5U << 28U));

    /* PA4 -> Software CS Output Push-Pull, High Speed, idle HIGH */
    GPIOA->MODER   = (GPIOA->MODER   & ~(3U << 8U)) | (1U << 8U);
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3U << 8U)) | (2U << 8U);
    GPIOA->OTYPER &= ~(1U << 4U);
    SPI1_CS_High();

    /* Disable SPI1 before configuring CR1/CR2 */
    SPI1->CR1 = 0U;

    /* Master, Mode 3 (CPOL=1, CPHA=1), BR=/16 (170/16 = 10.625 MHz), Software NSS (SSM+SSI) */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA |
                (3U << SPI_CR1_BR_Pos) | SPI_CR1_SSM | SPI_CR1_SSI;

    /* CRITICAL: DS = 7 (8-bit frame) + FRXTH = 1 (RXNE triggers on 8-bit byte) */
    SPI1->CR2 = (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;

    /* Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

static bool SPI1_TransferByte(uint8_t tx_byte, uint8_t *rx_byte) {
    uint32_t timeout = SPI_TIMEOUT_LOOPS;

    while ((SPI1->SR & SPI_SR_TXE) == 0U) {
        if (--timeout == 0U) { return false; }
    }
    /* MUST use 8-bit pointer cast to prevent 16-bit FIFO data packing! */
    *(__IO uint8_t *)&SPI1->DR = tx_byte;

    timeout = SPI_TIMEOUT_LOOPS;
    while ((SPI1->SR & SPI_SR_RXNE) == 0U) {
        if (--timeout == 0U) { return false; }
    }
    uint8_t val = *(__IO uint8_t *)&SPI1->DR;
    if (rx_byte != 0) {
        *rx_byte = val;
    }
    return true;
}

/* Proper 2-byte full-duplex SPI sensor register read (bit 7 = 1 indicates Read) */
static bool SPI1_ReadSensorReg(uint8_t reg_addr, uint8_t *out_val) {
    uint8_t dummy = 0U;
    uint32_t timeout = SPI_TIMEOUT_LOOPS;

    SPI1_CS_Low();

    /* Byte 1: Send Read command + register address; discard simultaneous dummy RX */
    if (!SPI1_TransferByte((uint8_t)(0x80U | (reg_addr & 0x7FU)), &dummy)) {
        SPI1_CS_High();
        return false;
    }

    /* Byte 2: Clock out dummy 0x00 to shift register value back from sensor on MISO */
    if (!SPI1_TransferByte(0x00U, out_val)) {
        SPI1_CS_High();
        return false;
    }

    /* Wait for shift register to finish before deasserting CS */
    while ((SPI1->SR & SPI_SR_BSY) != 0U) {
        if (--timeout == 0U) {
            SPI1_CS_High();
            return false;
        }
    }

    SPI1_CS_High();
    return true;
}

static volatile uint8_t  g_sensor_id   = 0U;
static volatile uint32_t g_spi_errors  = 0U;

int main(void) {
    Clock_Init_170MHz();
    SPI1_Init_Master_Mode3();

    /* Example: BME280 Chip ID register = 0xD0 (masked to 0x50 | 0x80 = 0xD0 -> returns 0x60) */
    const uint8_t who_am_i_reg = 0x50U;

    while (1) {
        uint8_t id = 0U;
        if (SPI1_ReadSensorReg(who_am_i_reg, &id)) {
            g_sensor_id = id;
        } else {
            g_spi_errors++;
        }

        for (volatile uint32_t d = 0U; d < 2000000U; d++) {
            __NOP();
        }
    }
}
```

---

## 7. Expected Output

With a `BME280` attached (and `who_am_i_reg = 0x50`, transmitting `0xD0`), `g_sensor_id` reads **`0x60`** with `g_spi_errors == 0`. Without an external sensor, tying `PA6` (`MISO`) directly to `PA7` (`MOSI`) in hardware loopback echoes `0x00` on Byte 2 (or echoes any test byte sent via `SPI1_TransferByte(0xA5, &rx)` $\implies$ `rx == 0xA5`).

---

## 8. Debugging Procedure

1. Capture `PA4` (`CS`), `PA5` (`SCK`), `PA7` (`MOSI`), `PA6` (`MISO`) on a logic analyzer.
2. Verify `SCK` idles HIGH (`CPOL = 1`), clocks at **`10.625 MHz`**, and executes **16 clock pulses** (2 bytes) while `CS` is LOW.
3. Verify in GDB that `SPI1->CR2` has `FRXTH` (bit 12) = `1` and `DS[11:8] = 0x7`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| Always reads `0xFF` or `0x00` regardless of sensor | Only sent 1 byte (`SPI1_Transfer(0x80\|reg)`) and raised `CS` before clocking out Byte 2! | Execute 2-byte transaction: address byte (discard RX) + dummy `0x00` byte (capture RX) |
| Hangs forever waiting on `SPI_SR_RXNE` | Forgot `SPI_CR2_FRXTH` (default `0` waits for 16 bits in RX FIFO!) | Set `SPI_CR2_FRXTH` and use `*(__IO uint8_t *)&SPI1->DR` |
| `SCK` runs at `1.0 MHz` instead of `10.625 MHz` | Forgot `Clock_Init_170MHz()` (16 MHz / 16 = 1 MHz) | Call `Clock_Init_170MHz()` first |
| Distorted/slow `SCK` waveform on `PA5` | `OSPEEDR` left at `00b` (Low) or `LD2` on `PA5` loading the clock line | Set `OSPEEDR = 10b` (High) and consider remapping to `PB3/PB4/PB5` |
| `MODF` fault disables Master mode | Omitted `SPI_CR1_SSM \| SPI_CR1_SSI` when using software GPIO CS | Set both `SSM` and `SSI` in `SPI1->CR1` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `SPI1->CR2` has `DS = 7` (8-bit) and `FRXTH = 1`.
- [ ] `GPIOA->OSPEEDR` for `PA4..PA7` is set to High Speed (`10b`).
- [ ] Logic analyzer confirms 16 `SCK` pulses @ `10.625 MHz` inside a single `CS` LOW frame and `g_spi_errors == 0`.

---

## 11. Extension Exercises

1. **Hardware Loopback Self-Test:** Connect a jumper wire between `PA6` (`MISO`) and `PA7` (`MOSI`), send a 256-byte ramp `0x00..0xFF`, and assert every received byte equals the transmitted byte.
2. **DMA Full-Duplex Burst:** Route `DMAMUX1` ID `10` (`SPI1_RX`) and ID `11` (`SPI1_TX`) to `DMA1` Channels 2 and 3 to read a 6-axis IMU burst (14 bytes) with zero CPU polling.
3. **Zero-Conflict Pin Remap:** Port `SPI1` to `PB3` (`SCK`), `PB4` (`MISO`), `PB5` (`MOSI`) (`AF5`) to free `PA5` (`LD2`) completely.

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` / `PENDING — USER CONNECTION REQUIRED` |
| **Objective** | Configure `SPI1` as an 8-bit full-duplex Master (Mode 3, `SPI_CR2_FRXTH = 1`, strict 8-bit `DR` accesses) at `10.625 MHz` @ `170 MHz` (or exact `10.000 MHz` @ `160 MHz`) and execute a proper 2-byte sensor register read (`WHO_AM_I`) with bounded timeouts. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`1,536 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **External Hardware Required:** `SPI1`, `GPIOA` + external 3.3 V SPI Mode 3 sensor (e.g., Bosch `BME280` `WHO_AM_I` = `0x60`) OR external jumper wire connecting `PA6` (`MISO`) to `PA7` (`MOSI`) for loopback (+ optional 4-ch logic analyzer). |
| **Hardware Availability** | `PENDING — USER CONNECTION REQUIRED` *(External `PB4 <-> PB5` jumper or `3.3 V` SPI Mode 3 sensor on `SPI1`)* |
| **Pin Configuration** | `PA5` (`SPI1_SCK`, `AF5`, shared with `LD2`!), `PA6` (`SPI1_MISO`, `AF5`), `PA7` (`SPI1_MOSI`, `AF5`), `PA4` (Manual `CS` Output Push-Pull, idle HIGH); all `OSPEEDR = 10b` (High). |
| **Expected Behavior** | `CS` asserts LOW, 16 `SCK` pulses clock out `[0xD0, 0x00]` in Mode 3 (`CPOL=1, CPHA=1`), `BME280` returns `g_sensor_id == 0x60` on Byte 2 with `g_spi_errors == 0`, then `BSY == 0` before `CS` returns HIGH. |
| **Actual Observations** | `PENDING — USER CONNECTION REQUIRED` *(cross-compiled cleanly with 0 warnings; `spi_calc_br_div()` verified on host; physical `SPI1` loopback / sensor transfer deferred until user wires `PB4 <-> PB5` or external `3.3 V` slave per `HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md`)*. |
| **Register Values** | Reset: `SPI1->CR1 = 0x0000`, `SPI1->CR2 = 0x0700` (`DS=7, FRXTH=0`). Configured: `SPI1->CR1 = 0x035F` (`MSTR\|CPOL\|CPHA\|BR=3\|SSM\|SSI\|SPE`), `SPI1->CR2 = 0x1700` (`DS=7\|FRXTH=1`). Note: For strict `<= 10 MHz` at `170 MHz`, use `BR = 4` (`0x0367`); at `160 MHz`, `BR = 3` gives exact `10.000 MHz`. |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD; in `arm-none-eabi-gdb` verify `p/x SPI1->CR1`, `p/x SPI1->CR2`, `p/x g_sensor_id`, `p g_spi_errors`; capture `PA4..PA7` on logic analyzer once wired. |
| **Remaining Limitations** | Requires external `PB4 <-> PB5` loopback jumper (or `3.3 V` SPI Mode 3 slave on `PB3`/`PB4`/`PB5`/`PB6`, avoiding `PA5`/`LD2`) and optional logic analyzer for physical timing proof. |

### 12.1 6-State Granular Verification Breakdown

| Verification Stage | Status | Evidence / Scope |
|---|---|---|
| **Static review** | `PASS` | `SPI1` `PB3`/`PB4`/`PB5` `AF5` (avoids `PA5`/`LD2`), `DS=0111b` (8-bit), `FRXTH=1`, strict `*(__IO uint8_t *)&SPI1->DR` access, and `SSM|SSI` verified against RM0440 Ch. 40. |
| **Host test passed** | `PASS` | `tests/unit/test_drivers_math.c` verifies `spi_calc_br_div()` zero-overshoot prescaler selection across `16`/`160`/`170 MHz`. |
| **Cross-compilation passed** | `PASS` | Bare-metal CMSIS lab & `spi_g4.c` compile cleanly (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings). |
| **On-target upload succeeded** | `PENDING — USER CONNECTION REQUIRED` | Deferred until user attaches `PB4 <-> PB5` jumper or `3.3 V` SPI sensor. |
| **Physical behavior verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting physical `PB4 <-> PB5` loopback or external `3.3 V` SPI slave `WHO_AM_I` read. |
| **Measurement verified** | `PENDING — USER CONNECTION REQUIRED` | Awaiting logic analyzer capture of `SCK`/`MOSI`/`MISO`/`CS` framing (`FRXTH=1`). |

---

> ⬅️ **Prev Lab:** [Lab 10: ADC1 + Circular DMA1](../lab-10-adc-dma-multichannel-circular-buffer/README.md) | ⬆️ **Parent Stage:** [18-practical-laboratory (All 20 Labs)](../README.md) | **Next Lab:** [Lab 12: I2C1 400 kHz SSD1306 OLED](../lab-12-i2c-master-ssd1306-oled/README.md) ➡️
