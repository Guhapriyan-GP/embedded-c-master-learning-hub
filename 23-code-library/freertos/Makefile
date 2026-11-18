# ----------------------------------------------------------------------------
# Reproducible FreeRTOS V11.1.0 Firmware Makefile for NUCLEO-G474RE (MB1367)
# Target: STM32G474RET6 (ARM Cortex-M4F, FPv4-SP-D16, 170 MHz Boost Mode)
# Supports: BUILD_TYPE=debug (-Og -g3) | BUILD_TYPE=release (-O2 -g)
# ----------------------------------------------------------------------------

TARGET      ?= nucleo_g474re_freertos
BUILD_TYPE  ?= debug
BUILD_DIR   ?= build

# Portable toolchain discovery (PATH first, optional TOOLCHAIN_BIN, then ST CubeIDE fallback)
TOOLCHAIN_PREFIX ?= arm-none-eabi-
ifdef TOOLCHAIN_BIN
    TOOLCHAIN_PATH := $(TOOLCHAIN_BIN)/
else
    ifeq ($(OS),Windows_NT)
        GCC_ON_PATH := $(shell where $(TOOLCHAIN_PREFIX)gcc.exe 2>NUL)
    else
        GCC_ON_PATH := $(shell command -v $(TOOLCHAIN_PREFIX)gcc 2>/dev/null)
    endif
    ifeq ($(strip $(GCC_ON_PATH)),)
        ST_GCC_EXE := $(firstword $(wildcard C:/ST/STM32CubeIDE_*/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin/$(TOOLCHAIN_PREFIX)gcc.exe))
        TOOLCHAIN_PATH := $(dir $(ST_GCC_EXE))
    else
        TOOLCHAIN_PATH :=
    endif
endif

CC          := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)gcc
OBJCOPY     := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objcopy
OBJDUMP     := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objdump
SIZE        := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)size
NM          := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)nm

# User application sources (compiled with strict -Wall -Wextra -Werror)
APP_SRCS    := Src/main.c \
               Src/rtos_bsp.c

# Shared verified STM32G474RE startup & CMSIS system initialization
TMPL_SRCS   := ../templates/Src/startup_stm32g474re.c \
               ../templates/Src/system_stm32g4xx.c

# Official pinned FreeRTOS-Kernel V11.1.0 sources (GCC/ARM_CM4F + heap_4)
RTOS_DIR    := FreeRTOS-Kernel
RTOS_SRCS   := $(RTOS_DIR)/tasks.c \
               $(RTOS_DIR)/queue.c \
               $(RTOS_DIR)/list.c \
               $(RTOS_DIR)/timers.c \
               $(RTOS_DIR)/event_groups.c \
               $(RTOS_DIR)/stream_buffer.c \
               $(RTOS_DIR)/portable/GCC/ARM_CM4F/port.c \
               $(RTOS_DIR)/portable/MemMang/heap_4.c

# Object mapping preserving directory structure
APP_OBJS    := $(patsubst Src/%.c,$(BUILD_DIR)/obj/app/%.o,$(APP_SRCS))
TMPL_OBJS   := $(patsubst ../templates/Src/%.c,$(BUILD_DIR)/obj/tmpl/%.o,$(TMPL_SRCS))
RTOS_OBJS   := $(patsubst $(RTOS_DIR)/%.c,$(BUILD_DIR)/obj/freertos/%.o,$(RTOS_SRCS))
OBJS        := $(APP_OBJS) $(TMPL_OBJS) $(RTOS_OBJS)
DEPS        := $(OBJS:.o=.d)

# Include paths (CMSIS & FreeRTOS headers marked -isystem for user strictness)
INCLUDES    := -IInc \
               -isystem $(RTOS_DIR)/include \
               -isystem $(RTOS_DIR)/portable/GCC/ARM_CM4F \
               -isystem ../Drivers/CMSIS/Device/ST/STM32G4xx/Include \
               -isystem ../Drivers/CMSIS/Core/Include

# Cortex-M4F architecture flags (ARMv7E-M, FPv4-SP-D16 Hard-Float ABI)
MCU_FLAGS   := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

ifeq ($(BUILD_TYPE),release)
    OPT_FLAGS := -O2 -g -DNDEBUG
else ifeq ($(BUILD_TYPE),debug)
    OPT_FLAGS := -Og -g3
else
    $(error Unsupported BUILD_TYPE='$(BUILD_TYPE)'. Use 'debug' or 'release')
endif

COMMON_CFLAGS := -std=c11 $(MCU_FLAGS) $(OPT_FLAGS) $(INCLUDES) -DSTM32G474xx \
                 -ffunction-sections -fdata-sections -fstack-usage -MMD -MP

# Strict warnings enforced on all user application code
APP_WARN    := -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef
# Clean warnings on upstream FreeRTOS-Kernel sources
RTOS_WARN   := -Wall -Wextra

# Shared verified STM32G474RE linker script (512 KB Flash, 96 KB SRAM1+2, 32 KB CCM)
LDSCRIPT    := ../templates/stm32g474re.ld
LDFLAGS     := $(MCU_FLAGS) -T $(LDSCRIPT) \
               --specs=nano.specs --specs=nosys.specs \
               -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
               -Wl,--gc-sections \
               -Wl,--print-memory-usage

# OpenOCD configuration (NUCLEO-G474RE onboard STLINK-V3E)
OPENOCD         ?= openocd
OPENOCD_IFACE   ?= interface/stlink.cfg
OPENOCD_TARGET  ?= target/stm32g4x.cfg

# Cross-platform directory creation & cleanup helpers
ifeq ($(findstring sh,$(notdir $(SHELL))),)
    MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM_RF   = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
else
    MKDIR_P = mkdir -p "$(1)"
    RM_RF   = rm -rf "$(1)"
endif

# ----------------------------------------------------------------------------
# Build Targets
# ----------------------------------------------------------------------------
.PHONY: all disasm size nm-check flash flash-openocd clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/obj/app/%.o: Src/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:APP:$(BUILD_TYPE)] $<
	$(CC) $(COMMON_CFLAGS) $(APP_WARN) -c $< -o $@

$(BUILD_DIR)/obj/tmpl/%.o: ../templates/Src/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:TMPL:$(BUILD_TYPE)] $<
	$(CC) $(COMMON_CFLAGS) $(APP_WARN) -c $< -o $@

$(BUILD_DIR)/obj/freertos/%.o: $(RTOS_DIR)/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:RTOS:$(BUILD_TYPE)] $<
	$(CC) $(COMMON_CFLAGS) $(RTOS_WARN) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@$(call MKDIR_P,$(dir $@))
	@echo [LD:$(BUILD_TYPE)] $@
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo [HEX] $@
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo [BIN] $@
	$(OBJCOPY) -O binary $< $@

disasm: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -h -S -d $< > $(BUILD_DIR)/$(TARGET).lss
	@echo [LSS] $(BUILD_DIR)/$(TARGET).lss

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) -A $<

nm-check: $(BUILD_DIR)/$(TARGET).elf
	$(NM) -n $< | grep -E " (SVC_Handler|PendSV_Handler|SysTick_Handler|TIM2_IRQHandler|EXTI15_10_IRQHandler|vTaskStartScheduler)$$"

flash: all
	STM32_Programmer_CLI -c port=SWD -w $(BUILD_DIR)/$(TARGET).bin 0x08000000 -v -rst

flash-openocd: all
	$(OPENOCD) -f $(OPENOCD_IFACE) -f $(OPENOCD_TARGET) \
		-c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@echo Cleaned $(BUILD_DIR).

-include $(DEPS)
