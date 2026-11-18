# ----------------------------------------------------------------------------
# NUCLEO-G474RE Safe Onboard Hardware Self-Test Suite (9 Self-Contained Tests)
# Uses real 23-code-library/Drivers/ + 23-code-library/templates/
# Zero external wiring required (onboard LD2 PA5, B1 PC13, STLINK VCP USART2).
# ----------------------------------------------------------------------------

TARGET      := onboard_selftest
BUILD_TYPE  ?= debug
BUILD_DIR   ?= build

ROOT_DIR    := ../..
DRV_DIR     := $(ROOT_DIR)/23-code-library/Drivers
TPL_DIR     := $(ROOT_DIR)/23-code-library/templates

TOOLCHAIN_PREFIX ?= arm-none-eabi-
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

CC      := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objcopy
SIZE    := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)size

APP_SRCS := Src/main.c
TPL_SRCS := $(TPL_DIR)/Src/startup_stm32g474re.c \
            $(TPL_DIR)/Src/system_stm32g4xx.c
DRV_SRCS := $(DRV_DIR)/clock_g4.c \
            $(DRV_DIR)/gpio_g4.c \
            $(DRV_DIR)/usart_g4.c \
            $(DRV_DIR)/tim_g4.c \
            $(DRV_DIR)/exti_g4.c \
            $(DRV_DIR)/adc_g4.c \
            $(DRV_DIR)/dma_g4.c \
            $(DRV_DIR)/iwdg_g4.c \
            $(DRV_DIR)/ring_buffer.c \
            $(DRV_DIR)/fault_handler.c

APP_OBJS := $(patsubst Src/%.c,$(BUILD_DIR)/obj/app/%.o,$(APP_SRCS))
TPL_OBJS := $(patsubst $(TPL_DIR)/Src/%.c,$(BUILD_DIR)/obj/tpl/%.o,$(TPL_SRCS))
DRV_OBJS := $(patsubst $(DRV_DIR)/%.c,$(BUILD_DIR)/obj/drv/%.o,$(DRV_SRCS))
OBJS     := $(APP_OBJS) $(TPL_OBJS) $(DRV_OBJS)
DEPS     := $(OBJS:.o=.d)

INCLUDES := -I$(DRV_DIR) \
            -isystem $(DRV_DIR)/CMSIS/Device/ST/STM32G4xx/Include \
            -isystem $(DRV_DIR)/CMSIS/Core/Include

MCU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

ifeq ($(BUILD_TYPE),release)
    OPT_FLAGS := -O2 -g -DNDEBUG
else
    OPT_FLAGS := -Og -g3
endif

WARN_FLAGS := -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef

CFLAGS := -std=c11 $(MCU_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) \
          $(INCLUDES) -DSTM32G474xx \
          -ffunction-sections -fdata-sections -fstack-usage \
          -MMD -MP

LDSCRIPT := $(TPL_DIR)/stm32g474re.ld

LDFLAGS := $(MCU_FLAGS) -T $(LDSCRIPT) \
           --specs=nano.specs --specs=nosys.specs \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
           -Wl,--gc-sections \
           -Wl,--print-memory-usage

LDLIBS  := -lm

ifeq ($(findstring sh,$(notdir $(SHELL))),)
    MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM_RF   = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
else
    MKDIR_P = mkdir -p "$(1)"
    RM_RF   = rm -rf "$(1)"
endif

.PHONY: all clean flash

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/obj/app/%.o: Src/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC] $<
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/tpl/%.o: $(TPL_DIR)/Src/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC] $<
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/drv/%.o: $(DRV_DIR)/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC] $<
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@$(call MKDIR_P,$(dir $@))
	@echo [LD] $@
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: all
	STM32_Programmer_CLI -c port=SWD mode=NORMAL -w $(BUILD_DIR)/$(TARGET).elf -v -rst

clean:
	@$(call RM_RF,$(BUILD_DIR))

-include $(DEPS)
