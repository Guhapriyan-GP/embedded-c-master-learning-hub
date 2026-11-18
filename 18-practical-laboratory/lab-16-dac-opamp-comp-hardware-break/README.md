# Lab 16: Mixed-Signal Hardware Safety — DAC1 + OPAMP1 PGA + COMP1 → `TIM1_BKIN` Sub-Microsecond Trip

> **Verification Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`) / Statically Reviewed (Not Tested on Physical NUCLEO-G474RE Hardware)

---

## 1. Objective & Concept IDs

Build a genuine **STM32G474RE mixed-signal hardware overcurrent protection chain** completely in silicon with **zero CPU latency**:
1. **`DAC1_CH1` (`PA4`)** generates a programmable overcurrent trip reference (`1.65 V`, `HFSEL = 10b` for `170 MHz`) observable on `PA4` and routed internally to `COMP1`.
2. **`OPAMP1`** amplifies a small analog shunt voltage on **`PA1`** in **PGA ×8 mode** (`OPAMPINTEN = 1`, internal route to `ADC1_IN13` — keeping `PA2` ST-LINK VCP completely free!).
3. **`COMP1`** compares **`PA1`** against **`DAC1_CH1`** (`~16.7 ns` propagation delay) and connects **directly in hardware** to **`TIM1_BKIN`** (`TIM1->AF1.BKCMP1E = 1`), asynchronously clearing **`TIM1->BDTR.MOE`** in **< 50 ns** to extinguish **`TIM1_CH1` (`PA8`)** PWM before power MOSFETs can be damaged!

- **Target Concept IDs:** `PERIPH-DAC-001` (`DAC1` `HFSEL` @ 170 MHz), `PERIPH-OPAMP-001` (`OPAMP1` Internal PGA), `PERIPH-COMP-001` (`COMP1` Hysteresis & Hardware Lock), `PERIPH-TIM-003` (`TIM1` `BDTR` Hardware Break `MOE`/`BKE`/`AF1`), `REL-SAFE-001` (Hardware-First Safety Interlocks)
- **Scope Layer:** `[STM32G474RE Exact]`

---

## 2. Required Hardware

- **Development Board:** ST NUCLEO-G474RE (`MB1367` Rev C)
- **External Components:** `10 kΩ` potentiometer (or bench DC supply `0..3.3 V`) connected to `PA1` to simulate a current-sense fault; oscilloscope (`CH1` on `PA8` `TIM1_CH1` PWM, `CH2` on `PA1` fault input, optional `CH3` on `PA4` `DAC1_OUT1` threshold)

---

## 3. Required Software

- `arm-none-eabi-gcc` (14.3.1+), `make`, `openocd`, `arm-none-eabi-gdb`
- CMSIS headers (`stm32g474xx.h`, `core_cm4.h`)

---

## 4. Wiring & Pin Configuration

| MCU Pin | Peripheral Function | `MODER` | `AFR` | Role |
|---|---|---|---|---|
| `PA4` (Arduino `A2`) | `DAC1_OUT1` | `11b` (Analog) | N/A | `1.650 V` programmable trip threshold (also routed internally to `COMP1_INM`) |
| `PA1` (Arduino `A1`) | `OPAMP1_VINP` & `COMP1_INP` | `11b` (Analog) | N/A | Analog sense input (fault triggers when $V_{\text{PA1}} > 1.650\text{ V}$) |
| `PA8` (Arduino `D7`) | `TIM1_CH1` PWM Output | `10b` (AF) | `AF6` | `50 kHz` 50% duty PWM — shuts down asynchronously in `< 50 ns` on trip! |
| `PA5` (`LD2`) | Fault Status LED | `01b` (Output) | N/A | Illuminated inside `TIM1_BRK_TIM15_IRQHandler` after hardware trip |

```text
                +------------------+       Internal Route       +--------------------+
[ 0..3.3V ] --> | PA1 (COMP1_INP)  | -------------------------> | (+)                |
                |  & OPAMP1_VINP   |                            |       COMP1        | ---> [ TIM1->AF1.BKCMP1E ]
                +------------------+                            |   (~16.7 ns prop)  |              |
                         |                                      |                    |              v
                [ OPAMP1 PGA x8 ] --> (Internal ADC1_IN13)      | (-)                |   Clears TIM1->BDTR.MOE
                                                                +--------------------+   in < 50 ns (NO CPU!)
[ DAC1_CH1 (PA4) = 1.65V ] ----------(Internal INMSEL=5)------->          ^                         |
                                                                                                    v
                                                                                         PA8 (TIM1_CH1) -> SAFE LOW
                                                                                         + TIM1_BRK_TIM15_IRQHandler
