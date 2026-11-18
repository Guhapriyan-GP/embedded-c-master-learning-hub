# ----------------------------------------------------------------------------
# Project 03 — FreeRTOS Digital Power & Motor Controller (STM32G474RE)
# Links official pinned FreeRTOS-Kernel V11.1.0 (ARM_CM4F + heap_4.c)
# Supports:
#   BUILD_TYPE=debug (-Og -g3) | BUILD_TYPE=release (-O2 -g)
#   POWER_SIM_MODE=1 (default onboard safe sim) | POWER_SIM_MODE=0 (ext stage)
# ----------------------------------------------------------------------------

TARGET         ?= freertos_digital_power_ctrl
BUILD_TYPE     ?= debug
BUILD_DIR      ?= build
POWER_SIM_MODE ?= 1

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

CC      := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objcopy
OBJDUMP := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objdump
SIZE    := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)size
NM      := $(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)nm

LIB_ROOT := ../../23-code-library
DRV_DIR  := $(LIB_ROOT)/Drivers
TMPL_DIR := $(LIB_ROOT)/templates
RTOS_DIR := $(LIB_ROOT)/freertos/FreeRTOS-Kernel

APP_SRCS  := Src/main.c
TMPL_SRCS := $(TMPL_DIR)/Src/startup_stm32g474re.c \
             $(TMPL_DIR)/Src/system_stm32g4xx.c
DRV_SRCS  := $(DRV_DIR)/clock_g4.c \
             $(DRV_DIR)/gpio_g4.c \
             $(DRV_DIR)/adc_g4.c \
             $(DRV_DIR)/usart_g4.c \
             $(DRV_DIR)/ring_buffer.c \
             $(DRV_DIR)/i2c_g4.c \
             $(DRV_DIR)/iwdg_g4.c \
             $(DRV_DIR)/fault_handler.c

# Official pinned FreeRTOS-Kernel V11.1.0 (GCC/ARM_CM4F + heap_4.c)
RTOS_SRCS := $(RTOS_DIR)/tasks.c \
             $(RTOS_DIR)/queue.c \
             $(RTOS_DIR)/list.c \
             $(RTOS_DIR)/timers.c \
             $(RTOS_DIR)/event_groups.c \
             $(RTOS_DIR)/stream_buffer.c \
             $(RTOS_DIR)/portable/GCC/ARM_CM4F/port.c \
             $(RTOS_DIR)/portable/MemMang/heap_4.c

APP_OBJS  := $(patsubst Src/%.c,$(BUILD_DIR)/obj/app/%.o,$(APP_SRCS))
TMPL_OBJS := $(patsubst $(TMPL_DIR)/Src/%.c,$(BUILD_DIR)/obj/tmpl/%.o,$(TMPL_SRCS))
DRV_OBJS  := $(patsubst $(DRV_DIR)/%.c,$(BUILD_DIR)/obj/drv/%.o,$(DRV_SRCS))
RTOS_OBJS := $(patsubst $(RTOS_DIR)/%.c,$(BUILD_DIR)/obj/rtos/%.o,$(RTOS_SRCS))
OBJS      := $(APP_OBJS) $(TMPL_OBJS) $(DRV_OBJS) $(RTOS_OBJS)
DEPS      := $(OBJS:.o=.d)

INCLUDES := -IInc \
            -I$(DRV_DIR) \
            -isystem $(RTOS_DIR)/include \
            -isystem $(RTOS_DIR)/portable/GCC/ARM_CM4F \
            -isystem $(DRV_DIR)/CMSIS/Device/ST/STM32G4xx/Include \
            -isystem $(DRV_DIR)/CMSIS/Core/Include

MCU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

ifeq ($(BUILD_TYPE),release)
    OPT_FLAGS := -O2 -g -DNDEBUG
else ifeq ($(BUILD_TYPE),debug)
    OPT_FLAGS := -Og -g3
else
    $(error Unsupported BUILD_TYPE='$(BUILD_TYPE)'. Use 'debug' or 'release')
endif

COMMON_CFLAGS := -std=c11 $(MCU_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
                 -DSTM32G474xx -DPOWER_SIM_MODE=$(POWER_SIM_MODE) $(EXTRA_CFLAGS) \
                 -ffunction-sections -fdata-sections -fstack-usage -MMD -MP

# Strict C11 warnings enforced on all project, template, and driver code
APP_WARN  := -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wundef
# Clean warnings on upstream FreeRTOS-Kernel V11.1.0 sources
RTOS_WARN := -Wall -Wextra

LDSCRIPT := $(TMPL_DIR)/stm32g474re.ld
LDFLAGS  := $(MCU_FLAGS) -T $(LDSCRIPT) \
            --specs=nano.specs --specs=nosys.specs \
            -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
            -Wl,--gc-sections \
            -Wl,--print-memory-usage

ifeq ($(findstring sh,$(notdir $(SHELL))),)
    MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM_RF   = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
else
    MKDIR_P = mkdir -p "$(1)"
    RM_RF   = rm -rf "$(1)"
endif

.PHONY: all disasm size nm-check flash clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/obj/app/%.o: Src/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:APP:$(BUILD_TYPE)] $<
	$(CC) $(COMMON_CFLAGS) $(APP_WARN) -c $< -o $@

$(BUILD_DIR)/obj/tmpl/%.o: $(TMPL_DIR)/Src/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:TMPL:$(BUILD_TYPE)] $<
	$(CC) $(COMMON_CFLAGS) $(APP_WARN) -c $< -o $@

$(BUILD_DIR)/obj/drv/%.o: $(DRV_DIR)/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:DRV:$(BUILD_TYPE)] $<
	$(CC) $(COMMON_CFLAGS) $(APP_WARN) -c $< -o $@

$(BUILD_DIR)/obj/rtos/%.o: $(RTOS_DIR)/%.c
	@$(call MKDIR_P,$(dir $@))
	@echo [CC:RTOS:$(BUILD_TYPE)] $<
	$(CC) $(COMMON_CFLAGS) $(RTOS_WARN) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@$(call MKDIR_P,$(dir $@))
	@echo [LD:$(BUILD_TYPE)] $@
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

disasm: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -h -S -d $< > $(BUILD_DIR)/$(TARGET).lss

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) -A $<

nm-check: $(BUILD_DIR)/$(TARGET).elf
	$(NM) -n $< | grep -E " (SVC_Handler|PendSV_Handler|SysTick_Handler|TIM1_BRK_TIM15_IRQHandler|vTaskStartScheduler)$$"

flash: all
	STM32_Programmer_CLI -c port=SWD -w $(BUILD_DIR)/$(TARGET).bin 0x08000000 -v -rst

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@echo Cleaned $(BUILD_DIR).

-include $(DEPS)
