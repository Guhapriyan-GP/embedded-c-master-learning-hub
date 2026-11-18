# RTC, TAMP (32 Backup Regs), USB FS & CRS Register Map

**Target MCU:** STM32G474RE (LQFP64)  
**Base Addresses (APB1):**
- `RTC`: `0x40002800` | `TAMP`: `0x40002400` (`BKP0R..BKP31R` @ `0x40002500..0x4000257C`, 128 bytes)
- `USB`: `0x40005C00` | `USB_PMAADDR` (1 KB dual-port Packet Memory Area): `0x40006000`
- `CRS` (Clock Recovery System for crystal-less HSI48 USB): `0x40002000`  
**IRQ Numbers (`stm32g474xx.h`):** `RTC_TAMP_LSECSS_IRQn=2`, `RTC_WKUP_IRQn=3`, `RTC_Alarm_IRQn=41`, `USB_HP_IRQn=19`, `USB_LP_IRQn=20`, `USBWakeUp_IRQn=42`, `CRS_IRQn=75`  
**Reference Manual:** RM0440, Ch 8 (*CRS*), Ch 37 (*RTC*), Ch 38 (*TAMP*), Ch 43 (*USB*)  
**Concept IDs:** `PERIPH-RTC-001`, `PERIPH-USB-001`, `SEC-TAMP-001`, `REL-CRASH-001`  
**Verification Status:** Every base address, struct offset, and IRQ verified directly against `stm32g474xx.h`; C code examples Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) (Not Tested on Physical Hardware).

---

### 1. RTC (Real-Time Clock) Register Map (`RTC_TypeDef` @ `0x40002800`)

Runs in the Backup Domain (clocked by **32.768 kHz LSE crystal `X2`** on NUCLEO-G474RE `PC14`/`PC15`, LSI ~32 kHz, or HSE/32). Requires `RCC_APB1ENR1_PWREN | RCC_APB1ENR1_RTCAPBEN` + `PWR->CR1.DBP = 1` + write-protection unlock `RTC->WPR = 0xCA; RTC->WPR = 0x53;`.

| Register | Offset | Reset Value | Access | Key Bitfields |
|---|---|---|---|---|
| `RTC_TR` / `RTC_DR` | `0x000` / `0x004` | `0x00000000` / `0x00002101` | RW | BCD Time (`HT/HU`, `MNT/MNU`, `ST/SU`) & Date (`YT/YU`, `WDU`, `MT/MU`, `DT/DU`) |
| `RTC_SSR` | `0x008` | `0x00000000` | RO | Sub-second synchronous down-counter |
| `RTC_ICSR` | `0x00C` | `0x00000007` | RW | Init & status (`ALRAWF`, `ALRBWF`, `WUTWF`, `INITS`, `RSF`, **`INITF` b6**, **`INIT` b7**) |
| `RTC_PRER` | `0x010` | `0x007F00FF` | RW | Prescalers (`PREDIV_A[22:16]` = 127, `PREDIV_S[14:0]` = 255 → $32768 / (128 \times 256) = 1\ \text{Hz}$) |
| `RTC_WUTR` | `0x014` | `0x0000FFFF` | RW | Periodic auto-wakeup reload counter |
| `RTC_CR` | `0x018` | `0x00000000` | RW | Control (`WUCKSEL`, `ALRAE`, `ALRBE`, `WUTE`, `ALRAIE`, `ALRBIE`, `WUTIE`, `OUT2EN`) |
| `RTC_WPR` | `0x024` | `0x00000000` | WO | Write protection unlock (**write `0xCA` then `0x53`**; any other byte re-locks!) |
| `RTC_CALR` | `0x028` | `0x00000000` | RW | Smooth digital calibration (±488 ppm in 0.95 ppm steps) |
| `RTC_ALRMAR/BR` | `0x040` / `0x048` | `0x00000000` | RW | Alarm A & B comparison registers (`ALRMASSR`/`ALRMBSSR` @ `0x044`/`0x04C`) |
| `RTC_SR` / `RTC_SCR` | `0x050` / `0x05C` | `0x00000000` | RO / WO | Status (`ALRAF`, `ALRBF`, `WUTF`, `TSF`) & clear register |

---

### 2. TAMP (Tamper & 32 × 32-Bit Backup Registers) — `0x40002400` (`TAMP_TypeDef`)

> **STM32G4 vs. STM32F4 Architecture Note:** On STM32G4, the **32 Backup Registers (`BKP0R..BKP31R`)** live inside **`TAMP`** (`0x40002500..0x4000257C`), NOT inside `RTC`! On any active tamper event (e.g., `TAMP_IN1` on `PC13`), hardware **immediately zeroes all 32 `BKPxR` registers + SRAM2**!

| Register | Offset | Access | Key Bitfields |
|---|---|---|---|
| `TAMP_CR1` | `0x000` | RW | Enable external tampers (`TAMP1E` b0, `TAMP2E` b1, `TAMP3E` b2) & internal tampers (`ITAMP3E..6E`) |
| `TAMP_CR2` | `0x004` | RW | Trigger edge (`TAMPxTRG`), no-erase mask (`TAMPxNOER`), mask bits |
| `TAMP_FLTCR` | `0x00C` | RW | Sampling frequency, filter count (`TAMPFLT`), precharge duration |
| `TAMP_IER` / `SR` / `SCR` | `0x02C` / `0x030` / `0x03C` | RW / RO / WO | Tamper interrupt enable, status flags (`TAMP1F..3F`), and clear register |
| **`TAMP_BKP0R..BKP31R`** | `0x100..0x17C` | RW | **128 bytes of VBAT/Standby/Shutdown-retained registers** (requires `PWR_CR1.DBP = 1` to write) |