```

---

## 5. Register Configuration & Mathematical Derivation

**Official Documentation:** **RM0440 Chapter 22** (*DAC*, §22.7.14 `DAC_MCR.HFSEL`), **RM0440 Chapter 24** (*COMP*, Table 197), **RM0440 Chapter 25** (*OPAMP*, Table 201), **RM0440 Chapter 28** (*Advanced-control timers TIM1*, `BDTR` & `AF1`).

| Peripheral | Clock Enable Bit | Key Registers & Exact Values |
|---|---|---|
| **`DAC1`** | `RCC->AHB2ENR.DAC1EN` (b16) *(AHB2, NOT APB1!)* | `MCR`: **`HFSEL = 10b`** (mandatory for $f_{\text{HCLK}} > 160\text{ MHz}$!), `MODE1 = 001b` (external pin + internal on-chip peripheral); `DHR12R1 = 2048U` ($\frac{2048}{4095} \times 3.3\text{ V} = 1.650\text{ V}$) |
| **`OPAMP1`** | `RCC->APB2ENR.SYSCFGEN` (b0) | `CSR`: `VPSEL = 00b` (`PA1`), `VMSEL = 10b` (PGA), `PGGAIN = 00010b` (`×8`), `OPAMPINTEN = 1` (internal to `ADC1_IN13`), `HIGHSPEEDEN = 1`, `OPAMPxEN = 1` |
| **`COMP1`** | `RCC->APB2ENR.SYSCFGEN` (b0) | `CSR`: `INPSEL = 0` (`PA1`), `INMSEL = 0101b` (`5U` = `DAC1_CH1`), `HYST = 010b` (`20 mV`), `POLARITY = 0`, `EN = 1` |
| **`TIM1`** | `RCC->APB2ENR.TIM1EN` (b11) | `ARR = 3399` (`50.0 kHz` @ 170 MHz), `CCR1 = 1700` (50% duty); `AF1 = TIM1_AF1_BKCMP1E` (disable external `BKINE`); `BDTR = TIM_BDTR_OSSI \| TIM_BDTR_OSSR \| TIM_BDTR_BKE \| TIM_BDTR_BKP \| TIM_BDTR_MOE` (`AOE = 0` latched safe off!) |

> [!IMPORTANT]
> **Why Software Polling Cannot Protect Power Electronics:** An RTOS task wake + ADC read + GPIO/TIM disable takes `10–100 µs`. Under a dead short across a 48 V DC bus, power MOSFETs fail thermally in `< 2–5 µs`. Routing `COMP1` directly into `TIM1_BKIN` (`BKCMP1E`) guarantees **`< 50 ns`** asynchronous hardware shutdown — even if the Cortex-M4 CPU is halted at a breakpoint or trapped in a HardFault!

---

## 6. Code Implementation

```c
#include "stm32g474xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;

static volatile uint32_t g_fault_trip_count = 0U;
static volatile uint32_t g_fault_latched    = 0U;

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

/* Post-fault diagnostic ISR ONLY — PWM was ALREADY extinguished by hardware in <50 ns! */
void TIM1_BRK_TIM15_IRQHandler(void) {
    if ((TIM1->SR & TIM_SR_BIF) != 0U) {
        TIM1->SR = ~TIM_SR_BIF;    /* Atomic rc_w0 clear */
        g_fault_trip_count++;
        g_fault_latched = 1U;
        GPIOA->BSRR = GPIO_BSRR_BS5; /* Illuminate LD2 fault indicator */
        __DSB();
    }
}

static void DAC1_Threshold_Init_1650mV(void) {
    /* DAC1 lives on AHB2 on STM32G474RE (NOT APB1!) */
    RCC->AHB2ENR |= RCC_AHB2ENR_DAC1EN | RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;

    /* PA4 -> Analog Mode (11b) */
    GPIOA->MODER |=  (3U << (4U * 2U));
    GPIOA->PUPDR &= ~(3U << (4U * 2U));

    DAC1->CR &= ~DAC_CR_EN1;

    /* CRITICAL @ 170 MHz: Set HFSEL = 10b (> 160 MHz interface mode)!
     * MODE1 = 001b: Normal mode, buffer ON, connected to BOTH external PA4 and internal COMP1 */
    DAC1->MCR = (DAC1->MCR & ~(DAC_MCR_HFSEL | DAC_MCR_MODE1)) |
                DAC_MCR_HFSEL_1                                 |
                DAC_MCR_MODE1_0;

    DAC1->CR |= DAC_CR_EN1;

    /* Wait for DAC wakeup time t_WAKEUP (~10 us) */
    for (volatile uint32_t i = 0U; i < 1000U; i++) { __NOP(); }

    /* Set 12-bit right-aligned threshold: 2048 / 4095 * 3.3V = 1.650 V */
    DAC1->DHR12R1 = 2048U;
}

