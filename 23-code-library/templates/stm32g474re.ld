/*
 * Linker Script for STM32G474RET6 (NUCLEO-G474RE, MB1367)
 * ARM Cortex-M4F (ARMv7E-M, FPv4-SP-D16)
 *
 * Verified Physical Memory Map (RM0440 Section 2.2, DS12288, stm32g474xx.h):
 *   - FLASH  : 0x08000000 .. 0x0807FFFF (512 KB, Dual-Bank / Single-Bank)
 *   - SRAM1  : 0x20000000 .. 0x20013FFF ( 80 KB, S-Bus & DMA accessible)
 *   - SRAM2  : 0x20014000 .. 0x20017FFF ( 16 KB, contiguous after SRAM1, parity)
 *              -> Combined contiguous RAM = 96 KB (0x20000000 .. 0x20017FFF)
 *   - CCMRAM : 0x10000000 .. 0x10007FFF ( 32 KB, I-Code & D-Code 0-WS execution)
 *              Note: Aliased on S-Bus at 0x20018000 .. 0x2001FFFF for DMA access;
 *              RAM LENGTH is intentionally kept at 96K so _estack (0x20018000)
 *              never collides with CCMRAM placed at 0x10000000.
 */

ENTRY(Reset_Handler)

/* Highest address of the main stack (8-byte aligned per ARM AAPCS) */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* 0x20018000 */

_Min_Heap_Size  = 0x200;                /* 512 B minimum heap reservation  */
_Min_Stack_Size = 0x400;                /* 1 KB minimum stack reservation  */

MEMORY
{
    FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM     (xrw) : ORIGIN = 0x20000000, LENGTH = 96K
    CCMRAM  (xrw) : ORIGIN = 0x10000000, LENGTH = 32K
}

SECTIONS
{
    /* Interrupt vector table: 16 core + 102 STM32G474RE external IRQs = 472 bytes.
     * Cortex-M4 SCB->VTOR requires power-of-2 alignment >= table size (512 bytes). */
    .isr_vector :
    {
        . = ALIGN(512);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code in FLASH */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Read-only constant data in FLASH */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM exception unwinding tables */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    /* C/C++ runtime constructor & destructor arrays (invoked by __libc_init_array) */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH

    /* Load Memory Address (LMA) of .data in FLASH */
    _sidata = LOADADDR(.data);

    /* Initialized data copied from FLASH LMA to RAM VMA by Reset_Handler */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.RamFunc)
        *(.RamFunc*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* Load Memory Address (LMA) of .ccmram in FLASH */
    _siccmram = LOADADDR(.ccmram);

    /* Zero-wait-state CCM-SRAM code/data copied from FLASH LMA to 0x10000000 by Reset_Handler */
    .ccmram :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } > CCMRAM AT> FLASH

    /* Zero-initialized data section in RAM */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* Uninitialized persistent RAM section (NOT cleared on reset; stores HardFault crash records) */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        _snoinit = .;
        KEEP(*(.noinit))
        KEEP(*(.noinit*))
        . = ALIGN(4);
        _enoinit = .;
    } > RAM

    /* Minimum heap and stack space verification (NOLOAD prevents binary bloat) */
    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* Link-time safety assertions */
ASSERT((_estack % 8) == 0, "Error: _estack must be 8-byte aligned per ARM AAPCS")
ASSERT((ADDR(.isr_vector) % 512) == 0, "Error: .isr_vector must be 512-byte aligned for Cortex-M4 SCB->VTOR")
ASSERT(SIZEOF(.isr_vector) == 472, "Error: .isr_vector must be exactly 472 bytes (16 core + 102 STM32G474RE external IRQs)")
ASSERT((_enoinit + _Min_Heap_Size + _Min_Stack_Size) <= _estack, "Error: RAM overflow - static RAM + .noinit + min heap + min stack exceeds 96 KB")
