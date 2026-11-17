# Stage 01 Mastery Exam: C Foundations & Memory Mechanics

### Part A: Theory & Concept Questions (50 Points)
1. **Two's Complement Representation:** What is the 8-bit two's complement binary representation of `-42`? Show the step-by-step bitwise derivation.
2. **Pointer Arithmetic:** Given `uint32_t *ptr = (uint32_t *)0x20000000;`, what is the exact physical address represented by `ptr + 4`? Explain the scaling rule.
3. **Struct Padding & Alignment:** On a 32-bit ARM architecture, what is the exact `sizeof` the following struct? Draw the byte layout including padding:
   ```c
   struct SensorData {
       uint8_t id;
       uint32_t timestamp;
       uint16_t raw_value;
   };
   ```
4. **Bitmasking:** Write a single C expression using bitwise operators to clear bits [11:10] and set bit 10 in a 32-bit register `REG` without altering any other bits.
5. **Storage Classes:** Contrast `static` when applied to a local variable inside a function versus when applied to a global variable at file scope.

---

### Part B: Practical Coding Challenge (50 Points)
Write an interrupt-safe C function `uint32_t pack_telemetry(uint8_t status, uint16_t voltage_mv, uint8_t checksum)` that packs these fields into a single 32-bit unsigned integer according to this specification:
- Bits [31:24]: Status (8 bits)
- Bits [23:8]: Voltage in millivolts (16 bits)
- Bits [7:0]: Checksum (8 bits)
Provide the corresponding unpacking macros.
