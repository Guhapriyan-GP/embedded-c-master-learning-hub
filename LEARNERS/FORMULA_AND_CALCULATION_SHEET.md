# Embedded C & STM32G474RE Formula and Calculation Sheet

Every formula on this desk sheet is verified against **ST RM0440**, **DS12288**, **UM2505**, **NXP UM10204**, **ARM DDI 0403E.e**, and **Liu & Layland (1973)** for the **STM32G474RET6 (NUCLEO-G474RE MB1367)**.

---

## 1. Digital Electronics, RC Settling & DS12288 GPIO Limits (`DIG-ELEC-*`, `HWSW-PCB-*`)

### Ohm's Law & LED Series Resistor
$$V = I \cdot R, \qquad P = V \cdot I = I^2 R, \qquad R_{\text{LED}} = \frac{V_{\text{DD}} - V_F}{I_{\text{LED}}}$$
- **Example (Green LED, $V_{\text{DD}} = 3.3\text{ V}$, $V_F = 2.0\text{ V}$, $I = 5\text{ mA}$):** $R_{\text{LED}} = \frac{3.3 - 2.0}{0.005} = 260\ \Omega \implies$ use standard **$270\ \Omega$** or **$330\ \Omega$**.

### RC Time Constant, Edge Rise Time & ADC Settling
$$\tau = R \cdot C, \qquad V(t) = V_{\text{final}}\left(1 - e^{-t/\tau}\right)$$
- **10% to 90% rise time:** $t_{r,10-90} = \ln(9)\,\tau \approx 2.197\,RC$
- **30% to 70% CMOS/I2C rise time:** $t_{r,30-70} = \ln\!\left(\frac{0.7}{0.3}\right)\tau \approx 0.8473\,RC$
- **12-Bit ADC $< 0.5\text{ LSB}$ settling (minimum $N$-bit settling $\ln(2^{N+1})$):**
  $$t_{S,\min} \ge (R_{\text{AIN}} + R_{\text{ADC}}) \cdot C_{\text{ADC}} \cdot \ln(2^{13}) \approx 9.011 \cdot (R_{\text{AIN}} + R_{\text{ADC}}) \cdot C_{\text{ADC}}$$

### STM32G474RE GPIO Current Limits (DS12288 Table 15)
| Parameter | Absolute Max | Engineering Rule |
|---|---|---|
| Per-pin source/sink ($I_{\text{IO}}$) | $\pm 20\text{ mA}$ | Design for $\le 8\text{ mA}$ to keep $V_{\text{OL}}/V_{\text{OH}}$ within CMOS spec |
| Total package supply ($\sum I_{\text{VDD}}$ / $\sum I_{\text{VSS}}$) | **$160\text{ mA}$** | Never exceed total across all active pins combined |
| `FT_f` Fast-Mode Plus I2C sink | $20\text{ mA}$ | Enabled via `SYSCFG->CFGR1` (`I2C_PBx_FMP` / `I2Cx_FMP`) |
| Max voltage on `FT` / `TT_a` pins | `FT`: $5.5\text{ V}$ / `TT_a`: $V_{\text{DD}} + 0.3\text{ V}$ | Never apply 5 V to `TT_a` pins (`PA0`, `PA1`, `PB0`, etc.)! |

### I2C Open-Drain Pull-Up Sizing (NXP UM10204)
$$R_{p,\min} = \frac{V_{\text{DD}} - V_{\text{OL},\max}}{I_{\text{OL}}}, \qquad R_{p,\max} = \frac{t_{r,\max}}{0.8473 \cdot C_b}$$
- **400 kHz Fast Mode ($V_{\text{DD}} = 3.3\text{ V}$, $V_{\text{OL}} = 0.4\text{ V}$, $I_{\text{OL}} = 3\text{ mA}$, $t_{r,\max} = 300\text{ ns}$, $C_b = 100\text{ pF}$):**
  $$R_{p,\min} = \frac{3.3 - 0.4}{0.003} = 967\ \Omega, \qquad R_{p,\max} = \frac{300 \times 10^{-9}}{0.8473 \times 100 \times 10^{-12}} = 3.54\text{ k}\Omega \implies \text{Choose } \mathbf{2.2\text{ k}\Omega\text{ or } 3.3\text{ k}\Omega}$$
