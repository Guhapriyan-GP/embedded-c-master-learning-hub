# Stage 01b: Digital Electronics, NUCLEO-G474RE Hardware Integration & Lab Equipment Mastery

> **Sequential Navigation (Step 1 of 17 — Canonical Acyclic Order):**  
> **Home:** [`START_HERE.md`](../START_HERE.md) | **Roadmap:** [`learning-sequence.md`](../01-master-concept-map/learning-sequence.md) | **Prev:** [`START_HERE.md`](../START_HERE.md) *(Entry Point)* | **Next:** [Stage 02 — C Language Foundations (`02-c-fundamentals/`)](../02-c-fundamentals/README.md)  
> **Prerequisites:** None *(Beginner Entry Point — read [`START_HERE.md`](../START_HERE.md) first)*

> **Curriculum Track:** Level 1 — Essential Hardware Bridge (Prerequisite before Stage 06–08 Hardware Labs)  
> **Target Silicon:** STMicroelectronics **STM32G474RET6** (LQFP64, ARM Cortex-M4F @ 170 MHz)  
> **Target Board:** **NUCLEO-G474RE** (PCB **MB1367**, ST User Manual **UM2505**)  
> **Code Verification Policy:** All C snippets in this module are **Statically Reviewed (Not Tested on Physical Hardware)** unless explicitly tagged **Compile-Verified (`arm-none-eabi-gcc 14.3.1`)**.

---

## 1. Learning Objectives

Upon completing Stage 01b, you will be able to:

| Concept ID | Level | Scope Layer | Engineering Objective |
|---|---|---|---|
| `DIG-ELEC-001` | Level 1 (Essential) | `[General EE / STM32G474RE]` | Apply Ohm's Law, Joule power dissipation, voltage dividers, LED current-limiting resistor sizing, and RC charging/settling mathematics ($\tau = RC$, $t_{\text{rise}} \approx 2.2\,RC$, $5\tau$) to GPIO debouncing and SAR ADC input impedance. |
| `DIG-ELEC-002` | Level 1 (Essential) | `[STM32G474RE Exact]` | Calculate CMOS logic thresholds ($V_{IH}, V_{IL}, V_{OH}, V_{OL}$) and noise margins, and safely interface 5V peripherals using STM32G474RE 5V-tolerant (`FT`, `FT_f`, `FT_c`, `FT_a`) vs 3.3V-only (`TT_a`) pin structures (DS12288). |
| `DIG-ELEC-003` | Level 1 (Essential) | `[STM32G474RE Exact]` | Enforce **DS12288 Table 15** absolute maximum and recommended GPIO/supply current limits ($I_{IO} \le \pm 20\text{ mA}$, $\sum I_{VDD}/\sum I_{VSS} \le 160\text{ mA}$) and design logic-level N-MOSFET low-side drivers for high-current loads. |
| `DIG-ELEC-004` | Level 2 (Intermediate) | `[General EE / STM32G474RE]` | Derive I2C pull-up resistor bounds ($R_{p,\min}$ and $R_{p,\max}$) per **NXP UM10204** for Standard Mode (100 kHz) and Fast Mode (400 kHz), proving why internal ~40 kΩ pull-ups fail high-speed I2C. |
| `DIG-ELEC-005` | Level 2 (Intermediate) | `[General EE / STM32G474RE]` | Design power-integrity decoupling networks (100 nF ceramic per $V_{DD}$ + bulk capacitance + $V_{DDA}$ filtering per **ST AN5093**), ESD/TVS clamping, and inductive flyback diodes ($V = L \frac{di}{dt}$). |
| `DIG-ELEC-006` | Level 2 (Intermediate) | `[STM32 Generic / STM32G474RE]` | Mitigate ringing, ground bounce, and EMI on fast digital traces using `GPIOx->OSPEEDR` slew-rate control, series damping resistors, and short ground return paths. |
| `HWSW-PCB-001` | Level 1 (Essential) | `[STM32G474RE Exact]` | Read the **NUCLEO-G474RE (MB1367, UM2505)** schematic, distinguish **`JP6` (IDD)** from **`JP5` (5V_SEL)**, handle **B1 (`PC13`) Active-HIGH**, **LD2 (`PA5`)**, **`PB8-BOOT0` 10 kΩ pull-down**, and **`PA9/PA10` UCPD dead-battery 5.1 kΩ pull-downs**. |
| `HWSW-PCB-002` | Level 1 (Essential) | `[STM32G474RE Exact]` | Resolve pin-multiplexing conflicts across ST Morpho (`CN7`/`CN10`) and Arduino Uno V3 (`CN5`/`CN6`/`CN8`/`CN9`) connectors using DS12288 Alternate Function tables and UM2505 solder-bridge tables. |
| `HWSW-LAB-001` | Level 1 (Essential) | `[General Lab / STM32G474RE]` | Operate an oscilloscope safely and accurately: 1x vs 10x probe attenuation and loading, probe compensation, bandwidth-rise-time rule ($f_{BW} \ge 0.35 / t_r$), ground-spring usage, edge/protocol triggering. |
| `HWSW-LAB-002` | Level 1 (Essential) | `[General Lab]` | Capture and decode SPI, I2C, and UART buses on a logic analyzer (Saleae Logic 2 or PulseView/sigrok) applying $4\times\text{--}10\times$ oversampling and correct 3.3V CMOS thresholds. |
| `HWSW-LAB-003` | Level 1 (Essential) | `[General Lab / STM32G474RE]` | Execute safe board bring-up and electrical measurements using a Digital Multimeter (continuity, DC voltage, current burden voltage, fuse protection) and a CC/CV current-limited Bench Power Supply. |
| `HWSW-ANA-001` | Level 2 (Intermediate) | `[General EE / STM32G474RE]` | Design low-noise analog power rails (`VDDA`/`VREF+` ferrite-bead filtering, multi-decade ceramic capacitor ESR/ESL & DC-bias derating, continuous vs star ground return paths) and audit NUCLEO-G474RE (MB1367, UM2505) `VDDA`/`VREF+` bridges (`SB8`/`SB10`, `SB14`/`R33`/`R34`) before enabling internal `VREFBUF`. |

---

## 2. Prerequisites

- Basic high-school algebra, scientific/engineering prefixes ($\text{m}, \mu, \text{n}, \text{p}, \text{k}, \text{M}$), and binary/hexadecimal number systems.
- Recommended concurrent reading: [Stage 02 — C Fundamentals](../02-c-fundamentals/README.md) (`C-INT-001`, `C-BIT-001`).
- **Required before:** [Stage 06 — STM32 Platform Architecture](../06-stm32-fundamentals/README.md), [Stage 08 — GPIO](../08-gpio/README.md), and all physical hardware labs in [18-practical-laboratory/](../18-practical-laboratory/README.md).

---

## 3. Detailed Technical Theory

Embedded firmware does not execute in an abstract mathematical sandbox; every bit written to a memory-mapped register physically drives MOSFET gates, charges trace capacitance, sinks or sources real milliamperes through silicon bond wires, and interacts with external passive and active components.

### 3.1 DC Circuit Foundations & RC Dynamics (`DIG-ELEC-001`)

1. **Ohm's Law & Power Dissipation:**
   Every GPIO pin, pull-up resistor, shunt resistor, and LED series resistor obeys Ohm's Law ($V = I R$) and Joule's Power Law ($P = V I = I^2 R = V^2 / R$). Always verify resistor package power ratings (e.g., standard 0603 SMD = $0.1\text{ W}$, 1/4 W through-hole = $0.25\text{ W}$) with at least a $2\times$ thermal derating margin.
2. **Voltage Dividers & Source Impedance:**
   A resistive divider ($R_1$ top, $R_2$ bottom) produces open-circuit voltage $V_{\text{out}} = V_{\text{in}} \frac{R_2}{R_1 + R_2}$ with **Thévenin equivalent output impedance** $R_{\text{th}} = R_1 \parallel R_2 = \frac{R_1 R_2}{R_1 + R_2}$. When feeding an STM32G474RE SAR ADC input, $R_{\text{th}}$ adds directly to the internal sampling switch resistance $R_{\text{ADC}}$ and determines how long the internal sampling capacitor $C_{\text{ADC}}$ needs to charge to within $\frac{1}{2}\text{ LSB}$ (12-bit accuracy requires $\ge 9.02\,\tau$).
