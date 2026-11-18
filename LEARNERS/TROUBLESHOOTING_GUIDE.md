# Beginner-to-Production Troubleshooting Guide (STM32G474RE + Windows 11 + VS Code)

Every embedded engineer hits silent hangs, HardFaults, wrong baud rates, and stubborn link errors. Use these **Symptom → Root Cause → Exact Fix** lookup tables to diagnose failures in minutes instead of hours.

> **Concept IDs Covered:** `TOOL-WARN-001`, `TOOL-LD-001`, `STM32-RCC-001..002`, `PERIPH-GPIO-001`, `EMB-REG-003`, `EMB-VOL-001`, `PERIPH-UART-001`, `PERIPH-SPI-001`, `PERIPH-I2C-001`, `PERIPH-ADC-001`, `TOOL-OCD-001`

---

## 1. Compiler Errors & Strict Warning Traps (`arm-none-eabi-gcc` with `-Werror`)

This repository enforces `-std=c11 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef`. Warnings become errors on purpose to catch silent bugs before flashing.

| Symptom | Root Cause | Exact Fix |
|---|---|---|
| `error: implicit conversion changes signedness` or `conversion from 'int' to 'uint16_t'` (`-Wconversion`) | ISO C integer promotion widens `uint8_t`/`uint16_t` operands to signed `int` before arithmetic or bitwise `~`. | Add explicit `U` suffixes (`1U << 5U`), keep register masks `uint32_t`, and cast narrowed results intentionally: `(uint16_t)(val & 0xFFFFU)`. |
| `error: implicit conversion from 'float' to 'double'` (`-Wdouble-promotion`) | Un-suffixed floating-point literals like `3.3` are `double` in C, dragging slow software `double` math onto a single-precision Cortex-M4F (`fpv4-sp-d16`). | Append **`f`** to every float literal (`3.3f`, `4095.0f`) and use `sinf()`/`sqrtf()` instead of `sin()`/`sqrt()`. |
| `error: 'GPIOA' undeclared` or `stm32g4xx.h: #error Please select target STM32G4xx device` | Missing `-DSTM32G474xx` preprocessor macro in `CFLAGS` or `.vscode/c_cpp_properties.json`. | Pass `-DSTM32G474xx` to `arm-none-eabi-gcc` and include `stm32g474xx.h` (verify `INCLUDES` points to `23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include`). |
| `error: declaration of 'i' shadows a previous local` (`-Wshadow`) | Inner loop/block declares a variable with the same name as an outer variable or global symbol. | Rename the inner variable (`idx`, `bit_idx`) so scope intent is unambiguous. |
| Polling loop `while (flag == 0) {}` hangs forever at `-O2` (works at `-Og`) | Missing `volatile` on a variable modified inside an ISR or a raw memory-mapped register pointer (`EMB-VOL-001`). The optimizer caches `flag` in a CPU register once! | Declare ISR-shared flags `static volatile uint32_t g_flag = 0U;` and access hardware via CMSIS `__IO` structs (`GPIOA->IDR`). |

---

## 2. Linker & Memory Map Errors (`arm-none-eabi-ld` & `stm32g474re.ld`)

