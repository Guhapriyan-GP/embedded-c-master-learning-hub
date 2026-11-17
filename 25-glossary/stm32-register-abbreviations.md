# STM32 Register Abbreviations & Naming Conventions

This guide decodes standard acronyms and register naming conventions across STMicroelectronics reference manuals:

| Acronym | Full Hardware Register Name | Functional Purpose |
|---|---|---|
| **ACR** | Access Control Register | Configures Flash memory wait states and caches (`FLASH_ACR`). |
| **AFR / AFRL / AFRH** | Alternate Function Register | Selects peripheral multiplexing (AF0-AF15) for GPIO pins. |
| **ARR** | Auto-Reload Register | Sets the timer period counter threshold in hardware timers (`TIMx_ARR`). |
| **BRR** | Baud Rate Register / Bit Reset Register | In USART: Baud rate divisor. In GPIO: Dedicated bit reset register. |
| **BSRR** | Bit Set/Reset Register | Atomic 32-bit register to set or clear GPIO pins. |
| **CCER** | Capture/Compare Enable Register | Enables timer output channels and selects polarity. |
| **CCMR1 / CCMR2** | Capture/Compare Mode Register | Selects timer channel mode (Output PWM vs Input Capture). |
| **CCR** | Capture/Compare Register | Sets timer PWM duty cycle or stores captured timestamp (`TIMx_CCRx`). |
| **CFGR** | Configuration Register | Found in RCC, ADC, SYSCFG for core subsystem setup. |
| **CR / CR1 / CR2** | Control Register | Primary configuration and enable register for peripherals. |
| **DIER** | DMA / Interrupt Enable Register | Enables interrupt and DMA event generation in timers (`TIMx_DIER`). |
| **DR** | Data Register | Transmit / Receive data holding register in SPI, USART, ADC. |
| **EGR** | Event Generation Register | Software trigger to force update events in timers (`TIMx_EGR`). |
| **EXTICR** | External Interrupt Configuration Register | In SYSCFG: Selects GPIO port routed to EXTI lines 0-15. |
| **IDR** | Input Data Register | Reads physical digital voltage levels on GPIO pins (`GPIOx_IDR`). |
| **IMR** | Interrupt Mask Register | Unmasks interrupt request generation in EXTI (`EXTI_IMR1`). |
| **ISR** | Interrupt and Status Register | Reads peripheral status flags (TXE, RXNE, EOC, etc.). |
| **MODER** | Mode Register | Configures GPIO pin state: Input, Output, Alternate Function, Analog. |
| **ODR** | Output Data Register | Sets or reads digital output states on GPIO pins (`GPIOx_ODR`). |
| **OSPEEDR** | Output Speed Register | Sets slew rate on GPIO output drivers (Low, Medium, High, Very High). |
| **OTYPER** | Output Type Register | Selects GPIO output topology: Push-Pull vs Open-Drain. |
| **PR / PR1** | Pending Register | Contains interrupt event pending flags in EXTI. Cleared by writing `1`! |
| **PUPDR** | Pull-Up / Pull-Down Register | Configures internal weak resistors on GPIO pins. |
| **RDR** | Receive Data Register | Read-only register containing received serial byte in USART/I2C. |
| **SMPR1 / SMPR2** | Sample Time Register | Selects ADC sampling clock cycles per analog channel. |
| **SQR1 - SQR4** | Sequence Register | Defines multi-channel conversion sequence and channel order in ADC. |
| **SR** | Status Register | Reads flags indicating peripheral hardware state. |
| **TDR** | Transmit Data Register | Write-only register containing byte to transmit in USART/I2C. |
| **TIMINGR** | Timing Register | Configures I2C bus clock SCL high/low times and filter delays. |
| **VOS** | Voltage Output Scaling | Core regulator voltage setting in PWR (`Range 1` vs `Range 2`). |