3. **RC Transient Response ($\tau = RC$):**
   Every PCB trace, jumper wire, probe tip, and MOSFET gate presents parasitic capacitance $C$. Driving that capacitance through resistance $R$ produces an exponential transition:
   - After $1\tau$: $63.2\%$ of final voltage
   - 10%–90% rise time: $t_{\text{rise}} = \ln(9)\,RC \approx 2.2\,RC$
   - After $3\tau$: $95.0\%$ of final voltage
   - After $5\tau$: $99.3\%$ of final voltage (considered fully settled for digital logic)
   - For a 12-bit ADC ($\frac{1}{2}\text{ LSB} = 2^{-13} \approx 0.0122\%$ error): $t_{\text{settle}} \ge \ln(2^{13})\,RC \approx 9.01\,RC$.

### 3.2 CMOS Logic Thresholds, Noise Margins & STM32G474RE Pin Types (`DIG-ELEC-002`)

CMOS inputs interpret continuous analog voltages as digital `0` or `1` using threshold specifications (with Schmitt-trigger hysteresis $V_{\text{hys}} \approx 0.1\,V_{DD}$ on STM32G4 digital inputs to reject slow-edge noise):

| Parameter | Meaning | STM32G474RE Typical/Guaranteed @ $V_{DD} = 3.3\text{ V}$ (DS12288 §5.3.14) |
|---|---|---|
| $V_{IL(\max)}$ | Maximum voltage guaranteed read as logic `0` | $\le 0.3\,V_{DD} = 0.99\text{ V}$ (FT/TT I/Os) |
| $V_{IH(\min)}$ | Minimum voltage guaranteed read as logic `1` | $\ge 0.7\,V_{DD} = 2.31\text{ V}$ (FT/TT I/Os) |
| $V_{OL(\max)}$ | Maximum output voltage when driving logic `0` | $\le 0.40\text{ V}$ (at $I_{IO} = +8\text{ mA}$ sink, standard CMOS) |
| $V_{OH(\min)}$ | Minimum output voltage when driving logic `1` | $\ge V_{DD} - 0.40\text{ V} = 2.90\text{ V}$ (at $I_{IO} = -8\text{ mA}$ source) |
| Indeterminate Zone | Forbidden steady-state zone ($V_{IL} < V < V_{IH}$) | $0.99\text{ V}$ to $2.31\text{ V}$ — causes shoot-through current in both P-MOS & N-MOS input inverters! |

**STM32G474RE Pin Structure Taxonomy (DS12288 Table 12 & Table 14):**

| Pin Suffix | Name | Max Safe Input Voltage (Powered, $V_{DD} = 3.3\text{ V}$) | Critical Electrical Caveat |
|---|---|---|---|
| `FT` | Five-volt Tolerant | $\min(V_{DD} + 4.0\text{ V},\ 5.5\text{ V}) = 5.5\text{ V}$ | **NOT 5V-tolerant when $V_{DD} = 0\text{ V}$!** Max pin voltage is $V_{DD} + 4.0\text{ V}$ (or $\le 3.6\text{ V}$ when unpowered — check DS12288 Table 14). Applying 5V to an unpowered MCU back-powers silicon through ESD clamps and can destroy the chip. |
| `FT_f` | 5V-Tolerant, I2C Fast-Mode Plus | $5.5\text{ V}$ | Supports 20 mA sink in I2C FM+ mode when enabled via `SYSCFG->CFGR1` (`I2C_PBx_FMP`). |
| `FT_a` | 5V-Tolerant with Analog Switch | $5.5\text{ V}$ (digital mode) / $\le V_{DDA} + 0.3\text{ V}$ (analog mode) | **Once analog mode (`MODER = 11` / ADC/OPAMP/COMP switch closed) is active**, input voltage MUST NOT exceed $V_{DDA}$ (3.3 V)! |
| `FT_c` | 5V-Tolerant, USB Type-C PD (`UCPD`) | $5.5\text{ V}$ | Pins `PA9`/`PA10` (and `PB4`/`PB6` on certain packages/CC lines) have internal **5.1 kΩ UCPD dead-battery pull-down resistors enabled by default at reset** (`PWR->CR3.UCPD_DBDIS = 0`). |
| `FT_u` | 5V-Tolerant, USB FS (`PA11`/`PA12`) | $5.5\text{ V}$ | Dedicated USB D-/D+ transceiver pads. |
| `TT_a` | Three-volt Tolerant with Analog | $\le \min(V_{DD}, V_{DDA}) + 0.3\text{ V} \approx 3.6\text{ V}$ | **NEVER apply 5 V!** Examples on STM32G474RE LQFP64 include `PA4` (`DAC1_OUT1`), `PA5` (`DAC1_OUT2` / `LD2`), `PA6` (`DAC2_OUT1`), `PA0`–`PA3` (verify every pin in DS12288 Table 12 before wiring!). |

### 3.3 STM32G474RE Current Limits & External Load Switching (`DIG-ELEC-003`)

Per **STM32G474RE Datasheet DS12288 Table 15 (Current Characteristics)**:

| Symbol | Parameter | Absolute Maximum Limit | Engineering Design Rule |
|---|---|---|---|
| $I_{IO(\text{PIN})}$ | Output current sunk or sourced by **any single I/O pin** | $\pm 20\text{ mA}$ | Design for **$\le 8\text{ mA}$** continuous per pin (guaranteed $V_{OL} \le 0.4\text{ V}$ / $V_{OH} \ge V_{DD}-0.4\text{ V}$). Never drive relays, motors, or high-power LEDs directly. |
| $I_{VDD(\text{PIN})}$ / $I_{VSS(\text{PIN})}$ | Current through a single $V_{DD}$ or $V_{SS}$ power pin | $100\text{ mA}$ | Includes CPU core (~25–35 mA @ 170 MHz) + peripherals + all GPIO loads. |
| $\sum I_{VDD}$ / $\sum I_{VSS}$ | **Total** current into all $V_{DD}$ / out of all $V_{SS}$ lines | **$160\text{ mA}$** | Even if each pin stays under 15 mA, driving 12 pins × 15 mA = 180 mA **exceeds $\sum I_{VDD} = 160\text{ mA}$** and risks fusing internal bond wires! |
| $I_{\text{INJ}(\text{PIN})}$ | Injected current on `FT`/`TT` pins | $-5\text{ mA} / +0\text{ mA}$ (`TT_a` positive injection forbidden) | Never forward-bias internal ESD clamping diodes as a cheap "level shifter." |

**Low-Side N-Channel MOSFET Driver (Safe High-Current Switching):**
To switch a 5V/12V load (e.g., 500 mA solenoid, relay, LED strip) from a 3.3V GPIO:
1. Choose a **logic-level** N-MOSFET with $V_{GS(\text{th})} \le 1.5\text{ V}$ and rated $R_{DS(\text{on})}$ specified at $V_{GS} = 2.5\text{ V}$ or $3.3\text{ V}$ (e.g., `BSS138` / `2N7002` for $\le 200\text{ mA}$; `AO3400A` / `IRLML2502` for 1–4 A).
2. Add a **series gate resistor** $R_G \approx 100\text{--}220\ \Omega$ to limit peak capacitive gate-charging current ($I_{\text{peak}} \le 3.3\text{ V} / 220\ \Omega = 15\text{ mA} < 20\text{ mA}$) and damp gate ringing.
3. Add a **gate pull-down resistor** $R_{PD} = 10\text{--}100\text{ k}\Omega$ from Gate to GND so the MOSFET stays safely **OFF** during MCU reset when STM32G4 GPIO pins default to Hi-Z **Analog mode (`MODER = 11b`)**!

### 3.4 I2C Open-Drain Pull-Up Sizing (`DIG-ELEC-004`)

I2C (`SDA`, `SCL`) is an **open-drain** bus: devices only pull the line LOW (`N-MOS` ON) and release the line (`Hi-Z`) so external pull-up resistors $R_p$ charge the total bus capacitance $C_b$ (PCB traces + connector + IC pin capacitances, typically $30\text{--}200\text{ pF}$ on a breadboard/jumper setup) back to $V_{DD}$.

