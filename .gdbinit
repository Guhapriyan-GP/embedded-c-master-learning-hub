# ----------------------------------------------------------------------------
# CLI GDB Initialization Script for STM32G474RE (NUCLEO-G474RE + OpenOCD)
# Usage: arm-none-eabi-gdb -x .gdbinit 23-code-library/templates/build/firmware.elf
# ----------------------------------------------------------------------------

# Connect to OpenOCD GDB server (launch: openocd -f interface/stlink.cfg -f target/stm32g4x.cfg)
target extended-remote localhost:3333

# Cortex-M4 hardware FPB / DWT limits (ARM DDI 0439F)
set remote hardware-breakpoint-limit 6
set remote hardware-watchpoint-limit 4

set print pretty on
set output-radix 16
set pagination off

define flash_run
    monitor reset halt
    load
    monitor reset halt
    thbreak main
    continue
end
document flash_run
Reset target, program ELF to Flash, halt at Reset, and run to main().
end

define fault_regs
    printf "SCB->CFSR  (0xE000ED28) = 0x%08x\n", *(unsigned int*)0xE000ED28
    printf "SCB->HFSR  (0xE000ED2C) = 0x%08x\n", *(unsigned int*)0xE000ED2C
    printf "SCB->DFSR  (0xE000ED30) = 0x%08x\n", *(unsigned int*)0xE000ED30
    printf "SCB->MMFAR (0xE000ED34) = 0x%08x\n", *(unsigned int*)0xE000ED34
    printf "SCB->BFAR  (0xE000ED38) = 0x%08x\n", *(unsigned int*)0xE000ED38
    printf "MSP = 0x%08x  PSP = 0x%08x  LR = 0x%08x  PC = 0x%08x\n", $msp, $psp, $lr, $pc
end
document fault_regs
Dump Cortex-M4 SCB fault status/address registers and active stack pointers.
end

define crash_log
    print /x g_crash_log
end
document crash_log
Inspect the persistent .noinit HardFault crash record (g_crash_log).
end

define clock_regs
    printf "RCC->CR      (0x40021000) = 0x%08x\n", *(unsigned int*)0x40021000
    printf "RCC->CFGR    (0x40021008) = 0x%08x\n", *(unsigned int*)0x40021008
    printf "RCC->PLLCFGR (0x4002100C) = 0x%08x\n", *(unsigned int*)0x4002100C
    printf "PWR->CR1     (0x40007000) = 0x%08x\n", *(unsigned int*)0x40007000
    printf "PWR->SR2     (0x40007014) = 0x%08x\n", *(unsigned int*)0x40007014
    printf "PWR->CR5     (0x40007080) = 0x%08x\n", *(unsigned int*)0x40007080
    printf "FLASH->ACR   (0x40022000) = 0x%08x\n", *(unsigned int*)0x40022000
end
document clock_regs
Dump STM32G474RE RCC, PWR, and FLASH registers for 170 MHz Boost Mode verification.
end
