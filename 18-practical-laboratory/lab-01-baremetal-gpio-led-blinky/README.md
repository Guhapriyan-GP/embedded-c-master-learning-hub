# Lab 01: Bare-Metal GPIO LED Blinky (Register vs LL vs HAL)

> **Verification Status:** `CROSS-COMPILED` (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror -O2`, 0 warnings) | **Hardware Queue:** `ONBOARD — READY FOR AGENT 4` *(Not Yet Executed on Physical NUCLEO-G474RE Hardware)*

> **Clock Configuration Profile:** **Primary = `16 MHz HSI16` (Reset Default)** | **Reusable Drivers:** [`23-code-library/Drivers/gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h) (`GPIO_EnableClock()`, `GPIO_ConfigOutput()`, `GPIO_SetPin()`, `GPIO_ResetPin()`, `GPIO_TogglePin()`) & [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h) (`Clock_Init_160MHz()`, `Clock_Init_170MHz()`, `Clock_DelayMs()`).

---

## 1. Objective & Concept IDs

Configure `GPIOA` Pin 5 (onboard Green User LED `LD2`) in push-pull digital output mode and toggle it atomically using the Bit Set/Reset Register (`BSRR`), while comparing the three industry-standard STM32 firmware abstraction layers: **Direct Register Access (CMSIS)**, **STM32Cube Low-Layer (LL)**, and **STM32Cube HAL**.