- **Why internal ~40 kΩ fails:** $t_r \approx 0.8473 \times 40\text{ k}\Omega \times 100\text{ pF} = 3.39\ \mu\text{s} \gg 300\text{ ns}$ (11× too slow!).

---

## 2. Main PLL Clocks — 170 MHz & 160 MHz Boost Mode (`STM32-RCC-*`, `CLK-PLL-001`)

$$f_{\text{PLL-IN}} = \frac{f_{\text{SRC}}}{M}, \qquad f_{\text{VCO}} = f_{\text{PLL-IN}} \times N, \qquad f_{\text{SYSCLK}} = f_{\text{PLL-R}} = \frac{f_{\text{VCO}}}{R}$$
$$f_{\text{PLL-Q}} = \frac{f_{\text{VCO}}}{Q}, \qquad f_{\text{PLL-P}} = \frac{f_{\text{VCO}}}{P}$$

**Mandatory DS12288 / RM0440 Operating Bounds:**
- $f_{\text{PLL-IN}} \in [2.66,\ 16.0]\text{ MHz}$ ($M = \text{PLLM} + 1 \in [1..16]$)
- $f_{\text{VCO}} \in [96,\ 344]\text{ MHz}$ (Range 1) with $N \in [8..127]$
- **$f_{\text{SYSCLK}} > 150\text{ MHz}$ (both 160 MHz and 170 MHz):** MUST set `PWR->CR5.R1MODE = 0` (Range 1 Boost), `FLASH->ACR.LATENCY = 4WS`, and step `HPRE = /2` for $\ge 1\ \mu\text{s}$ (`RM0440 §6.1.5`)!

| Target `SYSCLK` | Source ($f_{\text{SRC}}$) | $M$ (`PLLM`) | $f_{\text{PLL-IN}}$ | $N$ (`PLLN`) | $f_{\text{VCO}}$ | $R$ (`PLLR`) | `PWR_CR5` / `FLASH` | Useful `PLL_Q` & `PLL_P` |
|---|---|---:|---:|---:|---:|---:|---|---|
| **170 MHz** | HSI16 (16 MHz) | `4` (`3`) | `4 MHz` | `85` | `340 MHz` | `2` (`0`) | Boost (`R1MODE=0`), `4WS`, `HPRE=/2` ramp | `Q=/8` (`42.5 MHz`), `P=/6` (`56.67 MHz`) |
| **170 MHz** | HSE X3 (24 MHz) | `6` (`5`) | `4 MHz` | `85` | `340 MHz` | `2` (`0`) | Boost (`R1MODE=0`), `4WS`, `HPRE=/2` ramp | `Q=/8` (`42.5 MHz`), `P=/6` (`56.67 MHz`) |
| **160 MHz** | HSI16 (16 MHz) | `4` (`3`) | `4 MHz` | `80` | `320 MHz` | `2` (`0`) | Boost (`R1MODE=0`), `4WS`, `HPRE=/2` ramp | `Q=/8` (`40.0 MHz`), `P=/8` (`40.0 MHz`) |
| **160 MHz** | HSE X3 (24 MHz) | `6` (`5`) | `4 MHz` | `80` | `320 MHz` | `2` (`0`) | Boost (`R1MODE=0`), `4WS`, `HPRE=/2` ramp | `Q=/8` (`40.0 MHz`), `P=/8` (`40.0 MHz`) |
| **150 MHz** | HSI16 (16 MHz) | `4` (`3`) | `4 MHz` | `75` | `300 MHz` | `2` (`0`) | Normal (`R1MODE=1`), `4WS` | `Q=/6` (`50.0 MHz`), `P=/6` (`50.0 MHz`) |

---

## 3. Timers, PWM & `BDTR.DTG` 4-Range Dead-Time (`PERIPH-TIM-*`, `PERIPH-BDTR-001`)

### Timer Clock Rule
$$f_{\text{TIM-CLK}} = \begin{cases} f_{\text{PCLK}x} & \text{if } \text{PPRE}x = /1 \\ 2 \times f_{\text{PCLK}x} & \text{if } \text{PPRE}x > /1 \end{cases}$$

