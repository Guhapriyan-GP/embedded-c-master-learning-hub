# Stage 01 Mastery Exam: Digital Primer, C Foundations & Memory Mechanics

**Aligned Stages:** Stage 00–01 (`02-c-fundamentals/`)  
**Concept IDs:** `DIG-ELEC-001`, `C-TYPE-001`, `C-BIT-001`, `C-PTR-001`, `C-STRUCT-001`, `C-QUAL-001`, `C-FPTR-001`, `PROF-STD-002`  
**Total Points:** 100 | **Pass Threshold:** $\ge 85$ Points | **Answer Key:** [`answer-keys-and-rubrics.md`](answer-keys-and-rubrics.md#stage-01-answer-key--rubric)

---

### Part A: Theory, Representation & C Semantics (60 Points)

1. **Two's Complement Representation (10 Points):**
   Derive the exact 8-bit two's complement binary and hexadecimal representation of `-42` step-by-step. What is the valid signed range of `int8_t`?

2. **Pointer Arithmetic & Address Scaling (10 Points):**
   Given `uint32_t *ptr = (uint32_t *)0x20000000UL;`, what is the exact physical address of `ptr + 4`? What is the exact byte difference `(uint8_t *)(ptr + 4) - (uint8_t *)ptr`?

3. **Struct Alignment, Padding & Reordering (10 Points):**
   On a 32-bit ARM Cortex-M4 (AAPCS natural alignment), compute `sizeof(struct SensorData)` and draw the byte-by-byte memory layout including padding bytes:
   ```c
   struct SensorData {
       uint8_t  id;
       uint32_t timestamp;
       uint16_t raw_value;
   };
   ```
   Then reorder the fields to minimize `sizeof` **without** using `__attribute__((packed))`, and state the new size.

4. **Bitmasking & Unsigned Literal Discipline (10 Points):**
   Write a single MISRA/Barr-compliant C statement that clears bits `[11:10]` and sets bit `10` (`01b`) in a 32-bit unsigned variable `reg` without modifying any other bits. Explain why `(1 << 31)` is Undefined Behavior in ISO C while `(1UL << 31U)` is safe.

5. **Unsigned Underflow & SEI CERT C `INT30-C` (10 Points):**
   Identify the exact runtime bug in the following PWM ramp-down loop and rewrite it safely:
   ```c
   for (uint32_t duty = 6799U; duty > 0U; duty -= 50U) {
       set_pwm(duty);
   }
   ```

6. **Storage Classes & Function Pointers (10 Points):**
   (a) Contrast `static` on a function-local variable versus a file-scope global variable.
   (b) Declare a `typedef` named `event_handler_t` for a function pointer taking `(uint32_t event_id, void *ctx)` and returning `int32_t`, and show how an array of 4 handlers is declared as `const` in Flash `.rodata`.

---

### Part B: Practical Coding & Hardware Interfacing Challenge (40 Points)

7. **Telemetry Bit-Packing & Unpacking (25 Points):**
   Write a MISRA-compliant C function `uint32_t pack_telemetry(uint8_t status, uint16_t voltage_mv, uint8_t checksum)` packing:
   - Bits `[31:24]`: `status` (8 bits)
   - Bits `[23:8]`: `voltage_mv` (16 bits)
   - Bits `[7:0]`: `checksum` (8 bits)
   Provide three safe unpacking functions or parenthesized macros (`UNPACK_STATUS`, `UNPACK_VOLTAGE`, `UNPACK_CHECKSUM`) that avoid implicit signed promotions.

8. **DC Electrical Interfacing Math (15 Points):**
   A green LED ($V_f = 2.1\ \text{V}$, target $I_f = 6\ \text{mA}$) is driven by an STM32G474RE push-pull output at $V_{\text{DD}} = 3.3\ \text{V}$. Compute the required series resistor $R$, check against DS12288 Table 15 I/O pin current limits ($\pm 20\ \text{mA}$), and compute the resistor power dissipation.