- **Target Concept IDs:** `EMB-MEM-001` (MMIO & `volatile`), `EMB-BIT-001` (Bitmask RMW & Atomic Set/Reset), `STM32-BUS-001` (AHB2 Clock Gating), `PERIPH-GPIO-001` (STM32G4 GPIO Architecture)
- **Scope Layer:** `[STM32G474RE Exact]` / `[STM32 Generic]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C, STM32G474RET6 LQFP64)
- **Host Interface:** USB-A to Micro-USB (or USB-C on newer revisions) data cable for ST-LINK/V3E power, flashing, and SWD debugging
- **External Components:** None required (uses onboard Green LED `LD2` driven via `PA5`)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1 or compatible GNU Arm Embedded Toolchain)
- `make` / CMake and `openocd` (or ST-LINK GDB Server / STM32CubeProgrammer)
- `arm-none-eabi-gdb` (or VS Code with Cortex-Debug extension and `STM32G474.svd`)
- CMSIS device headers (`stm32g474xx.h`, `core_cm4.h`) from `23-code-library/Drivers/CMSIS/`

---

## 4. Wiring & Pin Configuration

Per **ST UM2505 (NUCLEO-G474RE User Manual)**, onboard green user LED **`LD2`** is connected to **`PA5`** (Arduino `D13`) via solder bridge `SB42` (or `SB6` depending on board rev; active-HIGH: driving `PA5 = 1` illuminates `LD2`).

| MCU Pin | Board Net | Direction | GPIO Mode | Output Type | Pull | Speed | Alternate Function |
|---|---|---|---|---|---|---|---|
| `PA5` | `LD2` (Green LED) | Output | `01b` (General-Purpose Output) | `0b` (Push-Pull) | `00b` (No Pull) | `00b` (Low Speed) | N/A |

> [!IMPORTANT]
> **STM32G4 Reset State Trap (RM0440 §9.4.1):** Unlike older STM32F4 MCUs where `GPIOx->MODER` resets to `00b` (Input), on **STM32G474RE** all non-debug GPIO pins reset to **`11b` (Analog Mode)** (`GPIOA->MODER` reset value = `0xABFFFFFF` — preserving SWD/JTAG on `PA13`/`PA14`/`PA15` while setting `PA0..PA12` to `11b`!). Always clear both mode bits (`&= ~(3U << (pin * 2U))`) before ORing the desired mode.

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 7** (*Reset and Clock Control — RCC*, §7.4.14 `RCC_AHB2ENR`) and **RM0440 Chapter 9** (*General-purpose I/Os — GPIO*, §9.4).

| Register | Base + Offset | Target Bits | Reset Value | Configured Value | Purpose |
|---|---|---|---|---|---|
| `RCC->AHB2ENR` | `0x40021000 + 0x4C` | Bit `0` (`GPIOAEN`) | `0x00000000` | `1` | Ungates AHB2 clock to `GPIOA` |
| `GPIOA->MODER` | `0x48000000 + 0x00` | Bits `[11:10]` (`MODER5`) | `11b` (Analog) | `01b` (Output) | Selects General-Purpose Output on `PA5` |
| `GPIOA->OTYPER` | `0x48000000 + 0x04` | Bit `5` (`OT5`) | `0` | `0` | Push-pull output stage |
| `GPIOA->OSPEEDR` | `0x48000000 + 0x08` | Bits `[11:10]` (`OSPEEDR5`) | `00b` | `00b` | Low slew rate (minimizes EMI for slow LED) |
| `GPIOA->PUPDR` | `0x48000000 + 0x0C` | Bits `[11:10]` (`PUPDR5`) | `00b` | `00b` | No internal pull-up/pull-down |
| `GPIOA->BSRR` | `0x48000000 + 0x18` | Bit `5` (`BS5`), Bit `21` (`BR5`) | `0x00000000` (WO) | `1U << 5` / `1U << 21` | Single-cycle atomic set/reset of `ODR5` |

### 5.1 Explicit Clock Configuration Profile Declaration (16 MHz vs 160 MHz vs 170 MHz)

This introductory lab intentionally runs at the STM32G474RE reset clock (**`16 MHz HSI16`**) so learners master GPIO registers before PLL switching (Lab 03). Because `delay_cycles()` is a software busy-wait loop, switching to a Range 1 Boost PLL profile scales the loop count directly with `SYSCLK`:

| Clock Profile | `clock_g4.h` API | `SYSCLK` / `HCLK` | `FLASH->ACR` Latency | Equivalent `delay_cycles()` Count for Same Cadence | Recommended Deterministic Delay |
|---|---|---|---|---|---|
| **`16 MHz HSI16` *(Primary here)*** | Reset default (`CLOCK_PROFILE_HSI16`) | `16,000,000 Hz` | `0 WS` (Range 1 Normal) | `400,000` | `SysTick` (`LOAD = 15,999`) or `Clock_DelayMs()` |
| **`160 MHz Range 1 Boost`** | `Clock_Init_160MHz()` (`M=4, N=80, R=2`) | `160,000,000 Hz` | `4 WS` (`R1MODE = 0`) | `4,000,000` (10.0× faster) | `Clock_DelayMs()` (auto-scales via `SystemCoreClock`) |
| **`170 MHz Range 1 Boost`** | `Clock_Init_170MHz()` (`M=4, N=85, R=2`) | `170,000,000 Hz` | `4 WS` (`R1MODE = 0`) | `4,250,000` (10.625× faster) | `Clock_DelayMs()` (auto-scales via `SystemCoreClock`) |

- **Reusable Production Driver Equivalent (`23-code-library/Drivers/`):** Replace manual register setup with `GPIO_ConfigOutput(GPIOA, 5U, GPIO_OTYPE_PUSHPULL, GPIO_SPEED_LOW, GPIO_PULL_NONE)` and `GPIO_TogglePin(GPIOA, 5U)` from [`gpio_g4.h`](../../23-code-library/Drivers/gpio_g4.h), paired with `Clock_DelayMs(500U)` from [`clock_g4.h`](../../23-code-library/Drivers/clock_g4.h).

### Bitfield Mask Calculation

- `MODER5` bit offset: $\text{pos} = 5 \times 2 = 10$.
- Clear mask: $\sim(3\text{U} \ll 10) = \text{0xFFFFF3FF}$.
- Set value for Output (`01b`): $(1\text{U} \ll 10) = \text{0x00000400}$.
- `BSRR` atomic set (`PA5 = 1`): $\text{BS5} = (1\text{U} \ll 5) = \text{0x00000020}$.
- `BSRR` atomic reset (`PA5 = 0`): $\text{BR5} = (1\text{U} \ll (5 + 16)) = (1\text{U} \ll 21) = \text{0x00200000}$.

> [!NOTE]
> **AHB2 Clock Enable Synchronization Delay:** Per RM0440 §7.2.17, a 2-AHB-clock delay is required after setting a peripheral enable bit in `RCC->AHB2ENR` before accessing that peripheral's registers. Performing a dummy read of `RCC->AHB2ENR` followed by `__DSB()` guarantees the clock gate has settled before writing `GPIOA->MODER`.

---

## 6. Code Implementation

### 6.1 Approach A — Direct Register Access (CMSIS Bare-Metal)

```c
#include "stm32g474xx.h"

