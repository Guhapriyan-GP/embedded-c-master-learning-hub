cmake_minimum_required(VERSION 3.21)

project(stm32g474re_firmware C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TARGET_NAME firmware)
set(LDSCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/stm32g474re.ld")

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(WARN_FLAGS
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wdouble-promotion
    -Wundef
)

add_executable(${TARGET_NAME}.elf
    Src/main.c
    Src/startup_stm32g474re.c
    Src/system_stm32g4xx.c
    ../Drivers/clock_g4.c
    ../Drivers/gpio_g4.c
    ../Drivers/ring_buffer.c
    ../Drivers/fault_handler.c
)

target_compile_definitions(${TARGET_NAME}.elf PRIVATE
    STM32G474xx
)

target_include_directories(${TARGET_NAME}.elf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers"
)

target_include_directories(${TARGET_NAME}.elf SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32G4xx/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Core/Include"
)

target_compile_options(${TARGET_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    ${WARN_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fstack-usage
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-O2 -g -DNDEBUG>
)

target_link_options(${TARGET_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${LDSCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map,--cref
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

set_target_properties(${TARGET_NAME}.elf PROPERTIES
    LINK_DEPENDS "${LDSCRIPT}"
)

add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}.elf> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}.elf> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}.elf>
    COMMENT "Generating Intel HEX, raw BIN, and ELF size report"
)
