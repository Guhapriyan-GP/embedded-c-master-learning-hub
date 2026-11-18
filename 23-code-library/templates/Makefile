# ----------------------------------------------------------------------------
# Production Bare-Metal Makefile for STM32G474RE (NUCLEO-G474RE, MB1367)
# Supports: BUILD_TYPE=debug (-Og -g3) | BUILD_TYPE=release (-O2 -g)
# Optional: TOOLCHAIN_BIN=/path/to/arm-gnu-toolchain/bin
# ----------------------------------------------------------------------------

TARGET      ?= firmware
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
READELF     := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)readelf

# Source files (application + reusable bare-metal drivers)
APP_SRCS    := Src/main.c \
               Src/startup_stm32g474re.c \
               Src/system_stm32g4xx.c

DRV_SRCS    := ../Drivers/clock_g4.c \
               ../Drivers/gpio_g4.c \
               ../Drivers/ring_buffer.c \
               ../Drivers/fault_handler.c

# Object mapping preserving directory structure (avoids flat filename collisions)
APP_OBJS    := $(patsubst Src/%.c,$(BUILD_DIR)/obj/src/%.o,$(APP_SRCS))
DRV_OBJS    := $(patsubst ../Drivers/%.c,$(BUILD_DIR)/obj/drivers/%.o,$(DRV_SRCS))
OBJS        := $(APP_OBJS) $(DRV_OBJS)
DEPS        := $(OBJS:.o=.d)

# Include directories (CMSIS marked -isystem so strict warnings target user/driver code)
INCLUDES    := -IInc \
               -I../Drivers \
               -isystem ../Drivers/CMSIS/Device/ST/STM32G4xx/Include \
               -isystem ../Drivers/CMSIS/Core/Include

# Cortex-M4F architecture flags (ARMv7E-M, FPv4-SP-D16 Hard-Float ABI)
MCU_FLAGS   := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Build profile flags
ifeq ($(BUILD_TYPE),release)
    OPT_FLAGS := -O2 -g -DNDEBUG
else ifeq ($(BUILD_TYPE),debug)
    OPT_FLAGS := -Og -g3
else
    $(error Unsupported BUILD_TYPE='$(BUILD_TYPE)'. Use 'debug' or 'release')
endif

# Strict C11 warning & diagnostics flags
WARN_FLAGS  := -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef

CFLAGS      := -std=c11 $(MCU_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) \
               $(INCLUDES) -DSTM32G474xx \
               -ffunction-sections -fdata-sections -fstack-usage \
               -MMD -MP

# Linker script & flags (newlib-nano + nosys, garbage collection, map + memory usage)
LDSCRIPT    := stm32g474re.ld
LDFLAGS     := $(MCU_FLAGS) -T $(LDSCRIPT) \
               --specs=nano.specs --specs=nosys.specs \
               -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
               -Wl,--gc-sections \
               -Wl,--print-memory-usage

# OpenOCD configuration (upstream OpenOCD >= 0.12 defaults for NUCLEO-G474RE STLINK-V3E)
OPENOCD         ?= openocd
OPENOCD_IFACE   ?= interface/stlink.cfg
OPENOCD_TARGET  ?= target/stm32g4x.cfg
OPENOCD_EXTRA   ?=

# Cross-platform directory creation & cleanup helpers (handles POSIX sh on Windows/Linux AND cmd.exe)
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
.PHONY: all disasm size flash flash-openocd erase clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/obj/src/%.o: Src/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:$(BUILD_TYPE)] $<
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/drivers/%.o: ../Drivers/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:$(BUILD_TYPE)] $<
	$(CC) $(CFLAGS) -c $< -o $@

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

# ----------------------------------------------------------------------------
# Flashing & Erase Targets (NUCLEO-G474RE with onboard STLINK-V3E)
# ----------------------------------------------------------------------------
flash: all
	STM32_Programmer_CLI -c port=SWD -w $(BUILD_DIR)/$(TARGET).bin 0x08000000 -v -rst

flash-openocd: all
	$(OPENOCD) $(OPENOCD_EXTRA) -f $(OPENOCD_IFACE) -f $(OPENOCD_TARGET) \
		-c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

erase:
	STM32_Programmer_CLI -c port=SWD -e all

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@echo Cleaned $(BUILD_DIR).

# Include compiler-generated header dependency files
-include $(DEPS)