/* Note: Busy-wait loop period depends on SYSCLK (16 MHz HSI16 at reset)
 * and compiler optimization (-O0 vs -O2). Use SysTick (Lab 04) for exact timing. */
static void delay_cycles(volatile uint32_t count) {
    while (count > 0U) {
        count--;
        __NOP();
    }
}

int main(void) {
    /* 1. Enable GPIOA peripheral clock on AHB2 + mandatory synchronization readback */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;
    __DSB();

    /* 2. Configure PA5 as General-Purpose Output (01b).
     *    CRITICAL: Clear both bits first because STM32G4 resets MODER to 11b (Analog). */
    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (1U << (5U * 2U));

    /* 3. Push-pull output, low slew rate, no pull-up/pull-down */
    GPIOA->OTYPER  &= ~(1U << 5U);
    GPIOA->OSPEEDR &= ~(3U << (5U * 2U));
    GPIOA->PUPDR   &= ~(3U << (5U * 2U));

    while (1) {
        /* Atomic Set via BSRR (Bit 5 = BS5) -> LD2 ON */
        GPIOA->BSRR = GPIO_BSRR_BS5;
        delay_cycles(400000U);

        /* Atomic Reset via BSRR (Bit 21 = BR5) -> LD2 OFF */
        GPIOA->BSRR = GPIO_BSRR_BR5;
        delay_cycles(400000U);
    }
}
```

### 6.2 Approach B — STM32Cube Low-Layer (LL) API Comparison

```c
/* Requires stm32g4xx_ll_bus.h and stm32g4xx_ll_gpio.h */
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_gpio.h"

void Lab01_LL_GPIO_Init(void) {
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA); /* Includes dummy readback internally */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_5, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_5, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_5, LL_GPIO_PULL_NO);
}

static inline void Lab01_LL_LED_Toggle(void) {
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_5);   /* Compiles to single STR to BSRR */
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_5); /* Compiles to single STR to BSRR */
}
```

### 6.3 Approach C — STM32Cube HAL API Comparison

```c
/* Requires stm32g4xx_hal.h and configured SysTick HAL_IncTick() */
#include "stm32g4xx_hal.h"

