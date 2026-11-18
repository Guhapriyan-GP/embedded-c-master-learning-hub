# ----------------------------------------------------------------------------
# Host Unit Test, Coverage (gcov), & Static Analyzer (gcc -fanalyzer) Makefile
# Portable across Windows (MSYS2 GCC / cmd.exe) and Linux (Ubuntu CI)
# Usage:
#   make test       - Build & execute all 4 host unit test suites
#   make coverage   - Build with --coverage, run all suites, report gcov -b
#   make analyze    - Run GCC -fanalyzer across all host-testable drivers & tests
#   make clean      - Remove all build, coverage, and .gcov artifacts
# ----------------------------------------------------------------------------

HOST_CC     ?= gcc
GCOV        ?= gcov
BUILD_DIR   := build
COV_DIR     := $(BUILD_DIR)/cov
DRV_DIR     := ../23-code-library/Drivers

WARN_FLAGS  := -Wall -Wextra -Werror -Wpedantic \
               -Wshadow -Wconversion -Wdouble-promotion -Wundef
CFLAGS      := -std=c11 -O1 -g $(WARN_FLAGS) -I$(DRV_DIR)
COV_CFLAGS  := -std=c11 -O0 -g --coverage -fprofile-arcs -ftest-coverage $(WARN_FLAGS) -I$(DRV_DIR)
ANALYZE_FLG := -std=c11 -O1 -fanalyzer $(WARN_FLAGS) -I$(DRV_DIR) -fsyntax-only

ifeq ($(OS),Windows_NT)
    EXE     := .exe
else
    EXE     :=
endif

ifeq ($(findstring sh,$(notdir $(SHELL))),)
    MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM_RF   = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
    RM_F    = if exist "$(subst /,\,$(1))" del /Q "$(subst /,\,$(1))"
    MV_GCOV = move /Y *.gcov "$(subst /,\,$(1))\" >NUL 2>NUL || exit /b 0
    RUN_BIN = $(subst /,\,$(1))
else
    MKDIR_P = mkdir -p "$(1)"
    RM_RF   = rm -rf "$(1)"
    RM_F    = rm -f $(1)
    MV_GCOV = mv -f *.gcov "$(1)/" 2>/dev/null || true
    RUN_BIN = ./$(1)
endif

# Driver sources compiled on host
DRIVER_MATH_SRCS := $(DRV_DIR)/exti_g4.c \
                    $(DRV_DIR)/tim_g4.c \
                    $(DRV_DIR)/usart_g4.c \
                    $(DRV_DIR)/spi_g4.c \
                    $(DRV_DIR)/i2c_g4.c \
                    $(DRV_DIR)/adc_g4.c \
                    $(DRV_DIR)/dma_g4.c \
                    $(DRV_DIR)/iwdg_g4.c

DRIVER_MATH_NAMES := exti_g4 tim_g4 usart_g4 spi_g4 i2c_g4 adc_g4 dma_g4 iwdg_g4

# Standard test binaries
TEST_FOUNDATIONS := $(BUILD_DIR)/test_c_foundations$(EXE)
TEST_RINGBUF     := $(BUILD_DIR)/test_ring_buffer$(EXE)
TEST_DRIVERS     := $(BUILD_DIR)/test_drivers_math$(EXE)
TEST_GPIO_MOCK   := $(BUILD_DIR)/test_gpio_mock$(EXE)

# Coverage-instrumented binaries & object files
COV_RINGBUF_OBJ  := $(COV_DIR)/ring_buffer.o
COV_GPIO_OBJ     := $(COV_DIR)/gpio_g4.o
COV_MATH_OBJS    := $(patsubst %,$(COV_DIR)/%.o,$(DRIVER_MATH_NAMES))

COV_FOUNDATIONS  := $(COV_DIR)/test_c_foundations$(EXE)
COV_RINGBUF      := $(COV_DIR)/test_ring_buffer$(EXE)
COV_DRIVERS      := $(COV_DIR)/test_drivers_math$(EXE)
COV_GPIO_MOCK    := $(COV_DIR)/test_gpio_mock$(EXE)

.PHONY: all test coverage analyze clean

all: $(TEST_FOUNDATIONS) $(TEST_RINGBUF) $(TEST_DRIVERS) $(TEST_GPIO_MOCK)

$(TEST_FOUNDATIONS): test_c_foundations.c
	@$(call MKDIR_P,$(BUILD_DIR))
	@echo [HOST-CC] $@
	$(HOST_CC) $(CFLAGS) $< -o $@

$(TEST_RINGBUF): unit/test_ring_buffer.c $(DRV_DIR)/ring_buffer.c
	@$(call MKDIR_P,$(BUILD_DIR))
	@echo [HOST-CC] $@
	$(HOST_CC) $(CFLAGS) $^ -o $@

$(TEST_DRIVERS): unit/test_drivers_math.c $(DRIVER_MATH_SRCS)
	@$(call MKDIR_P,$(BUILD_DIR))
	@echo [HOST-CC] $@
	$(HOST_CC) $(CFLAGS) -DDRIVER_HOST_TEST $^ -o $@

