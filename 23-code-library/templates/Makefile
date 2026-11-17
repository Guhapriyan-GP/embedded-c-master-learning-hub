# ----------------------------------------------------------------------------
# Makefile for STM32G474RE Bare-Metal Embedded C (NUCLEO-G474RE)
# ----------------------------------------------------------------------------

TARGET      = firmware
BUILD_DIR   = build

# Source Files & Includes
SRCS        = Src/main.c Src/startup_stm32g474re.c Src/system_stm32g4xx.c
INCLUDES    = -IInc \
              -IDrivers/CMSIS/Device/ST/STM32G4xx/Include \
              -IDrivers/CMSIS/Core/Include

# Toolchain Definitions
CC          = arm-none-eabi-gcc
CP          = arm-none-eabi-objcopy
SZ          = arm-none-eabi-size
HEX         = $(CP) -O ihex
BIN         = $(CP) -O binary

# Architecture & Compiler Flags
CPU         = -mcpu=cortex-m4
FPU         = -mfpu=fpv4-sp-d16
FLOAT_ABI   = -mfloat-abi=hard
MCU_FLAGS   = $(CPU) -mthumb $(FPU) $(FLOAT_ABI)

CFLAGS      = $(MCU_FLAGS) $(INCLUDES) -DSTM32G474xx -O0 -g3 -Wall -Wextra -std=gnu11 -fdata-sections -ffunction-sections

# Linker Script & Flags
LDSCRIPT    = stm32g474re.ld
LDFLAGS     = $(MCU_FLAGS) -T $(LDSCRIPT) -nostdlib -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -Wl,--gc-sections

# Objects
OBJS        = $(addprefix $(BUILD_DIR)/, $(notdir $(SRCS:.c=.o)))
vpath %.c $(sort $(dir $(SRCS)))

# OpenOCD Configuration
OPENOCD_SCRIPTS = C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.debug.openocd_2.3.400.202606220929/resources/openocd/st_scripts

# ----------------------------------------------------------------------------
# Build Rules
# ----------------------------------------------------------------------------
all: $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo [CC] $<
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@echo [LD] $@
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	@echo [SIZE]
	$(SZ) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo [HEX] $@
	$(HEX) $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo [BIN] $@
	$(BIN) $< $@
	@echo [BUILD SUCCESSFUL]

# ----------------------------------------------------------------------------
# Flashing & Debugging Targets
# ----------------------------------------------------------------------------
# Flash via STM32CubeProgrammer CLI (Standard ST-LINK)
flash: all
	STM32_Programmer_CLI -c port=SWD -w $(BUILD_DIR)/$(TARGET).bin 0x08000000 -v -rst

# Flash via OpenOCD
flash-openocd: all
	openocd -s "$(OPENOCD_SCRIPTS)" -f interface/stlink-dap.cfg -f target/stm32g4x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# Erase Chip
erase:
	STM32_Programmer_CLI -c port=SWD -e all

# Clean Build Artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo Cleaned build artifacts.

.PHONY: all clean flash flash-openocd erase