- **Why $R_p$ cannot be too small ($R_{p,\min}$):** If $R_p$ is too low, the N-MOS pull-down transistor must sink excessive current, raising $V_{OL}$ above the NXP UM10204 maximum of $0.4\text{ V}$ (at $I_{OL} = 3\text{ mA}$ for Standard/Fast Mode) and wasting power.
- **Why $R_p$ cannot be too large ($R_{p,\max}$):** If $R_p$ is too high (such as STM32's internal **~40 kΩ** pull-ups!), the RC rise time $t_r = 0.8473 \cdot R_p \cdot C_b$ (measuring $0.3\,V_{DD}$ to $0.7\,V_{DD}$, since $\ln(0.7/0.3) = \ln(7/3) \approx 0.8473$) becomes so slow that edges fail the I2C rise-time limit ($t_{r,\max} = 1000\text{ ns}$ for 100 kHz Standard Mode; **$t_{r,\max} = 300\text{ ns}$ for 400 kHz Fast Mode**), causing missed clock pulses and corrupted data!

### 3.5 Power Integrity, Decoupling, ESD & Inductive Flyback Protection (`DIG-ELEC-005`)

1. **Decoupling Capacitors (ST AN5093):**
   At 170 MHz, digital switching creates nanosecond current spikes ($di/dt$). PCB trace inductance blocks the distant voltage regulator from responding instantaneously. Per **ST AN5093** (*Getting started with STM32G4 Series hardware development*):
   - Place **one 100 nF (0.1 µF) X7R/X5R ceramic capacitor** as close as physically possible to **every** $V_{DD}/V_{SS}$ pin pair (low ESL high-frequency local charge reservoir).
   - Place **one 4.7 µF bulk ceramic capacitor** on the $V_{DD}$ rail near the MCU.
   - Filter **$V_{DDA}$** (analog supply for ADC1–5, DAC1–4, OPAMP1–6, COMP1–7) with **100 nF + 1 µF** ceramic capacitors (and optional ferrite bead) to keep 170 MHz digital switching noise out of 12-bit analog conversions.
2. **Inductive Kickback & Flyback Diodes:**
   When turning OFF current through an inductive load (relay coil, DC motor, solenoid), the collapsing magnetic field forces an opposing voltage spike $V = L \frac{di}{dt}$ that easily reaches **50–300 V**, instantly avalanching and destroying MOSFETs and MCUs. Always place a **reverse-biased flyback (freewheeling) diode** (e.g., `1N5819` Schottky or `1N4007` rectifier) directly across the inductive coil terminals (cathode to $+V_{\text{supply}}$, anode to MOSFET drain) to circulate the coil current safely.
3. **ESD & TVS Protection:**
   Human Body Model (HBM) electrostatic discharges reach 2–15 kV. External-facing connectors (CAN, USB, UART, external buttons) require low-capacitance **TVS diode arrays** (e.g., `USBLC6-2SC6` for USB FS, `PESD1CAN` for FDCAN) placed directly at the board entry connector, before any series resistors.

#### 3.5.1 Analog Power-Rail Integrity: `VDDA`/`VREF+` Ferrite Beads, Capacitor ESR & Ground Return Paths (`HWSW-ANA-001`)

A 12-bit SAR ADC on STM32G474RE has an LSB step size of only **$805.7\ \mu\text{V}$** at $V_{\text{REF+}} = 3.30\text{ V}$ (or **$610.4\ \mu\text{V}$** at $V_{\text{REF+}} = 2.50\text{ V}$). Unfiltered 170 MHz digital switching ripple ($30\text{--}100\text{ mV}_{\text{pp}}$ on `VDD`) coupled into `VDDA` or `VREF+` directly degrades Effective Number of Bits (**ENOB**) by 2–4 bits! Per **ST AN5093** (*Getting Started with STM32G4 Hardware Dev*), **ST AN2834** (*How to Get the Best ADC Accuracy*), and **ST DS12288 §5.3.1 & §5.3.21**:

1. **Ferrite-Bead Pi/LC Filtering on `VDDA` (LQFP64 Pin 14) & `VREF+` (LQFP64 Pin 13):**
   - Isolate `VDDA` from noisy digital `VDD` using a series **ferrite bead** ($Z \approx 600\ \Omega\text{ at }100\text{ MHz}$, low DC resistance $R_{\text{DC}} < 0.3\ \Omega$) paired with local ceramic decoupling (**`10 nF` or `100 nF` X7R/C0G + `1 µF` X7R**) placed within `< 3 mm` of `VDDA` (Pin 14) and `VSSA` (Pin 12).
   - **Mandatory DS12288 Power-Sequencing Constraint:** Never use a high-resistance RC filter on `VDDA` — the DC drop across the bead must keep $|V_{\text{DD}} - V_{\text{DDA}}| \le 300\text{ mV}$ at all times during power-up, operation, and power-down, and $V_{\text{REF+}} \le V_{\text{DDA}}$!
2. **Decoupling Capacitor ESR, ESL & Ceramic DC-Bias Derating:**
   - **Self-Resonant Frequency (SRF) & ESL:** Smaller packages (`0402`/`0603`) have lower parasitic inductance (ESL $\approx 0.4\text{--}0.8\text{ nH}$), pushing SRF higher to shunt 170 MHz harmonics. Pair a small value (`10–100 nF`) for HF shunting with a larger bulk capacitor (`1 µF`–`4.7 µF`) for mid-band impedance.
   - **`VREF+` / `VREFBUF` Stability Window (DS12288 §5.3.21):** When driving `VREF+` from the internal **`VREFBUF`** (2.048 V / 2.500 V / 2.900 V), STM32G4 requires **`100 nF || 1 µF`** (typical $1.1\ \mu\text{F}$ total, $0.5\text{--}1.5\ \mu\text{F}$ effective capacitance) with total **ESR between $0.01\ \Omega$ and $1\ \Omega$**! Using tiny 0402 6.3 V Y5V ceramics can lose $> 60\%$ capacitance under 2.5 V DC bias, starving phase margin and causing `VREFBUF` oscillation! Always use **X7R or C0G/NP0 rated $\ge 10\text{--}16\text{ V}$**.
3. **Ground Return Paths (Continuous Ground Plane vs Star Ground):**
   - High-frequency digital return currents (> 1 MHz) follow the path of **least inductance** directly beneath their signal trace on the ground plane, whereas DC/low-frequency analog return currents follow the path of **least resistance**.
   - **Anti-Pattern (Naive Split Ground Planes):** Cutting a slot between "AGND" and "DGND" under the MCU frequently **worsens** noise and creates a radiating slot antenna if *any* trace crosses the gap!
   - **Best Practice (Henry Ott / ST AN2834):** Use a **single solid, continuous ground plane** and **spatially partition** components — place analog sensors, op-amps, and `VSSA` decoupling on one side of the MCU and keep high-$di/dt$ switching regulators, MOSFET half-bridges, and `OSPEEDR = 11` buses physically confined to the opposite side so digital return currents never traverse the analog zone. Where two planes are mandated by a mixed-signal isolation barrier, tie them at a **single star point directly beneath `VSSA`/`VSS`**.
4. **NUCLEO-G474RE (MB1367, UM2505) `VDDA` & `VREF+` Jumpers:**
   - **`L1`/`L2` Ferrite Beads (`SB8`/`SB10`):** On MB1367, **`SB8`** and **`SB10`** are **OFF (open)** by default so onboard SMD ferrite beads **`L1`** (`VDDA`) and **`L2`** (`VREF+`) actively filter the analog rails. Closing `SB8`/`SB10` bypasses `L1`/`L2` (only for specific external Arduino `AVDD` supply setups per UM2505).
   - **`VREF+` Reference Configuration (`SB14`, `R33`, `R34`):** Per UM2505, `SB14` (default OFF) and `R34` (33 kΩ, default fitted) configure the onboard external `VREF+` reference network (`SB14` ON shunting `R33` = 10 kΩ with `R34` removed selects 2.5 V). **CRITICAL HARDWARE CONTENTION RULE:** Before enabling the STM32G474RE's internal **`VREFBUF`** (`VREFBUF->CSR.ENVR = 1, HIZ = 0`), ALWAYS inspect the UM2505 MB1367 schematic for your board revision and disconnect the external `VREF+` supply path so the internal buffer does not short against an external voltage rail!

### 3.6 Signal Integrity, Slew Rate (`OSPEEDR`) & EMI (`DIG-ELEC-006`)

Setting every GPIO to `OSPEEDR = 11` (Very High Speed) is a classic amateur mistake. Faster edge rates ($t_r \approx 1\text{--}3\text{ ns}$) contain harmonic energy up to $f_{\text{knee}} \approx 0.5 / t_r \approx 150\text{--}500\text{ MHz}$, exciting parasitic jumper-wire inductance ($L \approx 1\text{ nH/mm}$) into severe **overshoot/undershoot ringing** ($V = L \frac{di}{dt}$), double-clocking SPI slaves, corrupting adjacent ADC channels, and failing EMC emissions tests.
- **Rule of Thumb:** Select the **lowest `GPIOx->OSPEEDR` setting** that meets your bus timing (`00` Low for LEDs/buttons/100 kHz I2C; `01` Medium for 1–10 MHz SPI/UART; `10`/`11` High/Very High only for >25 MHz QSPI/SPI/MCO), pair fast outputs with a **22–33 Ω series source-termination resistor**, and always route a **short ground return wire directly alongside** every fast signal wire.

---

### 3.7 NUCLEO-G474RE (MB1367, UM2505) Hardware Guide & Traps (`HWSW-PCB-001`, `HWSW-PCB-002`)

Before flashing a single byte, every firmware engineer must audit the board schematic (**UM2505**, PCB **MB1367 Rev C**). Five board-specific hardware facts on the NUCLEO-G474RE routinely trap engineers migrating from older Nucleo-F4xx (MB1136) boards:

```text
+=============================================================================+
|                 NUCLEO-G474RE (MB1367, UM2505) BOARD MAP                    |
+=============================================================================+
|  [CN1 USB Micro-B] ---> [STLINK-V3E Debugger + VCP Bridge]                  |
|                             |  SWD: PA13 (SWDIO), PA14 (SWCLK), PB3 (SWO)   |
|                             |  VCP: PA2 (TX->STLK), PA3 (RX<-STLK) SB17/23  |
|  [JP5: 5V_SEL]  <-- 5V Power Source Selector (STLK / E5V / CHGR) [NOT IDD!] |
|  [JP6: IDD]     <-- MCU VDD Current Measurement Jumper (REMOVE FOR AMMETER) |
|                                                                             |
|  [STM32G474RET6 LQFP64]                                                     |
|   * X3 (24 MHz HSE Crystal): PF0 (OSC_IN), PF1 (OSC_OUT) — POPULATED!       |
|   * X2 (32.768 kHz LSE):     PC14, PC15 — POPULATED!                        |
|   * LD2 (Green User LED):    PA5 (Active-HIGH via SB6) [CONFLICTS SPI1_SCK] |
|   * B1  (Blue User Button):  PC13 (ACTIVE-HIGH! External Pull-Down -> VDD)  |
|   * B2  (Black Reset):       PG10-NRST (Active-LOW)                         |
|   * PB8-BOOT0:               External 10 kOhm Pull-Down (R36) [I2C1 TRAP!]  |
|   * PA9 / PA10 (UCPD):       Internal 5.1 kOhm Dead-Battery PD at Reset!    |
+=============================================================================+
```

#### Critical NUCLEO-G474RE Hardware Traps & Remedies

| # | Board Feature | Exact MB1367 / UM2505 Hardware Reality | Common Trap & Required Firmware/Hardware Fix |
|---|---|---|---|
| **1** | **`JP6` vs `JP5` Jumpers** | **`JP6`** is in series with the MCU $V_{DD}$ supply (**IDD current measurement**). **`JP5`** selects the **5V board power source** (`[1-2]` STLK 5V, `[3-4]` E5V, `[5-6]` CHGR). | **SAFETY HAZARD:** Never put a multimeter in ammeter mode across `JP5`! Always measure MCU current strictly across **`JP6`**. |
| **2** | **User Button `B1` (`PC13`)** | Wired to **$V_{DD}$ when pressed** with an **external pull-down resistor** (**ACTIVE-HIGH**). | Unlike older MB1136 Nucleo-F4xx boards (Active-LOW), reading `PC13` returns **`1` when pressed** and `0` when released. Set `PUPDR = 00` (no internal pull-up fighting the external pull-down!) and trigger EXTI on **rising edge (`EXTI->RTSR1`)** for button press. |
| **3** | **User LED `LD2` (`PA5`)** | Connected via solder bridge **`SB6`** (ON by default) and Arduino **`D13`**. | `PA5` is also `SPI1_SCK` and `DAC1_OUT2`. Driving high-speed `SPI1_SCK` on `PA5` loads the clock line with the LED circuit (and blinks LD2). For clean high-speed SPI1 or precision DAC1_OUT2, either account for LED loading, route `SPI1_SCK` to `PB3` (note SWO conflict), use `SPI2`/`SPI3`, or open `SB6`. |
| **4** | **`PB8-BOOT0` 10 kΩ Pull-Down** | `PB8` shares the hardware `BOOT0` pin and has an external **10 kΩ pull-down resistor (`R36`)** to GND on MB1367. | Relying on STM32's ~40 kΩ internal pull-up when using `PB8` as `I2C1_SCL` creates a voltage divider: $V_{SCL} = 3.3\text{ V} \times \frac{10}{10+40} = 0.66\text{ V}$ (permanently stuck LOW!). **Fix:** Use external 2.2 kΩ–3.3 kΩ I2C pull-ups (and ensure `BOOT0` stays below $V_{IH}$ at reset, or remap `I2C1_SCL` to `PA15`, or clear `nSWBOOT0` option bit). |
| **5** | **`PA9`/`PA10` UCPD Dead-Battery Pull-Downs** | STM32G474 silicon enables internal **5.1 kΩ USB-C PD dead-battery pull-downs** (`UCPD1_DBCC1`/`DBCC2`) on `PA9`/`PA10` (and `PB6`/`PB4` CC lines) **by default out of reset**. | Any signal wired to `PA9`/`PA10` (e.g., `USART1_TX/RX`, `I2C2_SCL`, `TIM1_CH2/3`) gets dragged toward GND by 5.1 kΩ! **Fix:** Enable `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;` and set **`PWR->CR3 |= PWR_CR3_UCPD_DBDIS;`** immediately at startup! |
| **6** | **ST-LINK VCP (`PA2`/`PA3`)** | Connected via **`SB17`/`SB23`** (ON by default) to the onboard **STLINK-V3E** Virtual COM Port. | `PA2`/`PA3` support both **`LPUART1` (AF12)** and **`USART2` (AF7)**. Do NOT connect external UART/ADC peripherals to Arduino `D1`/`D0` (`PA2`/`PA3`) without disconnecting `SB17`/`SB23` (or use `USART1`/`USART3` on Morpho headers). |
| **7** | **HSE Crystal `X3` (`PF0`/`PF1`)** | MB1367 populates a **24 MHz quartz crystal (`X3`)** on `PF0`/`PF1` by default (unlike older Nucleos that used 8 MHz ST-LINK MCO). | Do not use `PF0`/`PF1` as GPIO when HSE crystal is populated; configure `HSE_VALUE = 24000000U` in RCC PLL calculations. |

#### NUCLEO-G474RE Pin-Conflict Quick Reference (Arduino Uno V3 vs ST Morpho `CN7`/`CN10`)

| MCU Pin | LQFP64 Pin | Pin Type (DS12288) | Arduino Uno V3 | Morpho Header | Onboard MB1367 Assignment | Common Alternate Functions (DS12288) |
|---|---|---|---|---|---|---|
| `PA0` | 14 | `TT_a` (3.3V only!) | `A0` (`CN8-1`) | `CN7-28` | Free | `ADC12_IN1`, `TIM2_CH1` (AF1), `USART2_CTS` |
| `PA2` | 16 | `TT_a` (3.3V only!) | `D1` (`CN9-2`) | `CN10-35` | **STLINK VCP TX** (`SB17`) | `USART2_TX` (AF7), `LPUART1_TX` (AF12), `ADC1_IN3` |
| `PA3` | 17 | `TT_a` (3.3V only!) | `D0` (`CN9-1`) | `CN10-37` | **STLINK VCP RX** (`SB23`) | `USART2_RX` (AF7), `LPUART1_RX` (AF12), `ADC1_IN4` |
| `PA4` | 20 | `TT_a` (3.3V only!) | `A2` (`CN8-3`) | `CN7-32` | Free | `DAC1_OUT1`, `SPI1_NSS` (AF5), `ADC2_IN17` |
| `PA5` | 21 | `TT_a` (3.3V only!) | `D13` (`CN5-6`) | `CN10-11` | **LD2 Green LED** (`SB6`) | `SPI1_SCK` (AF5), `DAC1_OUT2`, `TIM2_CH1` (AF1) |
| `PA6` | 22 | `TT_a` (3.3V only!) | `D12` (`CN5-5`) | `CN10-13` | Free | `SPI1_MISO` (AF5), `TIM3_CH1` (AF2), `DAC2_OUT1` |
| `PA7` | 23 | `FT_a` | `D11` (`CN5-4`) | `CN10-15` | Free | `SPI1_MOSI` (AF5), `TIM3_CH2` (AF2), `ADC2_IN4` |
| `PA8` | 41 | `FT_a` | `D7` (`CN9-8`) | `CN10-23` | Free | **`MCO` (AF0)**, `TIM1_CH1` (AF6), `HRTIM1_CHA1` (AF13) |
| `PA9` | 42 | `FT_c` (**UCPD PD!**) | `D8` (`CN5-1`) | `CN10-21` | **UCPD Dead-Battery PD** | `USART1_TX` (AF7), `I2C2_SCL` (AF4), `TIM1_CH2` (AF6) |
| `PA10` | 43 | `FT_c` (**UCPD PD!**) | `D2` (`CN9-3`) | `CN10-33` | **UCPD Dead-Battery PD** | `USART1_RX` (AF7), `TIM1_CH3` (AF6) |
| `PA13` | 46 | `FT` | — | `CN7-13` | **SWDIO** (STLINK-V3E) | `SWDIO` (AF0) — **Do NOT reassign!** |
| `PA14` | 49 | `FT` | — | `CN7-15` | **SWCLK** (STLINK-V3E) | `SWCLK` (AF0) — **Do NOT reassign!** |
| `PB3` | 55 | `FT` | `D3` (`CN9-4`) | `CN10-31` | **SWO Trace** (STLINK-V3E) | `TRACESWO` (AF0), `TIM2_CH2` (AF1), `SPI1/3_SCK` |
| `PB8` | 61 | `FT_f` | `D15/SCL` (`CN5-10`) | `CN10-3` | **`BOOT0` 10 kΩ PD (`R36`)** | `I2C1_SCL` (AF4), `FDCAN1_RX` (AF9), `TIM4_CH3` |
| `PB9` | 62 | `FT_f` | `D14/SDA` (`CN5-9`) | `CN10-5` | Free | `I2C1_SDA` (AF4), `FDCAN1_TX` (AF9), `TIM4_CH4` |
| `PC13` | 2 | `FT` | — | `CN7-23` | **B1 User Button (Active-HIGH)** | `EXTI13`, `PWR_WKUP2`, `RTC_TAMP1` |
| `PF0`/`PF1` | 5/6 | `FT` | — | `CN7-29/31` | **24 MHz HSE Crystal (`X3`)** | `OSC_IN`/`OSC_OUT` |

*(Always cross-check full LQFP64 pinout and AF numbers against **DS12288 Table 12 & Table 13** and **UM2505 Tables 13–17**.)*

---

### 3.8 Lab Equipment Standard Operating Procedures (SOPs) (`HWSW-LAB-001`, `HWSW-LAB-002`, `HWSW-LAB-003`)

#### 1. Oscilloscope SOP (`HWSW-LAB-001`)
1. **Always default to 10x probe attenuation** (input impedance $10\text{ M}\Omega \parallel 10\text{--}15\text{ pF}$). A 1x probe presents $1\text{ M}\Omega \parallel 80\text{--}120\text{ pF}$, which kills 400 kHz I2C edges and can stop a 24 MHz crystal oscillator!
2. **Compensate the 10x probe** on the oscilloscope's 1 kHz calibration lug until square-wave corners are crisp and flat (neither rounded nor peaking). Verify the scope channel attenuation menu is set to `10x`.
3. **Bandwidth & Rise-Time Rule:** To accurately measure a digital edge with rise time $t_r$, the oscilloscope + probe system needs bandwidth $f_{BW} \ge \frac{0.35}{t_r}$ (and $3\times\text{--}5\times$ higher for accurate amplitude/ringing fidelity; e.g., measuring a $3.5\text{ ns}$ SPI clock edge requires $\ge 100\text{ MHz}$ minimum bandwidth).
4. **Eliminate Ground-Lead Ringing:** The 10 cm alligator ground clip adds ~100–150 nH of loop inductance, creating fake 30–80 MHz ringing on scope captures. For high-speed clocks (`MCO`, `SPI`, `HRTIM`), replace the alligator clip with a **short ground spring** touching an adjacent GND pin.
5. **Triggering Discipline:** Use **Normal** (not Auto) trigger mode with **Single-Shot** capture when debugging one-time events (I2C START conditions, bootup glitches, EXTI pulses); use **Pulse-Width** or **Protocol (I2C/SPI/UART)** triggers for bus fault hunting.
6. **Mains Ground Safety:** Standard bench oscilloscope probe ground clips are tied directly to **Earth Ground**. Never clip a probe ground lead onto a non-grounded node or across a high-side shunt resistor — doing so creates a dead short through the scope!

#### 2. Logic Analyzer SOP (`HWSW-LAB-002`)
1. **Oversampling Rule (4x–10x Nyquist):** Digital logic analyzers sample asynchronously. While Nyquist requires $>2\times$, reliable edge timing and glitch detection require **$f_{\text{sample}} \ge 4\times f_{\text{signal}}$ minimum, and $10\times f_{\text{signal}}$ recommended** (e.g., for 1 MHz SPI, sample at $\ge 10\text{ MS/s}$; for 115,200 baud UART, sample at $\ge 1\text{--}2\text{ MS/s}$).
2. **Logic Threshold Voltage:** Set threshold to **3.3V CMOS** ($V_{\text{th}} \approx 1.65\text{ V}$) in Saleae Logic 2 or PulseView (`sigrok`) — never leave at 1.2V or 5V TTL.
3. **Protocol Decoder Setup:** Match exact bus framing (`CPOL`/`CPHA` for SPI; 8N1 & exact baud for UART; 7-bit shifted vs 8-bit R/W address convention for I2C). Remember: logic analyzers hide analog issues (slow RC rise times, ringing, marginal $V_{OL}$) — always pair with an oscilloscope when a bus misbehaves!

#### 3. Digital Multimeter (DMM) & Bench Power Supply Bring-Up SOP (`HWSW-LAB-003`)
Execute this **4-step Zero-Smoke Bring-Up Procedure** before powering any newly wired breadboard or custom PCB:
1. **Unpowered Continuity / Resistance Check:** With power OFF, probe between `3V3` and `GND`, and `5V` and `GND`. Verify resistance is **not** a dead short ($> 100\ \Omega$; initial capacitance charging ramp is normal).
2. **Configure Bench PSU Current Limit (CC/CV):** With outputs **OFF**, set voltage to target (`5.00 V` for NUCLEO `E5V` with `JP5` on `E5V`, or `3.30 V` for direct external rail) and set the **Constant-Current (CC) limit** to $\approx 1.5\times$ expected normal draw (e.g., **150 mA** for NUCLEO-G474RE).
3. **Controlled Power-On & Rail Verification:** Enable PSU output. Watch the CC indicator — if it immediately trips into Constant Current mode, **power off immediately** and locate the short or reversed IC! Measure `3V3` on `CN7` ($3.30\text{ V} \pm 2\%$) with your DMM before connecting ST-LINK or external sensors.
4. **Safe Current Measurement (Burden Voltage & Fuse Rules):**
   - Never probe across a voltage rail while leads are plugged into the DMM `mA`/`A` jacks (instantly blows the DMM fuse!).
   - When measuring STM32G4 Stop/Standby microamps across **`JP6`**, beware **DMM burden voltage** ($V_{\text{burden}} = I \cdot R_{\text{shunt}}$): in the $\mu\text{A}$ range, $R_{\text{shunt}}$ can be $100\text{--}1000\ \Omega$. When the MCU wakes and draws 30 mA, $30\text{ mA} \times 100\ \Omega = 3.0\text{ V}$ drops across the meter, causing an immediate **BOR brownout reset**! Use a dedicated source-measure unit (e.g., **Nordic PPK2**) or a shorting switch across the DMM during wake/boot.

---

## 4. Important Terminology

| Term | Engineering Definition |
|---|---|
| **Time Constant ($\tau$)** | Product $\tau = R \cdot C$ (seconds) governing exponential charging/discharging of a capacitive node through resistance $R$. |
| **Noise Margin ($NM_H, NM_L$)** | Voltage headroom separating driver output bounds from receiver input thresholds: $NM_H = V_{OH(\min)} - V_{IH(\min)}$ and $NM_L = V_{IL(\max)} - V_{OL(\max)}$. |
| **Shoot-Through Current** | Excessive $V_{DD}$-to-$V_{SS}$ leakage current flowing when a floating CMOS input sits in the indeterminate zone ($V_{IL} < V < V_{IH}$), turning both P-MOS and N-MOS transistors partially ON simultaneously. |
| **Open-Drain Output** | Output stage containing only a low-side N-MOS pull-down transistor; can drive LOW or release to Hi-Z, requiring an external pull-up resistor for logic HIGH (used by I2C and shared wired-AND interrupt lines). |
| **5V-Tolerant (`FT`) vs `TT_a`** | STM32 I/O pad structures indicating whether an input safely tolerates up to 5.5 V when $V_{DD} = 3.3\text{ V}$ (`FT`) or is strictly limited to $V_{DD}/V_{DDA} + 0.3\text{ V}$ (`TT_a`). |
| **Dead-Battery Pull-Down (`UCPD_DBDIS`)** | Internal 5.1 kΩ USB Type-C PD resistors active at reset on STM32G474 `PA9`/`PA10` so an unpowered device presents as a USB-C sink; must be disabled via `PWR->CR3` when repurposing those pins. |
| **Flyback / Freewheeling Diode** | Reverse-biased diode across an inductive load providing a safe current recirculation path at turn-off to clamp $V = L \frac{di}{dt}$ voltage spikes. |
| **Burden Voltage** | Voltage drop across a multimeter's internal current-sensing shunt resistor ($V = I \cdot R_{\text{shunt}}$), capable of starving/resetting an MCU during current spikes. |
| **Probe Compensation** | Adjusting the trimmer capacitor in a 10x oscilloscope probe so its RC divider matches the oscilloscope's input capacitance, preserving flat frequency response. |

---

## 5. Register-Level Explanations

Two STM32G474RE registers directly govern hardware-electrical behavior covered in this module (verified against `stm32g474xx.h`):

### 5.1 `PWR->CR3` (Offset `0x08`, Base `0x40007000`) — Disabling UCPD Dead-Battery Pull-Downs

- **RCC Enable Required First:** `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;` (bit 28)
- **Bit 14 — `PWR_CR3_UCPD_DBDIS` (`0x00004000UL`, Reset = `0`):**
  - `0` (Reset Default): Internal 5.1 kΩ UCPD dead-battery pull-down resistors are **ACTIVE** on `PA9`/`PA10` (`UCPD1_DBCC1`/`DBCC2`).
  - `1`: Dead-battery pull-down resistors are **DISABLED**, restoring normal high-impedance GPIO/peripheral behavior on `PA9`/`PA10`.

### 5.2 `GPIOx->OSPEEDR` (Offset `0x08`, Base `0x48000000 + 0x400*port`) — Slew-Rate / EMI Control

Each pin $n \in [0..15]$ occupies 2 bits (`[2n+1 : 2n]`):
- `00` — **Low Speed** (~2–5 MHz max): Best EMI/lowest ringing; use for LEDs, slow relays, 100 kHz I2C.
- `01` — **Medium Speed** (~10–25 MHz): Use for UART, 400 kHz I2C, moderate SPI.
- `10` — **High Speed** (~25–50 MHz): Use for fast SPI, PWM, FDCAN.
- `11` — **Very High Speed** (up to 80–170 MHz depending on $C_L$ per DS12288 Table 55): Use ONLY when timing margins demand sub-3 ns edges (`QSPI`, `MCO`, high-speed `SPI1`).

*(Also remember: per RM0440 §9.4.1, STM32G4 `GPIOx->MODER` resets to **`11b` Analog mode** on all non-debug pins specifically to disconnect the digital Schmitt trigger and prevent floating-input shoot-through current!)*

---

## 6. Relevant Formulas & Calculations

### 6.1 LED Current-Limiting Resistor Sizing
$$R_{\text{LED}} = \frac{V_{OH} - V_f}{I_f}, \qquad P_R = I_f^2 \cdot R_{\text{LED}}$$

**Worked Example:** Driving a green LED ($V_f = 2.1\text{ V}$) from STM32G474RE ($V_{DD} = 3.3\text{ V}$, $V_{OH} \approx 3.1\text{ V}$ at $I_f = 5\text{ mA}$):
$$R_{\text{LED}} = \frac{3.1\text{ V} - 2.1\text{ V}}{0.005\text{ A}} = 200\ \Omega \implies \text{Choose standard } 220\ \Omega \text{ or } 330\ \Omega$$
At $220\ \Omega$: $I_f = \frac{3.1 - 2.1}{220} = 4.55\text{ mA} \le 8\text{ mA}$ (safe!), $P_R = (0.00455)^2 \times 220 = 4.55\text{ mW} \ll 100\text{ mW}$ (0603 safe!).

### 6.2 RC Rise Time, Settling Time & Hardware Debounce

$$
\tau = R \cdot C, \qquad t_{\text{rise (10\%--90\%)}} = \ln(9)\,RC \approx 2.2\,RC, \qquad t_{\text{settle (99.3\%)}} = 5\tau = 5\,RC
$$

**Worked Example (Hardware Switch Debounce):** To filter 10 ms mechanical chatter with $R = 10\text{ k}\Omega$ and $C = 1\ \mu\text{F}$:
$$\tau = 10\times 10^3\ \Omega \times 1\times 10^{-6}\text{ F} = 10\text{ ms}, \qquad t_{\text{rise}} \approx 22\text{ ms}$$

### 6.3 CMOS Noise Margins
$$NM_H = V_{OH(\min)} - V_{IH(\min)}, \qquad NM_L = V_{IL(\max)} - V_{OL(\max)}$$

**Worked Example (STM32G474RE @ 3.3 V driving another 3.3 V CMOS input):**
$$NM_H = 2.90\text{ V} - 2.31\text{ V} = 0.59\text{ V}, \qquad NM_L = 0.99\text{ V} - 0.40\text{ V} = 0.59\text{ V}$$

### 6.4 I2C Pull-Up Resistor Sizing (NXP UM10204 §7.1)
$$R_{p,\min} = \frac{V_{DD} - V_{OL(\max)}}{I_{OL}}, \qquad R_{p,\max} = \frac{t_{r(\max)}}{0.8473 \cdot C_b}$$

**Worked Example (400 kHz I2C Fast Mode, $V_{DD} = 3.3\text{ V}$, $V_{OL(\max)} = 0.4\text{ V}$, $I_{OL} = 3\text{ mA}$, $t_{r(\max)} = 300\text{ ns}$, $C_b = 100\text{ pF}$):**

$$
R_{p,\min} = \frac{3.3\text{ V} - 0.4\text{ V}}{3\times 10^{-3}\text{ A}} = 967\ \Omega
$$

$$
R_{p,\max} = \frac{300\times 10^{-9}\text{ s}}{0.8473 \times 100\times 10^{-12}\text{ F}} = 3.54\text{ k}\Omega
$$

> **Critical Insight:** At $C_b = 100\text{ pF}$ and 400 kHz Fast Mode, the classic "always use 4.7 kΩ" rule-of-thumb **VIOLATES** the $300\text{ ns}$ rise-time specification ($t_r = 0.8473 \times 4700 \times 100\text{ pF} = 398\text{ ns} > 300\text{ ns}$)! Choosing **$2.2\text{ k}\Omega$** satisfies $967\ \Omega \le 2.2\text{ k}\Omega \le 3.54\text{ k}\Omega$ cleanly ($t_r = 186\text{ ns}$). Meanwhile, STM32's internal $40\text{ k}\Omega$ pull-up produces $t_r = 3.39\ \mu\text{s}$ — **11.3x slower than Fast Mode allows!**

### 6.5 Oscilloscope System Bandwidth, Inductive Kickback & 12-Bit ADC Rail Ripple Sensitivity
$$f_{BW} \ge \frac{0.35}{t_r}, \qquad V_{\text{spike}} = L \frac{di}{dt}, \qquad 1\text{ LSB}_{12\text{-bit}} = \frac{V_{\text{REF+}}}{2^{12}} = \frac{V_{\text{REF+}}}{4096}$$

**Worked Example (`HWSW-ANA-001` Noise Floor Impact):**
- At $V_{\text{REF+}} = 3.30\text{ V}$: $1\text{ LSB} = 3.30 / 4096 = \mathbf{805.7\ \mu\text{V}}$.
- If unfiltered digital switching induces $V_{\text{ripple}} = 16\text{ mV}_{\text{pp}}$ on `VREF+`, a mid-scale reading swings by $\frac{16\text{ mV}}{0.8057\text{ mV}} \approx \mathbf{20\text{ LSBs}}$ ($\approx 4.3\text{ bits}$ of lost resolution!), demonstrating why ferrite-bead filtering (`L1`/`L2`), low-ESR `100 nF || 1 µF` decoupling, and quiet `VSSA` return paths are mandatory.

---

## 7. Official Documentation References

- **ST DS12288** — *STM32G474xB / STM32G474xC / STM32G474xE Datasheet*: Table 12 (LQFP64 Pin Definitions: `FT`/`TT_a`), Table 13 (Alternate Functions), Table 14 (Voltage Characteristics), **Table 15 (Current Characteristics: $\pm 20\text{ mA}$, $160\text{ mA}$)**, §5.3.14 (I/O Static & AC Characteristics), §5.3.19 (ADC $R_{\text{AIN}}/C_{\text{ADC}}$).
- **ST UM2505** — *STM32G4 Nucleo-64 Boards (MB1367) User Manual*: §6.3 (`JP5` 5V_SEL), §6.4 (`JP6` IDD), §6.5–6.6 (`LD2` PA5, `B1` PC13 Active-HIGH), §6.7 (24 MHz HSE `X3`), §6.8 (VCP `SB17`/`SB23`), Tables 13–17 (Arduino & Morpho pinouts), Schematics.
- **ST RM0440** — *STM32G4 Series Reference Manual*: Chapter 6 (`PWR_CR3.UCPD_DBDIS`), Chapter 9 (`GPIOx_MODER` Analog reset, `OSPEEDR`).
- **ST AN5093** — *Getting Started with STM32G4 Series Hardware Development* (Decoupling, $V_{DDA}$/`VREF+` ferrite filtering, oscillator layout, boot pin wiring).
- **ST AN2834** — *How to Get the Best ADC Accuracy in STM32 Microcontrollers* (Power-rail integrity, star/continuous ground return paths, source impedance & averaging).
- **ST AN4073** — *How to Improve ADC Accuracy When Using STM32F2xx and STM32F4xx Microcontrollers* (Analog layout & clock-noise mitigation principles).
- **ST AN4899** — *STM32 Microcontroller GPIO Hardware Settings and Low-Power Consumption*.
- **NXP UM10204** — *I2C-Bus Specification and User Manual*, §7.1 (Pull-up resistor sizing).
- **Horowitz & Hill** — *The Art of Electronics* (3rd Ed.), Ch. 1 (Foundations & RC Circuits).

---

## 8. Verified Code Examples

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `stm32g474xx.h` (Not Tested on Physical Hardware).

### 8.1 Safe Board Hardware Bring-Up Helper (Disable UCPD Dead-Battery PD + Read Active-HIGH B1 + Slew-Controlled Output)

```c
#include "stm32g474xx.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Release internal 5.1 kOhm UCPD dead-battery pull-downs on PA9/PA10.
 *        Call early in startup before using PA9/PA10 as GPIO, USART1, or I2C2.
 */
void board_disable_ucpd_dead_battery(void)
{
    /* 1. Enable PWR peripheral clock on APB1 */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1; /* Read-back sync per RM0440 */

    /* 2. Disable 5.1 kOhm dead-battery pull-down resistors */
    PWR->CR3 |= PWR_CR3_UCPD_DBDIS;
}

/**
 * @brief Initialize NUCLEO-G474RE (MB1367) LD2 (PA5) and Active-HIGH B1 (PC13).
 *        Note: PA5 uses Low slew rate (OSPEEDR = 00) to minimize EMI/ringing.
 */
void board_led_and_button_init(void)
{
    RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN);
    (void)RCC->AHB2ENR; /* 2-cycle AHB clock-enable sync */

    /* PA5 (LD2): Push-Pull Output, Low Speed (00), No Pull (00) */
    GPIOA->OTYPER  &= ~(1UL << 5U);
    GPIOA->OSPEEDR &= ~(3UL << (5U * 2U)); /* 00: Low speed */
    GPIOA->PUPDR   &= ~(3UL << (5U * 2U));
    GPIOA->MODER    = (GPIOA->MODER & ~(3UL << (5U * 2U))) | (1UL << (5U * 2U));

    /* PC13 (B1): Digital Input (00), No Internal Pull (external PD exists on MB1367!) */
    GPIOC->PUPDR   &= ~(3UL << (13U * 2U));
    GPIOC->MODER   &= ~(3UL << (13U * 2U));
}

/**
 * @brief Read NUCLEO-G474RE User Button B1 (PC13).
 * @return true when pressed (Active-HIGH on MB1367 per UM2505).
 */
bool board_button_is_pressed(void)
{
    return ((GPIOC->IDR & GPIO_IDR_ID13) != 0UL);
}
```

### 8.2 Analog Rail Health Diagnostic: Runtime `VDDA` Measurement (`VREFINT_CAL`) & Peak-to-Peak LSB Noise Floor (`HWSW-ANA-001`)

> **Verification Status:** **Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wvla`)** against `stm32g474xx.h` (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"
#include <stddef.h>
#include <stdint.h>

#define VREFINT_CAL_ADDR    ((const volatile uint16_t *)0x1FFF75AAUL) /* DS12288 System ROM */
#define VREFINT_CAL_VREF_MV 3000UL

/* Compute actual VDDA (mV) from an ADC1_IN18 (VREFINT) raw conversion to verify rail drop */
uint32_t analog_measure_vdda_mv(uint16_t adc_vrefint_raw)
{
    if (adc_vrefint_raw == 0U) {
        return 0UL;
    }
    uint32_t vrefint_cal = (uint32_t)(*VREFINT_CAL_ADDR);
    return (VREFINT_CAL_VREF_MV * vrefint_cal) / (uint32_t)adc_vrefint_raw;
}

/* Measure peak-to-peak ADC spread (in LSBs) on a DC-shorted or quiet reference input */
uint32_t analog_adc_peak_to_peak_lsb(const uint16_t *samples, size_t count)
{
    if ((samples == NULL) || (count == 0U)) {
        return 0UL;
    }
    uint16_t min_v = samples[0];
    uint16_t max_v = samples[0];
    for (size_t i = 1U; i < count; ++i) {
        if (samples[i] < min_v) { min_v = samples[i]; }
        if (samples[i] > max_v) { max_v = samples[i]; }
    }
    return (uint32_t)(max_v - min_v);
}
```

---

## 9. Practical STM32G474RE Experiments

1. **Experiment 1 — NUCLEO-G474RE Schematic & Continuity Audit (`HWSW-PCB-001`, `HWSW-LAB-003`):**
   - With USB unplugged, use a DMM in continuity/resistance mode to verify `JP6` is in series with $V_{DD}$ while `JP5` selects `5V`.
   - Power via USB, verify `3.30 V ± 2%` on `CN7` `VDD`, and measure `PC13` (`CN7-23`) voltage: confirm **0.00 V released / 3.30 V pressed** (Active-HIGH!).
2. **Experiment 2 — `UCPD_DBDIS` Verification on `PA9` (`STM32-PWR-003`, `HWSW-PCB-001`):**
   - Connect a 10 kΩ pull-up from `3V3` to `PA9` (`CN10-21`). Before setting `PWR_CR3_UCPD_DBDIS`, measure `PA9` (~1.1 V due to the internal 5.1 kΩ dead-battery pull-down!). Step over `PWR->CR3 |= PWR_CR3_UCPD_DBDIS;` in GDB and watch `PA9` immediately jump to **3.30 V**.
3. **Experiment 3 — `OSPEEDR` Slew-Rate & Ringing Capture (`DIG-ELEC-006`, `HWSW-LAB-001`):**
   - Pair with [Lab 03 — RCC 170 MHz & MCO](../18-practical-laboratory/lab-03-rcc-pll-170mhz-clock-switch/README.md): output `MCO` (`PA8`) at 10.625 MHz and capture 10%–90% rise time and overshoot across `OSPEEDR = 00`, `01`, `10`, `11` using a compensated 10x probe with both a 10 cm alligator ground clip and a short ground spring.
4. **Experiment 4 — I2C Pull-Up Rise-Time Comparison (`DIG-ELEC-004`, `HWSW-LAB-001`):**
   - Pair with [Lab 12 — I2C Master](../18-practical-laboratory/lab-12-i2c-master-ssd1306-oled/README.md): capture `SDA` on an oscilloscope comparing internal ~40 kΩ pull-ups vs external 4.7 kΩ vs external 2.2 kΩ at 400 kHz Fast Mode.
   - **Debugging Checkpoint:** In GDB (`arm-none-eabi-gdb`), inspect `x/1wx 0x40007008` (`PWR->CR3`, confirming bit 14 `UCPD_DBDIS = 1`) and `x/1wx 0x48000810` (`GPIOC->IDR`, confirming bit 13 toggles `0 -> 1` when pressing B1).

---

## 10. Common Mistakes & Anti-Patterns

1. **Measuring $I_{DD}$ across `JP5` instead of `JP6`:** `JP5` is the 5V rail selector; putting an ammeter on `JP5` measures ST-LINK + regulator + MCU (or shorts rails). Always use **`JP6`**.
2. **Coding `PC13` (B1) as Active-LOW with internal pull-up:** On MB1367 (UM2505), B1 is **Active-HIGH** with an external pull-down. Enabling an internal pull-up fights the pull-down and wastes continuous current; inverted logic breaks Labs 02, 05, 13, and 14.
3. **Applying 5 V to `TT_a` pins (`PA0–PA6`) or to unpowered `FT` pins:** Permanently damages ESD clamps or ADC muxes. Always verify DS12288 Table 12 before wiring any 5 V sensor.
4. **Forgetting `PWR->CR3 |= PWR_CR3_UCPD_DBDIS` on `PA9`/`PA10`:** Leaves unexpected 5.1 kΩ pull-downs dragging UART/I2C/GPIO lines down.
5. **Using internal pull-ups on `PB8-BOOT0` for I2C1:** The onboard 10 kΩ `R36` pull-down defeats STM32's ~40 kΩ internal pull-up (~0.66 V stuck LOW).
6. **Exceeding $\sum I_{VDD} = 160\text{ mA}$:** Staying under 20 mA per pin still destroys the chip if the sum of all GPIO + core currents exceeds 160 mA.
7. **Omitting flyback diodes across relays/motors or gate pull-downs on MOSFETs:** Inductive kickback destroys FETs/MCUs; floating MOSFET gates briefly energize loads during reset while GPIOs are Hi-Z Analog.
8. **Using a 1x oscilloscope probe on fast signals:** ~100 pF loading severely distorts I2C/SPI waveforms and stalls crystal oscillators.

---

## 11. Hands-On Exercises

1. **Current Budget Audit:** A board drives 10 indicator LEDs directly from `GPIOB` (each $V_f = 2.0\text{ V}$, $R = 150\ \Omega$, $V_{OH} = 3.1\text{ V}$) while the STM32G474RE core draws 35 mA at 170 MHz. Calculate per-pin current and total $\sum I_{VDD}$. Does it violate DS12288 Table 15? Redesign $R$ to keep every pin $\le 6\text{ mA}$ and total $\sum I_{VDD} \le 100\text{ mA}$.
2. **I2C Pull-Up Calculation:** Compute the valid $[R_{p,\min}, R_{p,\max}]$ window for a 3.3 V, 100 kHz Standard Mode I2C bus ($t_{r,\max} = 1000\text{ ns}$, $V_{OL(\max)} = 0.4\text{ V}$, $I_{OL} = 3\text{ mA}$) with $C_b = 180\text{ pF}$. Repeat for 400 kHz Fast Mode ($t_{r,\max} = 300\text{ ns}$).
3. **MOSFET Load Driver Schematic:** Draw and dimension a complete 3.3 V GPIO to 12 V / 400 mA relay coil driver including gate series resistor $R_G$, gate pull-down $R_{PD}$, logic-level N-MOSFET, and flyback diode.
4. **Zero-Conflict NUCLEO-G474RE Pin Plan:** Assign pins on STM32G474RET6 (LQFP64) for simultaneously running: ST-LINK SWD + SWO, ST-LINK VCP UART, 24 MHz HSE crystal, 1× SPI master (avoiding `PA5` LD2 conflict!), 1× I2C master (avoiding `PB8-BOOT0` conflict!), 1× ADC potentiometer (`TT_a` safe), and 1× PWM output. Verify every pin against DS12288 Table 12/13 and UM2505.

---

## 12. Assessment Questions

1. Why does STM32G474RE reset GPIO pins to **Analog mode (`11b`)** rather than floating Digital Input (`00b`)? Explain in terms of CMOS input stage physics.
2. What is the maximum voltage allowed on pin `PA4` (`TT_a`) vs `PB9` (`FT_f`) when $V_{DD} = 3.3\text{ V}$? What happens if you apply 5 V to `PB9` while the NUCLEO board is **unpowered** ($V_{DD} = 0\text{ V}$)?
3. Calculate $R_{p,\min}$ and $R_{p,\max}$ for a 400 kHz I2C bus at $V_{DD} = 3.3\text{ V}$ with $C_b = 120\text{ pF}$. Explain mathematically why 4.7 kΩ fails.
4. You connect a multimeter in $\mu\text{A}$ current mode across `JP6` on NUCLEO-G474RE. The MCU runs Stop mode fine, but repeatedly reboots whenever it wakes into 170 MHz Run mode. Explain the exact physical cause and two engineering remedies.
5. An oscilloscope shows 40 MHz ringing of $\pm 1.2\text{ V}$ on a 5 MHz SPI clock line probed with a 10x probe and a 12 cm alligator ground clip. List three steps (both measurement and firmware/hardware) to distinguish real ringing from probe artifact and suppress real overshoot.
6. What happens on NUCLEO-G474RE if you configure `PA9` as a floating input connected through a 10 kΩ pull-up to 3.3 V without touching `PWR->CR3`?
7. (`HWSW-ANA-001`) Why does ST AN2834 recommend a single continuous ground plane with spatial analog/digital partitioning over a split ground plane under the MCU, and what capacitor ESR/DC-bias failure occurs if you decouple `VREFBUF` with an under-rated 6.3 V Y5V ceramic capacitor?

---

## 13. Measurable Completion Criteria

To pass Stage 01b, you must satisfy all six criteria:
- [ ] **Electrical Calculations Verified:** Independently solve LED sizing, RC settling ($5\tau$ & $9\tau$), CMOS noise margin, DS12288 Table 15 current budget, and UM10204 I2C $[R_{p,\min}, R_{p,\max}]$ calculations with 100% numerical accuracy.
- [ ] **MB1367 Board Audit Complete:** Correctly identify `JP6` (IDD) vs `JP5` (5V_SEL), `B1` (`PC13` Active-HIGH), `LD2` (`PA5`/`SB6`), `PB8-BOOT0` (`R36`), `PA9/PA10` (`UCPD_DBDIS`), `PA2/PA3` (VCP), `SB8`/`SB10` (`L1`/`L2` analog ferrite beads), `SB14`/`R34` (`VREF+`), and `PF0/PF1` (24 MHz X3) on UM2505 schematics.
- [ ] **Analog Rail Design Verified (`HWSW-ANA-001`):** Design an AN5093/AN2834-compliant `VDDA`/`VREF+` ferrite + low-ESR ceramic decoupling network respecting $|V_{\text{DD}} - V_{\text{DDA}}| \le 300\text{ mV}$, and verify ADC peak-to-peak LSB noise floor via `VREFINT_CAL`.
- [ ] **Pin-Conflict Matrix Built:** Produce a conflict-free LQFP64 pin assignment table validated against DS12288 Table 12/13 (respecting `FT` vs `TT_a` voltage limits).
- [ ] **Safe Lab Bring-Up Demonstrated:** Execute the 4-step DMM continuity + CC/CV bench PSU current-limited bring-up checklist and compensate a 10x oscilloscope probe before powering external circuits.
- [ ] **Logic Analyzer & Scope Rules Applied:** Correctly specify $\ge 4\times\text{--}10\times$ logic analyzer sample rate, 3.3 V CMOS threshold, and $f_{BW} \ge 0.35 / t_r$ scope bandwidth for a target SPI/I2C/PWM waveform.

---

> **Sequential Navigation (Step 1 of 17):**  
> **Prev:** [`START_HERE.md`](../START_HERE.md) *(Entry Point)* | **Home:** [`START_HERE.md`](../START_HERE.md) | **Progress:** [`LEARNER_PROGRESS_DASHBOARD.md`](../LEARNERS/LEARNER_PROGRESS_DASHBOARD.md) | **Next:** [Stage 02 — C Language Foundations (`02-c-fundamentals/`)](../02-c-fundamentals/README.md)