$(TEST_GPIO_MOCK): unit/test_gpio_mock.c $(DRV_DIR)/gpio_g4.c mocks/stm32g474xx.h
	@$(call MKDIR_P,$(BUILD_DIR))
	@echo [HOST-CC] $@
	$(HOST_CC) $(CFLAGS) -Imocks unit/test_gpio_mock.c $(DRV_DIR)/gpio_g4.c -o $@

test: all
	@echo "--- [1/4] Executing Stage 01-03 Embedded C Foundations Tests ---"
	$(call RUN_BIN,$(TEST_FOUNDATIONS))
	@echo "--- [2/4] Executing Ring Buffer Driver Unit Tests ---"
	$(call RUN_BIN,$(TEST_RINGBUF))
	@echo "--- [3/4] Executing 8-Driver Pure Helper Math Tests (-DDRIVER_HOST_TEST) ---"
	$(call RUN_BIN,$(TEST_DRIVERS))
	@echo "--- [4/4] Executing GPIO Register Mock & Fault Bounds Tests ---"
	$(call RUN_BIN,$(TEST_GPIO_MOCK))
	@echo "[ALL 4 HOST UNIT TEST SUITES PASSED]"

# ----------------------------------------------------------------------------
# Coverage Target (gcov -b on all 10 host-testable drivers)
# ----------------------------------------------------------------------------
$(COV_RINGBUF_OBJ): $(DRV_DIR)/ring_buffer.c
	@$(call MKDIR_P,$(COV_DIR))
	$(HOST_CC) $(COV_CFLAGS) -c $< -o $@

$(COV_GPIO_OBJ): $(DRV_DIR)/gpio_g4.c mocks/stm32g474xx.h
	@$(call MKDIR_P,$(COV_DIR))
	$(HOST_CC) $(COV_CFLAGS) -Imocks -c $(DRV_DIR)/gpio_g4.c -o $@

$(COV_DIR)/%.o: $(DRV_DIR)/%.c
	@$(call MKDIR_P,$(COV_DIR))
	$(HOST_CC) $(COV_CFLAGS) -DDRIVER_HOST_TEST -c $< -o $@

coverage: $(COV_RINGBUF_OBJ) $(COV_GPIO_OBJ) $(COV_MATH_OBJS)
	@$(call MKDIR_P,$(COV_DIR))
	@echo [GCOV-CC] Building coverage-instrumented test suites...
	$(HOST_CC) $(COV_CFLAGS) test_c_foundations.c -o $(COV_FOUNDATIONS)
	$(HOST_CC) $(COV_CFLAGS) unit/test_ring_buffer.c $(COV_RINGBUF_OBJ) -o $(COV_RINGBUF)
	$(HOST_CC) $(COV_CFLAGS) -DDRIVER_HOST_TEST unit/test_drivers_math.c $(COV_MATH_OBJS) -o $(COV_DRIVERS)
	$(HOST_CC) $(COV_CFLAGS) -Imocks unit/test_gpio_mock.c $(COV_GPIO_OBJ) -o $(COV_GPIO_MOCK)
	@echo "--- Running Instrumented Unit Test Suites ---"
	$(call RUN_BIN,$(COV_FOUNDATIONS))
	$(call RUN_BIN,$(COV_RINGBUF))
	$(call RUN_BIN,$(COV_DRIVERS))
	$(call RUN_BIN,$(COV_GPIO_MOCK))
	@echo "--- Generating Real gcov Line & Branch Coverage Report ---"
	$(GCOV) -b -o $(COV_DIR) $(DRV_DIR)/ring_buffer.c $(DRV_DIR)/gpio_g4.c $(DRIVER_MATH_SRCS)
	@$(call MV_GCOV,$(COV_DIR))

# ----------------------------------------------------------------------------
# Static Analysis Target (GCC -fanalyzer across all host-testable drivers & tests)
# ----------------------------------------------------------------------------
analyze:
	@echo "--- Running GCC -fanalyzer Static Analysis ---"
	$(HOST_CC) $(ANALYZE_FLG) test_c_foundations.c
	$(HOST_CC) $(ANALYZE_FLG) unit/test_ring_buffer.c $(DRV_DIR)/ring_buffer.c
	$(HOST_CC) $(ANALYZE_FLG) -DDRIVER_HOST_TEST unit/test_drivers_math.c $(DRIVER_MATH_SRCS)
	$(HOST_CC) $(ANALYZE_FLG) -Imocks unit/test_gpio_mock.c $(DRV_DIR)/gpio_g4.c
	@echo "[GCC -FANALYZER STATIC ANALYSIS PASSED: 0 WARNINGS]"

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@$(call RM_F,*.gcda)
	@$(call RM_F,*.gcno)
	@$(call RM_F,*.gcov)
	@echo Cleaned test build and coverage artifacts.
