# Stage 13: Embedded Memory Subsystems, Flash & Bootloaders

## 1. Prerequisites
Stage 05 (Toolchain & Linker scripts).

## 2. Core Technical Theory & Engineering Principles
Embedded systems require robust management of volatile SRAM and non-volatile Flash memory for runtime data, persistent configurations, and firmware updates.

### 1. Embedded Flash Architecture
The STM32G474RE features 512 KB of Dual-Bank Flash memory:
- **Pages & Sectors:** Organized into 2 KB pages. Writing data requires erasing an entire page first.
- **Endurance:** Minimum 10,000 erase/write cycles.
- **Dual-Bank Execution:** Enables Live Firmware Updates (Read-While-Write), where code executes from Bank 1 while a new firmware image is written into Bank 2.

### 2. EEPROM Emulation
Microcontrollers without dedicated EEPROM use software wear-leveling algorithms across Flash pages to store non-volatile configuration parameters without burning out individual memory cells prematurely.

### 3. In-Application Programming (IAP) & Bootloaders
A custom bootloader resides at the start of Flash (`0x08000000`). It checks for a valid application image in upper memory, verifies cryptographic signatures or checksums, and branches to the application:
1. Relocate Vector Table: `SCB->VTOR = APPLICATION_ADDRESS;`
2. Set Main Stack Pointer: `__set_MSP(*(volatile uint32_t *)APPLICATION_ADDRESS);`
3. Branch to Application Reset_Handler: Call function pointer stored at `APPLICATION_ADDRESS + 4`.

## 3. Practical Coding Exercises & Challenges
1. Write a bare-metal routine to unlock, erase, and write a Flash memory page.
2. Implement a simple EEPROM emulation key-value store with wear leveling.
3. Build a two-stage bootloader that jumps to an application image at offset `0x08010000`.

## 4. Expected Learning Outcomes & Mastery Criteria
Ability to design custom bootloaders, flash drivers, and secure non-volatile storage routines.