| Symptom | Root Cause | Exact Fix |
|---|---|---|
| `undefined reference to '_sbrk'`, `'_write'`, `'_close'`, `'_read'` | Linking Newlib `printf`/`malloc` without syscall stubs or `--specs=nosys.specs`. | Add `--specs=nano.specs --specs=nosys.specs` in `LDFLAGS`, and implement `_write(int fd, char *ptr, int len)` when retargeting `printf` to `USART2` (`Lab 08`). |
| `Error: .isr_vector must be 512-byte aligned` or `SIZEOF(.isr_vector) == 472` assertion fails | STM32G474RE has 16 core + 102 external IRQs = **118 vectors = 472 bytes**, requiring next power-of-2 alignment (**512 bytes**, `0x200`) for `SCB->VTOR`. | Keep `. = ALIGN(512); KEEP(*(.isr_vector))` in [`stm32g474re.ld`](../23-code-library/templates/stm32g474re.ld) and verify all 118 entries exist in `startup_stm32g474re.c`. |
| `region 'RAM' overflowed by X bytes` or `FLASH overflowed` | Large local/global buffers or un-`const` lookup tables placed in `RAM` (96 KB SRAM1+SRAM2) instead of `FLASH` (512 KB). | Mark read-only tables `static const` (moves them to `.rodata` in Flash), enable `-ffunction-sections -fdata-sections -Wl,--gc-sections`, and inspect `build/firmware.map`. |
| Functions/data tagged `__attribute__((section(".ccmram")))` crash with UsageFault/garbage | Either `CCMRAM` lacks `(xrw)` execute permission, or `Reset_Handler` failed to copy `_siccmram` (Flash LMA) → `_sccmram.._eccmram` (`0x10000000` VMA). | Verify `CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 32K`, `_siccmram = LOADADDR(.ccmram);`, and the copy loop in `Reset_Handler` before calling `main()`. |
| DMA transfer to a buffer placed in CCM-SRAM fails with DMA Transfer Error (`TEIF`) | Native CCM-SRAM address `0x10000000` is connected **only** to Cortex-M4 I-Code/D-Code buses — DMA cannot see `0x10000000`! | Either place DMA buffers in SRAM1/SRAM2 (`0x20000000–0x20017FFF`) or program `DMA_CMARx` with the STM32G4 S-Bus CCM-SRAM alias **`0x20018000` + offset**. |
| `warning: LOAD segment with RWX permissions` | Benign GNU `ld` 2.39+ informational warning because `CCMRAM (xrw)` holds executable code in RAM. | Safe to ignore (or pass `-Wl,--no-warn-rwx-segments` if desired). Confirm ELF sections with `arm-none-eabi-readelf -SW build/firmware.elf`. |

---

## 3. Clock Tree & Power Scaling Problems (`RCC`, `PWR`, `FLASH`)

| Symptom | Root Cause | Exact Fix |
|---|---|---|
| Stuck forever polling `while (!(RCC->CR & RCC_CR_HSIRDY))` or wrong bit polled | Hardcoding STM32F4 bit 0/1 instead of STM32G4 **`HSION` = Bit 8**, **`HSIRDY` = Bit 10** (`RCC_CR`). | Always use CMSIS bit masks (`RCC_CR_HSION`, `RCC_CR_HSIRDY`, `RCC_CR_HSEON`, `RCC_CR_HSERDY`) and include a bounded timeout counter. |
| Stuck forever polling `PWR->SR1` for `VOSF` or writing `PWR->CR5` at offset `0x10` | On STM32G4, `VOSF` is **Bit 10 of `PWR->SR2` (`0x14`)**, and `R1MODE` is **Bit 8 of `PWR->CR5` (`0x80`)**! Also forgetting `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;`. | Enable `PWREN` + readback, poll `while (PWR->SR2 & PWR_SR2_VOSF)`, then clear `PWR->CR5 &= ~PWR_CR5_R1MODE;` for Range 1 Boost (`> 150 MHz`). |
| MCU crashes with HardFault / BusFault the instant `RCC->CFGR` switches to 170 MHz (or 160 MHz) PLL | Missing one of the 3 mandatory RM0440 §6.1.5 prerequisites: (1) `PWR->CR5.R1MODE = 0` (Boost Mode), (2) `FLASH->ACR.LATENCY = 4WS` verified by readback, or (3) **intermediate `HPRE = /2` (85 MHz) for $\ge 1\ \mu\text{s}$** before switching `HPRE = /1`! | Follow [`23-code-library/Drivers/clock_g4.c`](../23-code-library/Drivers/clock_g4.c) exact sequence: `PWREN` → `VOS=01b` + poll `VOSF==0` → `R1MODE=0` → `4WS` + poll → `PLLON` + poll `PLLRDY` → `HPRE=/2` → `SW=PLL` + poll `SWS==PLL` → delay $\ge 1\ \mu\text{s}$ → `HPRE=/1`. |
| Stuck forever waiting on `PLLRDY` | Modifying `RCC->PLLCFGR` while `PLLON == 1`, or violating DS12288 bounds ($f_{\text{PLL\_IN}} \notin [2.66, 16]\text{ MHz}$ or $f_{\text{VCO}} \notin [96, 344]\text{ MHz}$), or forgetting `RCC_PLLCFGR_PLLREN`. | Switch SYSCLK to HSI16, clear `PLLON`, wait `PLLRDY == 0`, program valid $M/N/R$ with `PLLREN = 1`, then set `PLLON`. |
| UART prints garbage / LED blinks 10.6× too slow or too fast | Silent clock mismatch: code calculates `BRR` or `TIM2->PSC` assuming **170 MHz** (or **160 MHz**), while MCU is still running on default **16 MHz HSI16** (or vice versa), or `SystemCoreClockUpdate()` was never called. | Call `Clock_Init_170MHz()` (and check its `bool` return!), call `SystemCoreClockUpdate()`, and verify `RCC->CFGR` (`SWS=11b`, `HPRE=0`) via GDB `clock_regs` macro. |

