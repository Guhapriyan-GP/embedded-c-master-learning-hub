# ARM Cortex-M4 NVIC & SysTick Register Map

**Core Architecture:** ARMv7-M  
**System Control Space (SCS) Base:** `0xE000E000`  
**SysTick Base:** `0xE000E010` (`SysTick_Type`)  
**NVIC Base:** `0xE000E100` (`NVIC_Type`)  
**SCB Base:** `0xE000ED00` (`SCB_Type`)  
**Reference Manuals:** ARMv7-M Architecture RM & ST PM0214

---

### 1. SysTick (24-bit System Timer) Register Map

The SysTick timer is integrated directly into the processor core, providing a standardized, architecture-independent millisecond timebase for operating systems and delay functions.

| Register | Offset | Reset Value | Description |
|---|---|---|---|
| `SysTick_CTRL` | `0x00` | `0x00000000` | Control and status register (Enable, Interrupt enable, Clock source) |
| `SysTick_LOAD` | `0x04` | Undefined | 24-bit reload value register (Max: `0x00FFFFFF` = 16,777,215) |
| `SysTick_VAL` | `0x08` | Undefined | 24-bit current value register (Writing any value resets counter to 0) |
| `SysTick_CALIB`| `0x0C` | Implementation | Calibration register |

#### `SysTick_CTRL` Bitfields
- **Bit 0 (`ENABLE`):** 1 = Counter operates; 0 = Counter halted.
- **Bit 1 (`TICKINT`):** 1 = Counting down to 0 asserts SysTick exception request (`SysTick_Handler`).
- **Bit 2 (`CLKSOURCE`):** 0 = External reference clock ($\text{HCLK} / 8$); 1 = Core processor clock ($\text{HCLK}$).
- **Bit 16 (`COUNTFLAG`):** Returns 1 if timer counted to 0 since last read; cleared automatically on read or by writing to `VAL`.

---

### 2. NVIC (Nested Vectored Interrupt Controller) Register Map

The NVIC manages external peripheral interrupt channels (up to 102 channels on STM32G474).

| Register Group | Base Offset | Description |
|---|---|---|
| `NVIC_ISER[0..7]` | `0x000` | Interrupt Set-Enable Registers (Writing `1` enables IRQ; writing `0` has no effect) |
| `NVIC_ICER[0..7]` | `0x080` | Interrupt Clear-Enable Registers (Writing `1` disables IRQ) |
| `NVIC_ISPR[0..7]` | `0x100` | Interrupt Set-Pending Registers (Software IRQ trigger) |
| `NVIC_ICPR[0..7]` | `0x180` | Interrupt Clear-Pending Registers (Clears pending flag) |
| `NVIC_IABR[0..7]` | `0x200` | Interrupt Active Bit Registers (Read-only: indicates currently executing ISR) |
| `NVIC_IPR[0..59]` | `0x300` | Interrupt Priority Registers (8-bit field per IRQ) |

#### Priority Bit Implementation on STM32G4
Although the ARMv7-M architecture defines up to 8 bits for priority per IRQ, **ST implements only the top 4 bits (Bits [7:4])** on the STM32G4. The lower 4 bits are tied to zero.
- Priority levels range from `0` (Highest priority) to `15` (Lowest priority).
- **SCB AIRCR Priority Grouping (`PRIGROUP`):** Splits the 4 priority bits between **Preemption Priority** (can interrupt another active ISR) and **Subpriority** (resolves simultaneous triggers; does NOT preempt).

---

### 3. CMSIS Code Examples

#### Example 1: 1 ms SysTick Interrupt Initialization at 170 MHz
```c
/* Target: 1 millisecond interrupt tick
 * f_HCLK = 170,000,000 Hz
 * Ticks required = 170,000,000 * 0.001 s = 170,000 ticks
 * LOAD value = Ticks - 1 = 169,999 */

void SysTick_Init_1ms(void) {
    SysTick->CTRL = 0;                  /* Disable timer during configuration */
    SysTick->LOAD = (170000000UL / 1000UL) - 1UL; /* 169,999 */
    SysTick->VAL  = 0;                  /* Clear current count register */
    
    /* Enable SysTick with Core Clock (CLKSOURCE=1) and Interrupt (TICKINT=1) */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

volatile uint32_t ms_ticks = 0;
void SysTick_Handler(void) {
    ms_ticks++;
}
```

#### Example 2: NVIC Priority Grouping & Interrupt Enable
```c
/* 1. Configure Priority Grouping to Group 4:
 *    4 bits of preemption priority (16 levels), 0 bits of subpriority.
 *    Requires VECTKEY (0x5FA) in top 16 bits of SCB->AIRCR! */
NVIC_SetPriorityGrouping(3); /* CMSIS macro handles VECTKEY write */

/* 2. Configure Timer 2 Priority (e.g., Priority 5) */
NVIC_SetPriority(TIM2_IRQn, 5);

/* 3. Enable Timer 2 Interrupt in NVIC */
NVIC_EnableIRQ(TIM2_IRQn);
```
