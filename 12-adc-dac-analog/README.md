# Stage 11: High-Speed Analog Signal Acquisition (ADC & DAC)

## 1. Prerequisites
Stage 10 (Serial protocols) & Stage 07 (GPIO hardware).

## 2. Core Technical Theory & Engineering Principles
The STM32G474RE contains an exceptional mixed-signal analog engine featuring up to five 12-bit SAR ADCs, digital-to-analog converters (DACs), analog comparators (COMP), and programmable operational amplifiers (OPAMPs).

### 1. Successive Approximation Register (SAR) ADC
A SAR ADC digitizes analog voltages by performing a binary search comparison against an internal reference voltage ($V_{	ext{REF+}}$):
- **Resolution:** 12 bits ($2^{12} = 4096$ discrete voltage steps).
- **Quantization Step Size (LSB):**
  $$	ext{LSB} = rac{V_{	ext{REF+}}}{4096} pprox rac{3.3\ 	ext{V}}{4096} pprox 0.805\ 	ext{mV}$$
- **Total Conversion Time:**
  $$t_{	ext{CONV}} = t_{	ext{SMP}} + 12.5 	ext{ ADC clock cycles}$$
  Sampling time must be chosen sufficiently long to fully charge the internal sample-and-hold capacitor based on external input source impedance.

### 2. Hardware Calibration Sequence
Before enabling an ADC on STM32G4, an internal calibration routine (`ADC_CR.ADCAL`) must be executed with the ADC disabled to nullify offset errors.

### 3. DAC & Mixed-Signal Peripherals
- **Digital-to-Analog Converter (DAC):** Converts 12-bit digital values to continuous output voltages.
- **Analog Comparators (COMP):** Ultra-fast comparison of input voltages against internal DAC thresholds, capable of triggering emergency PWM shutdown on overcurrent.

## 3. Practical Coding Exercises & Challenges
1. Calibrate ADC1 and perform continuous conversions on PA0.
2. Implement a timer-triggered ADC conversion sequence to eliminate software jitter.
3. Configure an analog watchdog to generate an interrupt when an input voltage exceeds a safety threshold.

## 4. Expected Learning Outcomes & Mastery Criteria
Competency in configuring high-speed mixed-signal acquisition, calibration, and analog safety systems.