---

## 4. STM32G474RE Register & Silicon Gotchas

| Symptom | Root Cause | Exact Fix |
|---|---|---|
| Peripheral register writes silently read back `0x00000000` (or BusFault) | Peripheral bus clock not enabled in `RCC->AHB1ENR`/`AHB2ENR`/`APB1ENR1`/`APB2ENR`, or register written on the very next cycle before clock gate propagates. | Enable the exact clock bit (note: **`DAC1–4` are on `AHB2ENR`**, `SYSCFG` on `APB2ENR`, `DMAMUX1EN` on `AHB1ENR`) and perform a dummy readback: `(void)RCC->AHB2ENR;`. |
| GPIO input (`IDR`) always reads `0` even when pin is driven to `3.3 V` | **STM32G4 resets `GPIOx->MODER` to `11b` (Analog Mode)** (NOT `00b` Input as on STM32F4!), which disconnects the digital Schmitt trigger! | Explicitly clear the 2-bit `MODER` field to `00b`: `GPIOC->MODER &= ~(3U << (13U * 2U));`. |
| Writing `EXTI->PR1 |= EXTI_PR1_PIF13;` or `ADC1->ISR |= ADC_ISR_EOC;` drops unrelated interrupts | `rc_w1` (write-1-to-clear) registers clear **every** bit written as `1`. A read-modify-write `|=` reads other pending `1` bits and accidentally writes `1` back to clear them! | Always use **direct assignment `=`**: `EXTI->PR1 = EXTI_PR1_PIF13;` and `ADC1->ISR = ADC_ISR_EOC;`. |
| Writing `TIM2->SR &= ~TIM_SR_UIF;` misses concurrent capture/compare flags | `TIMx->SR` is **`rc_w0` (write-0-to-clear)**. An RMW `&=` reads `0` for a flag that sets between the read and write, then writes `0` back and destroys it! | Always write the inverted mask directly: `TIM2->SR = ~TIM_SR_UIF;` (writes `0` only to `UIF` and `1` to all other bits, leaving them untouched). |
| ISR fires twice per single button press or timer tick | Cortex-M4 write buffer delays the peripheral flag-clear store past ISR return (`BX LR`), so NVIC re-enters the ISR immediately (`ES0430 §2.1.3`). | Clear the pending flag at the **start** of the ISR and execute `__DSB();` before returning. |
| Blue button `B1` (`PC13`) works inverted or reads stuck HIGH | NUCLEO-G474RE (`MB1367`, `UM2505`) wires `B1` **Active-HIGH** with an external pull-down. Enabling internal pull-up fights the pull-down; using falling edge (`FTSR1`) triggers on release instead of press! | Set `PUPDR = 00b` (no pull), test `(GPIOC->IDR & (1U << 13U)) != 0U` for pressed, and use **rising edge (`EXTI->RTSR1`)**. |
| `I2C1_SCL` on `PB8` stuck LOW (~0.66 V) or MCU boots into ROM bootloader | `PB8` is shared with `BOOT0` and has an onboard **10 kΩ pull-down (`R36`)** on MB1367. Internal ~40 kΩ pull-ups create a $0.66\text{ V}$ divider. | Fit external **2.2 kΩ–3.3 kΩ** pull-ups (checking `BOOT0` state at reset), or remap `I2C1_SCL` to `PA15` (AF4). |
| Signals on `PA9`, `PA10`, `PB6`, or `PB4` are dragged down by ~5.1 kΩ | STM32G4 resets with USB-C **UCPD dead-battery pull-downs enabled** (`PWR_CR3.UCPD_DBDIS == 0`). | Enable `PWREN` and execute `PWR->CR3 |= PWR_CR3_UCPD_DBDIS;` early in startup. |
| `ADC1` readings are noisy, wildly inaccurate, or stuck after reset | Two classic STM32G4 ADC traps: (1) **`ADC_CR` resets to `0x20000000` (`DEEPPWD=1`)** — must clear `DEEPPWD`, set `ADVREGEN=1`, wait $\ge 20\ \mu\text{s}$, calibrate (`ADCAL`) before `ADEN`; (2) setting `CKMODE = 01` (`HCLK/1 = 170 MHz`) or `10` (`85 MHz`) **overclocks** the 60 MHz ADC spec! | Use **`CKMODE = 11b` (`HCLK/4 = 42.5 MHz <= 60 MHz`)** and follow the exact `DEEPPWD=0` → `ADVREGEN=1` → 20 µs → `ADCAL` → `ADEN` sequence (`Lab 09`). |
| `DMA1` transfer stops or drops sibling-channel flags | Writing `CGIFx` in `DMA1->IFCR` triggers `ES0430 §2.3.1` erratum when another flag asserts simultaneously; or forgetting `RCC->AHB1ENR |= RCC_AHB1ENR_DMAMUX1EN`. | Enable `DMAMUX1EN` + `DMA1EN`, and clear **individual** flags (`DMA_IFCR_CTCIF1`, `DMA_IFCR_CHTIF1`) — never `CGIFx`. |

