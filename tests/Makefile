# ----------------------------------------------------------------------------
# Host Unit Test Runner Makefile (GCC / Clang, Windows & Linux)
# Usage: make test
# ----------------------------------------------------------------------------

HOST_CC     ?= gcc
BUILD_DIR   := build
CFLAGS      := -std=c11 -O1 -g -Wall -Wextra -Werror -Wpedantic \
               -Wshadow -Wconversion -Wdouble-promotion -Wundef \
               -I../23-code-library/Drivers

ifeq ($(OS),Windows_NT)
    EXE     := .exe
else
    EXE     :=
endif

ifeq ($(findstring sh,$(notdir $(SHELL))),)
    MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM_RF   = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
    RUN_BIN = $(subst /,\,$(1))
else
    MKDIR_P = mkdir -p "$(1)"
    RM_RF   = rm -rf "$(1)"
    RUN_BIN = ./$(1)
endif

TEST_FOUNDATIONS := $(BUILD_DIR)/test_c_foundations$(EXE)
TEST_RINGBUF     := $(BUILD_DIR)/test_ring_buffer$(EXE)

.PHONY: all test clean

all: $(TEST_FOUNDATIONS) $(TEST_RINGBUF)

$(TEST_FOUNDATIONS): test_c_foundations.c
	@$(call MKDIR_P,$(BUILD_DIR))
	@echo [HOST-CC] $@
	$(HOST_CC) $(CFLAGS) $< -o $@

$(TEST_RINGBUF): unit/test_ring_buffer.c ../23-code-library/Drivers/ring_buffer.c
	@$(call MKDIR_P,$(BUILD_DIR))
	@echo [HOST-CC] $@
	$(HOST_CC) $(CFLAGS) $^ -o $@

test: all
	@echo "--- Executing Stage 01 C Foundations Tests ---"
	$(call RUN_BIN,$(TEST_FOUNDATIONS))
	@echo "--- Executing Ring Buffer Driver Unit Tests ---"
	$(call RUN_BIN,$(TEST_RINGBUF))
	@echo "[ALL HOST UNIT TESTS PASSED]"

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@echo Cleaned test build directory.