static void OPAMP1_PGA_x8_Init(void) {
    /* OPAMP1 & COMP1 share the APB2 SYSCFG clock */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->APB2ENR;
    (void)tmpreg;

    /* PA1 -> Analog Mode (11b) */
    GPIOA->MODER |=  (3U << (1U * 2U));
    GPIOA->PUPDR &= ~(3U << (1U * 2U));

    /* OPAMP1: Non-inverting PGA x8 on PA1 (VPSEL=00), VMSEL=10 (PGA), PGGAIN=00010 (x8),
     * OPAMPINTEN=1 (route output internally to ADC1_IN13 -> keeps PA2 VCP free!), High-Speed */
    OPAMP1->CSR = (0U << OPAMP_CSR_VPSEL_Pos)  |
                  (2U << OPAMP_CSR_VMSEL_Pos)  |
                  (2U << OPAMP_CSR_PGGAIN_Pos) |
                  OPAMP_CSR_OPAMPINTEN         |
                  OPAMP_CSR_HIGHSPEEDEN        |
                  OPAMP_CSR_OPAMPxEN;
}

static void COMP1_Overcurrent_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    volatile uint32_t tmpreg = RCC->APB2ENR;
    (void)tmpreg;

    /* COMP1: (+) = PA1 (INPSEL=0), (-) = DAC1_CH1 internal (INMSEL=5),
     * Hysteresis = 20 mV (HYST=2), Non-inverted output, Enable */
    COMP1->CSR = (0U << COMP_CSR_INPSEL_Pos) |
                 (5U << COMP_CSR_INMSEL_Pos) |
                 (2U << COMP_CSR_HYST_Pos)   |
                 COMP_CSR_EN;
}

static void TIM1_PWM_With_HardwareBreak_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    volatile uint32_t tmpreg = RCC->APB2ENR;
    (void)tmpreg;

    /* PA8 -> AF6 (TIM1_CH1), High Speed */
    GPIOA->MODER   = (GPIOA->MODER   & ~(3U << (8U * 2U))) | (2U << (8U * 2U));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3U << (8U * 2U))) | (2U << (8U * 2U));
    GPIOA->AFR[1]  = (GPIOA->AFR[1]  & ~(0xFU << 0U))      | (6U << 0U);

    /* PA5 -> Fault LED Output, initially OFF */
    GPIOA->MODER  = (GPIOA->MODER & ~(3U << (5U * 2U))) | (1U << (5U * 2U));
    GPIOA->BSRR   = GPIO_BSRR_BR5;

    TIM1->CR1  = 0U;
    TIM1->PSC  = 0U;
    TIM1->ARR  = 3399U; /* 170 MHz / 3400 = 50.000 kHz */
    TIM1->CCR1 = 1700U; /* 50% duty cycle */

    /* PWM Mode 1 with preload on CH1 */
    TIM1->CCMR1 = (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM1->CCER  = TIM_CCER_CC1E;

    /* Safe idle state: OIS1 = 0 (drive LOW when MOE clears on Break!) */
    TIM1->CR2 &= ~TIM_CR2_OIS1;

    /* CRITICAL: Route COMP1_OUT directly to TIM1 Break 1 (BKCMP1E = 1, active HIGH polarity)
     * Disable external pin BKINE so floating pin does not false-trip */
    TIM1->AF1 = TIM1_AF1_BKCMP1E;

    /* Configure Break & Dead-Time Register:
     * BKE=1 (enable Break), BKP=1 (Break active HIGH matches COMP1_OUT=1),
     * OSSI=1, OSSR=1 (actively drive safe LOW when MOE clears), AOE=0 (stay LATCHED OFF!) */
    TIM1->BDTR = TIM_BDTR_OSSI | TIM_BDTR_OSSR | TIM_BDTR_BKE | TIM_BDTR_BKP;

    /* Force shadow reload, clear stale Break flag, enable Break interrupt */
    TIM1->EGR  = TIM_EGR_UG;
    TIM1->SR   = ~TIM_SR_BIF;
    TIM1->DIER = TIM_DIER_BIE;

    NVIC_SetPriorityGrouping(0U);
    NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, 0U); /* Highest urgency fault notification */
    NVIC_ClearPendingIRQ(TIM1_BRK_TIM15_IRQn);
    NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);

    /* Start timer and assert Main Output Enable (MOE) */
    TIM1->CR1  |= TIM_CR1_ARPE | TIM_CR1_CEN;
    TIM1->BDTR |= TIM_BDTR_MOE;
}