### Update Frequency, Edge-Aligned PWM & Center-Aligned PWM
$$f_{\text{update}} = f_{\text{PWM,edge}} = \frac{f_{\text{TIM-CLK}}}{(\text{PSC} + 1) \cdot (\text{ARR} + 1)}, \qquad f_{\text{PWM,center}} = \frac{f_{\text{TIM-CLK}}}{2 \cdot (\text{PSC} + 1) \cdot \text{ARR}}$$
$$\text{Duty Cycle (\%)} = \frac{\text{CCR}x}{\text{ARR} + 1} \times 100\%, \qquad \text{PWM Resolution (bits)} = \log_2(\text{ARR} + 1)$$
- **Example (25 kHz edge-aligned PWM @ 170 MHz, `PSC = 0`):** $\text{ARR} = \frac{170{,}000{,}000}{25{,}000} - 1 = 6{,}799$ ($\approx 12.7\text{ bits}$ resolution).

### Advanced Timer Dead-Time (`TIMx->BDTR.DTG[7:0]`, RM0440 §28.6.20)
Let $t_{\text{DTS}} = \frac{2^{\text{CKD}}}{f_{\text{TIM-CLK}}}$ (when `CR1.CKD = 00b` at $170\text{ MHz}$, $t_{\text{DTS}} = 5.882\text{ ns}$):

| `DTG[7:5]` Bit Pattern | Decimal `DTG` | Formula for Dead-Time $T_{\text{DT}}$ | Step @ 170 MHz | Range @ 170 MHz |
|---|---|---|---|---|
| `0xx` (`DTG[7] = 0`) | `0 .. 127` | $T_{\text{DT}} = \text{DTG}[6:0] \times t_{\text{DTS}}$ | $5.88\text{ ns}$ | $0\text{ ns} .. 747\text{ ns}$ |
| `10x` (`DTG[7:6] = 10`) | `128 .. 191` | $T_{\text{DT}} = (64 + \text{DTG}[5:0]) \times 2\,t_{\text{DTS}}$ | $11.76\text{ ns}$ | $753\text{ ns} .. 1.49\ \mu\text{s}$ |
| `110` (`DTG[7:5] = 110`) | `192 .. 223` | $T_{\text{DT}} = (32 + \text{DTG}[4:0]) \times 8\,t_{\text{DTS}}$ | $47.06\text{ ns}$ | $1.51\ \mu\text{s} .. 2.96\ \mu\text{s}$ |
| `111` (`DTG[7:5] = 111`) | `224 .. 255` | $T_{\text{DT}} = (32 + \text{DTG}[4:0]) \times 16\,t_{\text{DTS}}$ | $94.12\text{ ns}$ | $3.01\ \mu\text{s} .. 5.93\ \mu\text{s}$ |

---

## 4. Communication Buses (`USART`, `LPUART`, `I2C TIMINGR`)

### USART1–3 / UART4–5 (`OVER8 = 0`, 16× Oversampling)
$$\text{BRR} = \text{round}\!\left(\frac{f_{\text{KER}} / \text{PRESC}}{\text{Baud}}\right), \qquad \text{Error (\%)} = \frac{\text{Baud}_{\text{actual}} - \text{Baud}_{\text{target}}}{\text{Baud}_{\text{target}}} \times 100\%$$
- At $170\text{ MHz}$, `PRESC=/1`, `115200 bps`: $\text{BRR} = \text{round}(170{,}000{,}000 / 115{,}200) = \mathbf{1476}$ (`0x05C4`, $-0.02\%$ error).
- At $160\text{ MHz}$, `PRESC=/1`, `115200 bps`: $\text{BRR} = \text{round}(160{,}000{,}000 / 115{,}200) = \mathbf{1389}$ (`0x056D`, $-0.01\%$ error).
- At $16\text{ MHz}$ HSI16, `115200 bps`: $\text{BRR} = \text{round}(16{,}000{,}000 / 115{,}200) = \mathbf{139}$ (`0x008B`, $-0.08\%$ error).