---

## 5. ST-LINK / OpenOCD / GDB / Windows 11 Failures

| Symptom | Root Cause | Exact Fix |
|---|---|---|
| `openocd` fails with `LIBUSB_ERROR_ACCESS` or `Error: open failed` | Another tool (`STM32CubeProgrammer`, a previous orphaned `openocd.exe`, or STM32CubeIDE) already holds the STLINK-V3E USB handle. | Close CubeProgrammer/CubeIDE, run `taskkill /F /IM openocd.exe` in PowerShell, unplug/replug `CN1`, and retry. |
| Serial Monitor / PuTTY says `Access denied` or `Cannot open COMx` | Another terminal tab (VS Code Serial Monitor, Tera Term, Python `pyserial`) already has `COMx` open. | Close the competing serial terminal; verify the active port number in **Device Manager → Ports (COM & LPT)**. |
| `Cannot connect to target` / `SWD DPIDR failed` after flashing Low-Power or bad GPIO code | Firmware entered Stop/Standby mode, disabled `SWDIO`/`SWCLK` (`PA13`/`PA14`), or crashed continuously before SWD could attach. | Hold down the black **Reset (`B2`)** button on NUCLEO-G474RE, run `STM32_Programmer_CLI -c port=SWD mode=UR -e all` (Connect Under Reset), and release `B2` when prompted (or use OpenOCD `-c "reset_config srst_only srst_nogate connect_assert_srst"`). |
| Firmware hangs when unplugged from debugger, but works inside GDB | Unconditional `__asm__("bkpt #0")` (or semihosting) executes with no debugger attached, escalating immediately to a HardFault! | Guard software breakpoints with `if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U) { __asm__ volatile("bkpt #0"); }` (see [`fault_handler.c`](../23-code-library/Drivers/fault_handler.c)). |
| VS Code `XPERIPHERALS` pane is blank during `F5` debug | `23-code-library/Drivers/SVD/STM32G474.svd` has not been downloaded/copied yet. | Follow Step 5.2 of [`BEGINNER_SETUP_WINDOWS11.md`](BEGINNER_SETUP_WINDOWS11.md) to place `STM32G474.svd` at that exact path. |
| Watchdog resets MCU while paused at a GDB breakpoint | `IWDG` / `WWDG` keeps counting when the CPU core halts unless frozen in `DBGMCU`. | Set `DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP | DBGMCU_APB1FZR1_DBG_WWDG_STOP;` during development (`Lab 13`). |