---

### 3. USB Full-Speed Device (`0x40005C00`) & CRS (`0x40002000`)

USB 2.0 FS (12 Mbps) Device-only controller using `PA11` (`USB_DM`) and `PA12` (`USB_DP`, integrated 1.5 kΩ pull-up via `BCDR.DPPU`). **CRS** auto-trims **HSI48** against 1 kHz host USB SOF frames for **crystal-less USB**!

| Peripheral & Register | Offset | Access | Key Bitfields |
|---|---|---|---|
| `USB_EP0R..EP7R` | `0x000..0x01C` | RW/t/rc_w0 | Endpoint 0–7 config (`EA[3:0]`, `STAT_TX[5:4]` toggle, `DTOG_TX` toggle, `CTR_TX` rc_w0, `EP_TYPE[10:9]`, `STAT_RX[13:12]` toggle, `CTR_RX` rc_w0) |
| `USB_CNTR` | `0x040` | RW | `FRES` b0 (force reset), `PDWN` b1 (power-down; clear & wait 1 µs!), `LPMODE`, `FSUSP`, `RESETM`, `CTRM` |
| `USB_ISTR` | `0x044` | RO/rc_w0 | `EP_ID[3:0]`, `DIR` b4, `SOF`, `RESET`, `SUSP`, `WKUP`, `ERR`, `CTR` b15 |
| `USB_FNR` / `DADDR` / `BTABLE` | `0x048` / `0x04C` / `0x050` | RO / RW / RW | Frame number, USB device address (`ADD[6:0]`, `EF` b7), Buffer Descriptor Table offset in 1 KB `USB_PMAADDR` (`0x40006000`) |
| `USB_BCDR` | `0x058` | RW | Battery charging detection & **`DPPU` (Bit 15 = internal 1.5 kΩ D+ pull-up to initiate enumeration!)** |
| `CRS_CR` | `0x000` | RW | `SYNCOKIE`, `CEN` b5 (counter enable), **`AUTOTRIMEN` b6** (auto-trim HSI48), `TRIM[14:8]` |
| `CRS_CFGR` | `0x004` | RW | `RELOAD[15:0]` (`47999` for 48 MHz / 1 kHz SOF), `FELIM[23:16]` (`34`), `SYNCSRC[29:28]` (`10` = USB SOF) |
| `CRS_ISR` / `CRS_ICR` | `0x008` / `0x00C` | RO / WO | `SYNCOKF` b0 (HSI48 locked to USB SOF!), `SYNCWARNF`, `ERRF`, `ESYNCF` |

---

### 4. CMSIS Code Examples: `TAMP->BKPxR` Crash Persistence & Crystal-Less USB+CRS Bring-Up

> **Status:** Compile-Verified (`arm-none-eabi-gcc 14.3.1`, `-Wall -Wextra -Werror`) / Statically Reviewed (Not Tested on Physical Hardware).

```c
#include "stm32g474xx.h"

/* 1. Persist post-mortem fault PC & CFSR across soft/watchdog resets in TAMP->BKP0R..BKP2R */
void TAMP_SaveCrashRecord(uint32_t fault_pc, uint32_t cfsr) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN | RCC_APB1ENR1_RTCAPBEN;
    (void)RCC->APB1ENR1;          /* Mandatory RCC clock-enable readback */
    PWR->CR1      |= PWR_CR1_DBP; /* Unlock backup domain */
    TAMP->BKP0R    = 0xC0DEC0DEU; /* Magic signature */
    TAMP->BKP1R    = fault_pc;
    TAMP->BKP2R    = cfsr;
}

/* 2. Enable crystal-less 48 MHz HSI48 + CRS + USB FS pull-up (ready for RESET/CTR IRQs) */
void USB_FS_CRS_Init(void) {
    /* Enable HSI48 and route to CLK48SEL (00b) */
    RCC->CRRCR   |= RCC_CRRCR_HSI48ON;
    while (!(RCC->CRRCR & RCC_CRRCR_HSI48RDY)) { }
    RCC->CCIPR   &= ~RCC_CCIPR_CLK48SEL_Msk;

    /* Enable CRS auto-trim synced to USB SOF (SYNCSRC = 10b, default RELOAD = 47999) */
    RCC->APB1ENR1 |= RCC_APB1ENR1_CRSEN | RCC_APB1ENR1_USBEN;
    (void)RCC->APB1ENR1;          /* Mandatory RCC clock-enable readback */
    CRS->CFGR      = (2U << CRS_CFGR_SYNCSRC_Pos) | (34U << CRS_CFGR_FELIM_Pos) | 47999U;
    CRS->CR       |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

    /* Power up USB transceiver (clear PDWN, wait >= 1 us, release FRES, enable DPPU) */
    USB->CNTR  = USB_CNTR_FRES;
    for (volatile uint32_t i = 0U; i < 200U; i++) { __NOP(); }
    USB->CNTR  = 0U;
    USB->ISTR  = 0U;
    USB->BTABLE = 0U;
    USB->CNTR  = USB_CNTR_RESETM | USB_CNTR_CTRM;
    USB->BCDR |= USB_BCDR_DPPU; /* Connect 1.5 kOhm pull-up on PA12 (USB_DP) */
    NVIC_EnableIRQ(USB_LP_IRQn);
}
```