### LPUART1 (20-Bit Fractional `BRR`)
Valid when $3 \cdot \text{Baud} \le f_{\text{KER,presc}} \le 4096 \cdot \text{Baud}$:
$$\text{BRR}_{\text{LPUART}} = \text{round}\!\left(\frac{256 \times f_{\text{KER,presc}}}{\text{Baud}}\right)$$

### I2C `TIMINGR` Timing Equations (RM0440 §39.4.9)
$$t_{\text{PRESC}} = (\text{PRESC} + 1) \cdot t_{\text{I2CCLK}}$$
$$t_{\text{SCLL}} = (\text{SCLL} + 1) \cdot t_{\text{PRESC}}, \qquad t_{\text{SCLH}} = (\text{SCLH} + 1) \cdot t_{\text{PRESC}}$$
$$t_{\text{SCLDEL}} = (\text{SCLDEL} + 1) \cdot t_{\text{PRESC}}, \qquad t_{\text{SDADEL}} = \text{SDADEL} \cdot t_{\text{PRESC}}$$
$$f_{\text{SCL}} \approx \frac{1}{t_{\text{SCLL}} + t_{\text{SCLH}} + t_{\text{SYNC1}} + t_{\text{SYNC2}}}$$
- **Verified `Lab 12` & `i2c_g4.h` 400 kHz Fast Mode @ $f_{\text{I2CCLK}} = 170\text{ MHz}$:** `TIMINGR = 0x70811021` (`PRESC=7`, `SCLDEL=8`, `SDADEL=1`, `SCLH=0x10`, `SCLL=0x21` $\implies$ $f_{\text{SCL(max)}} = 396.3\text{ kHz} \le 400\text{ kHz}$, $t_{\text{SCLDEL}} = 423.5\text{ ns} \ge 400\text{ ns}$).
- **Verified `i2c_g4.h` 400 kHz Fast Mode @ $f_{\text{I2CCLK}} = 160\text{ MHz}$:** `TIMINGR = 0x70710F1F` (`PRESC=7`, `SCLDEL=7`, `SDADEL=1`, `SCLH=0x0F`, `SCLL=0x1F` $\implies$ $f_{\text{SCL(max)}} = 396.0\text{ kHz} \le 400\text{ kHz}$, $t_{\text{SCLDEL}} = 400.0\text{ ns}$).
- **Verified `i2c_g4.h` 100 kHz Standard Mode @ $f_{\text{I2CCLK}} = 16\text{ MHz}$ (HSI16, RM0440 Table 384):** `TIMINGR = 0x30420F13` (`PRESC=3`, `SCLDEL=4`, `SDADEL=2`, `SCLH=0x0F`, `SCLL=0x13`).

---

## 5. Mixed-Signal Analog (`ADC` Calibration, Temp Sensor & `DAC`)

### ADC Conversion Time & Clock Compliance (`DS12288`)
$$f_{\text{ADC}} \le 60\text{ MHz}\text{ (single)},\quad \le 52\text{ MHz}\text{ (multi)}, \qquad t_{\text{CONV}} = (t_{\text{SMP}} + 12.5) \cdot \frac{1}{f_{\text{ADC}}}\text{ (12-bit)}$$
- At $170\text{ MHz}$ HCLK: use **`CKMODE = 11b` (`HCLK/4 = 42.5 MHz`)**. Internal temp sensor (`ADC1_IN16`) requires $t_S \ge 5\ \mu\text{s} \implies 5\ \mu\text{s} \times 42.5\text{ MHz} = 212.5\text{ cycles} \implies$ select **`247.5`** (`110b`) or **`640.5`** (`111b`) cycles!

### ADC Voltage, `VREFINT_CAL` & Factory Die Temperature (`TS_CAL1`/`TS_CAL2`)
Factory calibration constants in STM32G474RE System ROM (measured at $V_{\text{DDA}} = 3.0\text{ V}$):
- **`VREFINT_CAL`** (`uint16_t` @ `0x1FFF75AA`), **`TS_CAL1`** (30 °C @ `0x1FFF75A8`), **`TS_CAL2`** (130 °C @ `0x1FFF75CA`)