---

## 6. Peripheral Communication Failures (`USART`, `SPI`, `I2C`)

| Symptom | Root Cause | Exact Fix |
|---|---|---|
| **UART:** Garbled characters or Framing Error (`FE`) | Wrong `BRR` (clock mismatch between 16 MHz HSI16 vs 170 MHz PLL), wrong terminal baud (not `115200 8N1`), or missing `PA2`/`PA3` `AF7` (`USART2`) / `AF12` (`LPUART1`). | Verify `BRR = round(f_CK / 115200)` (`1476` = `0x05C4` at 170 MHz; `139` = `0x008B` at 16 MHz), confirm `RCC->CCIPR.USART2SEL = 00b` (PCLK1), and check `ISR.ORE` (clear with `ICR = USART_ICR_ORECF`). |
| **UART:** Receiver stops forever after a burst of bytes | Overrun Error (`USART_ISR_ORE`) latched when RX wasn't read in time — STM32G4 UART blocks further RXNE until `ORE` is cleared. | Always check and clear `ORE` in the RX ISR (`USART2->ICR = USART_ICR_ORECF;`) and buffer incoming bytes in a lock-free SPSC ring buffer (`Lab 08`). |
| **SPI:** Polling `while (!(SPI1->SR & SPI_SR_RXNE))` hangs forever on 8-bit transfer | STM32G4 has a 32-bit RX FIFO whose default threshold (`SPI_CR2.FRXTH = 0`) waits for **16 bits** before setting `RXNE`! | Set **`SPI1->CR2 |= SPI_CR2_FRXTH;`** (bit 12 = 8-bit threshold) and write/read bytes via `*(__IO uint8_t *)&SPI1->DR` (never 16-bit write to `DR` for 1 byte!). |
| **SPI:** Sensor register read always returns `0x00` or `0xFF` | Sending only the 1-byte register address without clocking out a **2nd dummy byte (`0x00`/`0xFF`)**, or wrong `CPOL`/`CPHA` mode, or `CS` toggled too early before `BSY == 0`. | Assert `CS` LOW → send `(reg | 0x80)` and discard RX byte 1 → send dummy `0x00` and keep RX byte 2 → wait `!(SPI1->SR & SPI_SR_BSY)` → release `CS` HIGH (`Lab 11`). |
| **I2C:** Immediate `NACKF` (Not Acknowledge) | Wrong 7-bit address shift (STM32G4 `I2C_CR2.SADD[7:1]` expects **`addr7 << 1`**!), missing external pull-ups (`2.2–4.7 kΩ`), or `PB8-BOOT0` 10 kΩ pull-down trap. | Shift 7-bit slave address left by 1 (`(uint32_t)addr7 << 1U`), check `NACKF` in every polling loop, clear via `I2C1->ICR = I2C_ICR_NACKCF`, and verify pull-ups on `SCL`/`SDA` (`Lab 12`). |
| **I2C:** Bus permanently stuck `ISR.BUSY == 1` after MCU reset mid-transaction | An external slave was pulling `SDA` LOW when the MCU reset, locking the bus. | Before enabling `I2C1`, configure `SCL`/`SDA` as Open-Drain GPIO outputs, clock **`9` manual `SCL` pulses** until `SDA` releases HIGH, generate a manual `STOP` condition, then switch pins to `AF4` (`Lab 12`). |
