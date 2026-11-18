# Project 01: Smart Instrument Panel & Telemetry Unit

> **Verification Status:** Statically Reviewed Architecture & Pin/Clock Specification (Integration Blueprint — Refer to Compile-Verified Labs 02, 03, 04, 05, 07, 08, 09, 13 for Individual Peripheral Drivers)

---

## 1. Project Overview & Architecture

The Smart Instrument Panel is a multi-peripheral bare-metal STM32G474RE application (`170 MHz SYSCLK`, Range 1 Boost Mode) that monitors user inputs (Active-HIGH button `B1` on `PC13`, analog potentiometer on `PA0`), modulates physical actuator output (**`TIM2_CH2` hardware PWM on `PA1`**, eliminating the `PA0` pin conflict!), and streams structured JSON telemetry over `USART2` (`PA2`/`PA3`, `115200 8N1`) while supervised by the hardware Independent Watchdog (`IWDG`).

```text
[ Pushbutton B1 (PC13, Active-HIGH) ] ──> [ EXTI13 Rising Edge Flag ] ─┐
                                                                       ├──> [ Non-Blocking Superloop @ 170 MHz ]
[ 10 kΩ Potentiometer (PA0 / ADC1_IN1) ] ──> [ ADC1 @ 42.5 MHz ] ──────┘      │           │            │
                                                                              │           │            │
                                                    [ TIM2_CH2 PWM (PA1) ] <──┘           │            └──> [ IWDG (1.0 s) ]
                                                                                          ▼
                                                                             [ USART2 (PA2/PA3, 115200) ]
                                                                                          │
                                                                                          ▼
                                                                                  [ Host PC Terminal ]
```

---

## 2. Verified Conflict-Free Hardware Subsystems (STM32G474RE / NUCLEO-G474RE)

| Subsystem | Peripheral & Channel | MCU Pin | `AFR` / Mode | Exact Verified Configuration |
|---|---|---|---|---|
| **System Clock** | `RCC` + `PWR` + `FLASH` | Internal | `170 MHz` PLL | HSI16 (`M=4, N=85, R=2`), `PWR_CR5.R1MODE=0`, `4WS`, RM0440 §6.1.5 `HPRE=/2 -> >=1 us -> /1` |
| **Analog Input** | `ADC1_IN1` | **`PA0`** (Arduino `A0`) | Analog (`11b`) | **`CKMODE = 11b` (`42.5 MHz <= 60 MHz`)**, `SMP1 = 47.5` cycles, single-ended calibrated |
| **Actuator PWM** | **`TIM2_CH2`** | **`PA1`** (Arduino `A1`) *(alt: `PB10` `TIM2_CH3` `AF1`)* | `AF1` (`10b`) | `20.000 kHz` PWM (`PSC=0, ARR=8499`), `OC2PE + ARPE + EGR=UG`. *(Moved off `PA0` to eliminate `ADC1_IN1` conflict, and avoids `PA5` LD2/SPI1 overlap!)* |
| **User Input** | `EXTI13` | **`PC13`** (`B1` Button) | Input (`00b`), No Pull | **Active-HIGH** per UM2505 (external pull-down `R31`) $\implies$ **Rising Edge (`EXTI->RTSR1`)** |
| **Telemetry UART** | `USART2` | **`PA2`** (TX), **`PA3`** (RX) | `AF7` (`10b`) | `115200 baud` (`BRR = 1476` @ 170 MHz), SPSC RX ring buffer + bounded TX timeout |
| **Health Watchdog** | `IWDG` | Internal (`32 kHz LSI`) | `PR=100b` (`/64`), `RLR=499` | `1.0 s` nominal reset window; `DBGMCU_APB1FZR1_DBG_IWDG_STOP` enabled for GDB |

> [!IMPORTANT]
> **Pin-Conflict & Polarity Fixes Applied:**
> 1. **`PA0` Conflict Resolved:** `PA0` is dedicated exclusively to `ADC1_IN1`. `TIM2` PWM output is assigned cleanly to **`PA1` (`TIM2_CH2`, `AF1`)** (or `PB10` `TIM2_CH3` `AF1` if `PA1` is reserved for `OPAMP1`/`COMP1`).
> 2. **Active-HIGH `PC13` (`B1`):** Configured for **Rising Edge (`EXTI->RTSR1`)** with `GPIOC->MODER[27:26] = 00b` and `PUPDR = 00b` per UM2505.
> 3. **Zero Blocking Debounce in ISR:** `EXTI15_10_IRQHandler` ONLY clears `EXTI->PR1`, sets a timestamp/event flag, and executes `__DSB()`; the 25 ms debounce FSM runs cleanly in the non-blocking main superloop!

---

## 3. Firmware Architecture

1. **Clock & Peripheral Startup (`main()`):**
   - Calls `Clock_Init_170MHz()` (Lab 03) **first** so SysTick, TIM2, USART2, and ADC1 all share exact `170 MHz` timing.
   - Initializes `SysTick` (`1 ms`, `LOAD = 169999`), `ADC1` (`42.5 MHz`), `TIM2_CH2` (`20 kHz` on `PA1`), `USART2` (`115200`), `EXTI13` (Rising Edge), and `IWDG` (`1.0 s`).
2. **Interrupt Service Routines (Minimal & Atomic):**
   - `SysTick_Handler`: Increments `volatile uint32_t g_systick_ms`.
   - `EXTI15_10_IRQHandler`: Clears `EXTI->PR1 = EXTI_PR1_PIF13;`, records `g_btn_edge_flag = 1U;`, executes `__DSB();`. (No blocking delays, no `ODR ^=` inside ISR!)
   - `USART2_IRQHandler`: Drains host commands into SPSC RX ring buffer (Lab 08).
3. **Non-Blocking Cooperative Superloop (Wraparound-Safe Tick Math):**
   - **Every 1 ms:** Polls the 4-state non-blocking button debouncer FSM (Lab 02) to toggle operating mode (`ACTIVE` $\leftrightarrow$ `STANDBY`).
   - **Every 50 ms:** Triggers calibrated single-shot `ADC1_IN1` conversion (bounded timeout), scales `raw (0..4095)` to `TIM2->CCR2 = (raw * 8499UL) / 4095UL` (or `0` in `STANDBY` mode).
   - **Every 500 ms:** Streams JSON telemetry over `USART2`:
     `{"tick_ms":12500,"adc_raw":2048,"voltage_mv":1650,"pwm_pct":50,"mode":"ACTIVE"}\r\n`
   - **Every loop pass (< 250 ms):** Refreshes `IWDG->KR = 0xAAAAU;` ONLY when all subsystems report healthy status.

---

## 4. Verification & Testing Checklist

- [ ] Verify `RCC->CFGR.SWS == PLL` (`170 MHz`) and `ADC12_COMMON->CCR.CKMODE == 11b` (`42.5 MHz`) in GDB.
- [ ] Probe `PA1` with oscilloscope: confirm exact `20.00 kHz` PWM scaling linearly `0%..100%` as `PA0` potentiometer rotates `0..3.3 V`.
- [ ] Press `B1` (`PC13`): confirm mode toggles cleanly once per press (`ACTIVE` $\leftrightarrow$ `STANDBY`) on rising edge with zero double-triggering.
- [ ] Connect host terminal at `115200 8N1` and verify valid JSON packets every `500 ms` with zero baud-rate corruption.
- [ ] Verify `IWDG` recovers automatically within `~1.0 s` if an artificial fault blocks the superloop.