void Lab01_HAL_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE(); /* Includes dummy readback */

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_5;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void Lab01_HAL_LED_Loop(void) {
    while (1) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   /* Writes BSRR */
        HAL_Delay(500U);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); /* Writes BSRR */
        HAL_Delay(500U);
    }
}
```

### 6.4 3-Way Implementation Comparison: When to Use Each

| Criterion | Direct Register (CMSIS) | STM32Cube LL (`static inline`) | STM32Cube HAL |
|---|---|---|---|
| **Flash / RAM Overhead** | Minimal (tens of bytes) | Minimal (inlines to identical 1–2 instructions at `-O2`) | Moderate–High (handle structs, callbacks, error checks) |
| **Execution Latency** | 1–2 cycles (`STR` to `BSRR`) | 1–2 cycles (zero abstraction penalty) | ~10–30 cycles (parameter checks, branch/call overhead) |
| **Portability Across STM32** | Low (tied to G4 bit positions) | Medium (portable across G4/L4/H7 LL) | High (portable across STM32 families) |
| **Hardware Transparency** | 100% — forces mastery of RM0440 | High — 1:1 map to register bitfields | Low — hides register ordering & errata details |
| **When to Use in Production** | Bootloaders, ultra-tight ISRs, safety audits, learning hardware | High-speed production drivers (control loops, SPI/DMA ISRs) | Rapid prototyping, complex stacks (USB, FDCAN, CubeMX) |

---

## 7. Expected Output

On a NUCLEO-G474RE running at default reset clock (`16 MHz HSI16`), green LED **`LD2`** (`PA5`) blinks steadily at a visible cadence (exact frequency varies with `-O0` vs `-O2`; deterministic 1 Hz timing is introduced in Lab 04 via SysTick).

---

## 8. Debugging Procedure

1. Build with `-g3 -O0` and flash via OpenOCD / ST-LINK GDB Server.
2. Start GDB (`arm-none-eabi-gdb`) and set breakpoints on `GPIOA->BSRR = GPIO_BSRR_BS5;` and `GPIOA->BSRR = GPIO_BSRR_BR5;`.
3. Inspect registers in GDB:
   ```gdb
   (gdb) x/1wx 0x4002104C    # Verify RCC->AHB2ENR bit 0 == 1
   (gdb) x/1wx 0x48000000    # Verify GPIOA->MODER[11:10] == 01b (0x...4...)
   (gdb) x/1wx 0x48000014    # Observe GPIOA->ODR bit 5 toggle 0 <-> 1 across steps
   ```
4. In VS Code Cortex-Debug (`XPERIPHERALS` pane), expand `GPIOA -> MODER` and `GPIOA -> ODR` and confirm `MODER5 = 1` and `ODR5` toggles cleanly on each step over `BSRR`.

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| LED never turns on; `GPIOA->MODER` stays `0x00000000` in debugger | Forgot `RCC->AHB2ENR \|= RCC_AHB2ENR_GPIOAEN;` | Enable `GPIOAEN` and perform dummy readback before touching `GPIOA` |
| `PA5` remains stuck in Analog mode (`11b`) | Wrote `GPIOA->MODER \|= (1U << 10)` without clearing bit 11 first (`11b \| 01b = 11b`!) | Always clear 2-bit field first: `GPIOA->MODER &= ~(3U << 10U);` |
| LED blinks wildly fast or appears dimly solid ON at `-O2` | Non-`volatile` delay loop variable optimized away completely by compiler | Qualify loop counter `volatile` or use hardware `SysTick` (Lab 04) |
| Random pin glitches when another ISR modifies `GPIOA` | Used non-atomic `GPIOA->ODR ^= (1U << 5)` (read-modify-write hazard) | Use atomic `GPIOA->BSRR` writes exclusively |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror` (zero warnings).
- [ ] In GDB/SVD viewer, `RCC->AHB2ENR & 0x1 == 1` prior to `GPIOA->MODER` configuration.
- [ ] `GPIOA->MODER` bits `[11:10]` read `01b` while `PA13`/`PA14` SWD bits (`[29:26]`) remain untouched (`10b` Alternate Function — debugger stays connected!).
- [ ] Stepping over `GPIOA->BSRR = GPIO_BSRR_BS5` sets `GPIOA->ODR` bit 5 to `1` without altering any other `ODR` bit.
- [ ] Stepping over `GPIOA->BSRR = GPIO_BSRR_BR5` clears `GPIOA->ODR` bit 5 to `0`.

---

## 11. Extension Exercises