int main(void) {
    Clock_Init_170MHz();
    DAC1_Threshold_Init_1650mV();
    OPAMP1_PGA_x8_Init();
    COMP1_Overcurrent_Init();
    TIM1_PWM_With_HardwareBreak_Init();

    while (1) {
        /* Even if CPU executes infinite __WFI() or halts in GDB, COMP1->TIM1_BKIN
         * still shuts down PA8 PWM asynchronously in < 50 ns on overcurrent! */
        __WFI();
    }
}
```

---

## 7. Expected Output

1. When `PA1` is below `1.65 V`, `PA8` outputs a steady `50.00 kHz`, `50%` duty square wave and `LD2` (`PA5`) is OFF.
2. The instant `PA1` rises above `1.65 V` (`PA4` reference), `COMP1->CSR.VALUE` asserts `1`, **hardware asynchronously clears `TIM1->BDTR.MOE = 0`**, `PA8` drops to `0 V` in **`< 50 ns`**, and `LD2` latches ON via `TIM1_BRK_TIM15_IRQHandler`.
3. Lowering `PA1` back below `1.65 V` keeps `PA8` safely **latched OFF** (`AOE = 0`) until firmware explicitly verifies fault clearance and re-enables `MOE`.

---

## 8. Debugging Procedure

1. Halt the CPU completely in GDB (`Ctrl+C`) while `TIM1` is outputting `50 kHz` on `PA8`!
2. Raise `PA1` above `1.65 V` **while the CPU is still halted** and observe `PA8` immediately drop to `0 V` on the oscilloscope — proving **0% CPU dependence**!
3. Inspect registers in GDB:
   ```gdb
   (gdb) p/x COMP1->CSR     # Bit 30 (VALUE) == 1 when PA1 > 1.65V
   (gdb) p/x TIM1->BDTR     # Bit 15 (MOE) == 0 (hardware cleared!)
   (gdb) p/x TIM1->AF1      # Bit 1 (BKCMP1E) == 1
   ```

---

## 9. Common Errors & Troubleshooting

| Symptom | Root Cause | Verified Fix |
|---|---|---|
| `DAC1` output is distorted or fails to update at 170 MHz | Forgot `DAC_MCR_HFSEL_1` (`10b`) required for $f_{\text{HCLK}} > 160\text{ MHz}$ | Set `DAC1->MCR \|= DAC_MCR_HFSEL_1;` before `EN1` |
| `TIM1_CH1` never turns on (`MOE` stays `0`) | Left `TIM1_AF1_BKINE` enabled on a floating external pin, OR `PA1` starts above `1.65 V` | Set `TIM1->AF1 = TIM1_AF1_BKCMP1E;` (disabling `BKINE`) and verify `PA1 < 1.65 V` |
| `COMP1` / `OPAMP1` writes have no effect | Forgot `RCC->APB2ENR \|= RCC_APB2ENR_SYSCFGEN` | Enable `SYSCFGEN` before configuring `COMP1`/`OPAMP1` |

---

## 10. Verification Procedure

- [ ] Code compiles cleanly with `arm-none-eabi-gcc -Wall -Wextra -Werror`.
- [ ] `DAC1->MCR` has `HFSEL = 10b` and `PA4` measures `1.650 V ± 20 mV`.
- [ ] `TIM1->BDTR.MOE` transitions `1 -> 0` asynchronously when `PA1 > 1.65 V` even while CPU is halted.
- [ ] `PA8` shutdown propagation delay measures `< 100 ns` on oscilloscope.

---

## 11. Extension Exercises

1. **One-Way Hardware Safety Lock:** Set `COMP1->CSR |= COMP_CSR_LOCK;` after configuration so runaway firmware can never accidentally disable overcurrent protection until a hardware reset!
2. **Switch-Node Ringing Blanking:** Configure `COMP1->CSR` (`COMP_CSR_BLANKING`, bits `[21:19]`) to blank `COMP1` for the first 200 ns of each PWM cycle (`TIM1_OC5`) to reject MOSFET turn-on switching spikes.
3. **Sample `OPAMP1` via `ADC1_IN13`:** Enable `ADC1` on Channel `13` (internally wired to `OPAMP1` via `OPAMPINTEN = 1`) and verify `200 mV` on `PA1` reads `~1600 mV` inside `ADC1->DR`.