$$V_{\text{DDA}} = 3.0\text{ V} \times \frac{\text{VREFINT-CAL}}{\text{ADC}_{\text{VREFINT}}}, \qquad V_{\text{IN}} = V_{\text{DDA}} \times \frac{\text{ADC}_{\text{RAW}}}{4095}$$
$$T_{\text{die}}(^\circ\text{C}) = \frac{130 - 30}{\text{TS-CAL2} - \text{TS-CAL1}} \times \left(\text{ADC}_{\text{TEMP}} \cdot \frac{V_{\text{DDA}}}{3.0\text{ V}} - \text{TS-CAL1}\right) + 30$$

### 12-Bit DAC Output Voltage
$$V_{\text{OUT}} = V_{\text{REF+}} \times \frac{\text{DOR}}{4096} \quad (\text{or } \approx V_{\text{REF+}} \cdot \frac{\text{DHR12R}}{4095}\text{ full-scale code } 4095)$$

---

## 6. Watchdogs (`IWDG` & `WWDG`)

### Independent Watchdog (`IWDG`, $f_{\text{LSI}} \approx 32\text{ kHz}$)
$$T_{\text{IWDG}} = \frac{(\text{RLR} + 1) \times \text{Prescaler}}{f_{\text{LSI}}}, \qquad \text{Prescaler} = 4 \times 2^{\text{PR}}\quad (\text{PR}=0..6 \implies /4../256)$$
- **1.0 s timeout (`PR = 100b = /64`, `RLR = 499`):** $T_{\text{IWDG}} = \frac{500 \times 64}{32{,}000} = 1.000\text{ s}$. *(Account for LSI $\pm 5\%$ tolerance in safety margins!)*

### Window Watchdog (`WWDG`, STM32G4 3-bit `WDGTB = 0..7` → `/1../128`)
$$T_{\text{WWDG,max}} = \frac{4096 \times 2^{\text{WDGTB}} \times (T[5:0] + 1)}{f_{\text{PCLK1}}}, \qquad T_{\text{window,min}} = \frac{4096 \times 2^{\text{WDGTB}} \times (T[6:0] - W[6:0])}{f_{\text{PCLK1}}}$$
- Refresh is **only legal** when $0x40 \le \text{CNT} \le W[6:0]$ (too early or $< 0x40$ triggers MCU reset!).

---

## 7. Cortex-M4 Bit-Banding & RTOS RMS Schedulability

### ARM Cortex-M4 Bit-Band Alias Address Formula (`ARM DDI 0403E.e`)
Valid **only** for SRAM `0x20000000–0x200FFFFF` (alias `0x22000000`) and Peripheral `0x40000000–0x400FFFFF` (alias `0x42000000`):
$$\text{alias addr} = \text{alias base} + (\text{byte offset} \times 32) + (\text{bit num} \times 4)$$
> **STM32G474RE Warning:** `GPIOA–G` (`0x48000000`) and `ADC`/`DAC` (`0x50000000`) are **OUTSIDE** the `0x40000000–0x400FFFFF` bit-band region! Always use `GPIOx->BSRR` / `BRR` for atomic GPIO operations.

### Rate Monotonic Scheduling (RMS) Utilization Bound (Liu & Layland)
For $n$ independent periodic tasks with worst-case execution time $C_i$ and period $T_i$ (deadlines $D_i = T_i$, fixed priorities sorted by shortest period):
$$U = \sum_{i=1}^{n} \frac{C_i}{T_i} \le U_{\text{lub}}(n) = n\left(2^{1/n} - 1\right)$$
- $n=1 \implies 100\%$ | $n=2 \implies 82.8\%$ | $n=3 \implies 78.0\%$ | $n=4 \implies 75.7\%$ | $n \to \infty \implies \ln(2) \approx \mathbf{69.3\%}$.
- If $U > U_{\text{lub}}(n)$ (but $U \le 1.0$), run exact **Response Time Analysis (RTA)**: $R_i^{(k+1)} = C_i + \sum_{j \in hp(i)} \left\lceil \frac{R_i^{(k)}}{T_j} \right\rceil C_j \le D_i$.