1. **Multi-Pin Atomic Update:** Connect two external LEDs to `PA6` and `PA7` (with 330 $\Omega$ series resistors) and write a single `GPIOA->BSRR` store that simultaneously turns `PA5` ON and `PA6` OFF in one clock cycle.
2. **Disassembly Audit:** Compile both Approach A (Register) and Approach B (LL) with `-O2` and run `arm-none-eabi-objdump -d` to verify that LL compiles to identical `STR` machine instructions with zero call overhead.
3. **Slew-Rate Measurement:** Toggle `PA5` in a tight loop and compare edge rise times on an oscilloscope across `OSPEEDR5 = 00b, 01b, 10b, 11b` (referencing DS12288 §5.3.14).

---

## 12. Standardized Verification Record

| Field | Record |
|---|---|
| **Verification Status** | `CROSS-COMPILED` *(Self-Contained Onboard — Queued for Physical Execution)* |
| **Objective** | Configure onboard Green LED `LD2` (`PA5`) out of STM32G4 Analog reset mode (`MODER5 = 11b -> 01b`) and blink atomically via `GPIOA->BSRR` while comparing Register, ST LL, and ST HAL APIs. |
| **Firmware Version** | `v2.2.0-remediation` (branch `feature/complete-remediation`, base commit `585dc4fa`) |
| **Compiler Version** | `arm-none-eabi-gcc 14.3.1 20250623 (GNU Tools for STM32 14.3.rel1.20251027-0700)` |
| **Build Result** | **PASS** — `arm-none-eabi-gcc -std=c11 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32G474xx -Wall -Wextra -Werror -O2 -c` succeeded with **0 warnings, 0 errors** (`960 B` object file). |
| **Test Environment** | Windows 11 + ARM GNU Toolchain 14.3.rel1 + NUCLEO-G474RE (`MB1367` Rev C, `STM32G474RET6`, STLINK-V3E `V3J17M11`, `COM8`) |
| **Required Peripherals** | **Onboard Only:** `RCC`, `GPIOA`, onboard Green LED `LD2` (`PA5` via `SB42`/`SB6`). Zero external wiring required. |
| **Hardware Availability** | `ONBOARD — READY FOR AGENT 4` |
| **Pin Configuration** | `PA5`: `MODER = 01b` (Output), `OTYPER = 0` (Push-Pull), `OSPEEDR = 00b` (Low), `PUPDR = 00b` (No Pull); `PA13`/`PA14` SWD pins preserved untouched. |
| **Expected Behavior** | At `16 MHz HSI16`, `LD2` (`PA5`) blinks steadily; `GPIOA->ODR` bit 5 toggles `0 <-> 1` cleanly via atomic `BSRR` writes (`BS5` / `BR5`). |
| **Actual Observations** | Pending Agent 4 Onboard Test *(not yet executed on physical silicon)*. |
| **Register Values** | Reset: `RCC->AHB2ENR = 0x00000000`, `GPIOA->MODER = 0xABFFFFFF`. Configured: `RCC->AHB2ENR[0] = 1`, `GPIOA->MODER = 0xABFFF7FF` (`MODER5[11:10] = 01b`), `GPIOA->OTYPER[5] = 0`, `GPIOA->BSRR = 0x00000020` (`BS5`) / `0x00200000` (`BR5`). |
| **Debugging Procedure** | Flash via `STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst` or OpenOCD (`openocd -f interface/stlink.cfg -f target/stm32g4x.cfg`); in `arm-none-eabi-gdb` verify `x/1wx 0x4002104C`, `x/1wx 0x48000000`, and `x/1wx 0x48000014` across `BSRR` steps. |
| **Remaining Limitations** | Busy-wait `delay_cycles()` timing depends on optimization level (`-O0` vs `-O2`) and `SYSCLK`; deterministic timing is implemented via `SysTick` (Lab 04) or `Clock_DelayMs()` (`clock_g4.h`). Physical board execution pending Agent 4. |
