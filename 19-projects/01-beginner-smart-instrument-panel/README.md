# Project 01: Smart Instrument Panel & Telemetry Unit

## 1. Project Overview & Architecture
The Smart Instrument Panel is a multi-peripheral bare-metal application that monitors system inputs (user buttons, analog sensors) and coordinates physical indicator outputs (PWM LED brightness, status flags) while streaming structured JSON telemetry over an asynchronous serial bus (USART2).

```
[ Pushbutton PC13 ] ──> [ EXTI Line 13 ] ─┐
                                          ├─> [ Main Control Superloop ] ──> [ TIM2 PWM Output (PA0) ]
[ Analog Potentiometer ] ──> [ ADC1 ] ────┘               │
                                                          └─> [ USART2 (PA2/PA3) ] ──> [ Host PC Terminal ]
```

## 2. Hardware Subsystems
- **Processor:** STM32G474RE running at 170 MHz (Range 1 Boost Mode).
- **Analog Input:** ADC1 Channel 1 (PA0) sampling potentiometer voltage.
- **User Input:** Pushbutton on PC13 triggering falling-edge EXTI interrupt.
- **Actuator Output:** TIM2 Channel 1 generating 20 kHz hardware PWM on PA5 (modulating LED intensity).
- **Telemetry Interface:** USART2 operating at 115,200 baud streaming formatted telemetry packets.

## 3. Firmware Architecture
1. **Interrupt Service Routines (ISRs):**
   - `EXTI15_10_IRQHandler`: Detects mode toggle button press with software debouncing; sets a volatile event flag.
   - `SysTick_Handler`: Increments 1 ms system tick for deterministic timekeeping.
2. **Main Superloop:**
   - Every 100 ms: Triggers an ADC single-shot conversion to sample input voltage.
   - Maps ADC voltage (0-4095) directly to TIM2 PWM duty cycle (`CCR1` register).
   - Every 500 ms: Transmits a telemetry packet over USART2:
     `{"tick": 12500, "adc_raw": 2048, "voltage_mv": 1650, "mode": "ACTIVE"}\r\n`
3. **Safety & Health:**
   - Independent Watchdog (IWDG) refreshed every loop iteration with a 1.0-second timeout window.

## 4. Verification & Testing
1. Connect serial terminal to NUCLEO ST-LINK VCP (115,200 baud, 8N1).
2. Turn potentiometer: Observe PWM LED smoothly brightening and dimming in real time.
3. Observe serial terminal output streaming valid JSON packets continuously.
