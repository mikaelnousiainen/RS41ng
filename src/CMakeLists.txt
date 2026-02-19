# Collect all sources via GLOB_RECURSE (re-run cmake after adding new files)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.c")
file(GLOB_RECURSE SOURCES_ASM CONFIGURE_DEPENDS "*.s")
file(GLOB_RECURSE SOURCES_CXX CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "*.h")

# --- Per-target configuration ---
if(STM32F1)
    message(STATUS "Target: STM32F1 (RS41 old / DFM-17)")
    set(MCU_CPU cortex-m3)
    set(MCU_DEFINE STM32F100xB)
    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F100XB_FLASH.ld)
    set(HAL_DIR hal_stm32f1xx)
    set(EXCLUDE_HAL_DIR hal_stm32l4xx)
else()
    message(STATUS "Target: STM32L4 (RS41 RSM4x4 series)")
    set(MCU_CPU cortex-m4)
    set(MCU_DEFINE STM32L412xx)
    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32L412RBTX_FLASH.ld)
    set(HAL_DIR hal_stm32l4xx)
    set(EXCLUDE_HAL_DIR hal_stm32f1xx)
endif()

# Filter out the unused HAL
list(FILTER SOURCES EXCLUDE REGEX "${EXCLUDE_HAL_DIR}/.*")
list(FILTER SOURCES_ASM EXCLUDE REGEX "${EXCLUDE_HAL_DIR}/.*")
list(FILTER SOURCES_CXX EXCLUDE REGEX "${EXCLUDE_HAL_DIR}/.*")
list(FILTER HEADERS EXCLUDE REGEX "${EXCLUDE_HAL_DIR}/.*")

# --- Executable ---
add_executable(${PROJECT_NAME}.elf
    ${SOURCES}
    ${SOURCES_ASM}
    ${SOURCES_CXX}
    ${HEADERS}
    ${LINKER_SCRIPT}
)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    C_STANDARD 99
    C_EXTENSIONS ON
    CXX_STANDARD 11
    CXX_EXTENSIONS OFF
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    ${MCU_DEFINE}
    SUPPORT_CPLUSPLUS
    __ASSEMBLY__
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=${MCU_CPU}
    -mthumb
    -Wall
    -ffunction-sections
    -fdata-sections
    # -g
    -Os
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${HAL_DIR}/cmsis
    ${HAL_DIR}/cmsis_boot/startup
    ${HAL_DIR}/cmsis_boot
    ${HAL_DIR}/stm_lib/inc
    drivers
    codecs
    syscalls
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=${MCU_CPU}
    -mthumb
    # -g
    -Os
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--gc-sections
    --specs=nano.specs
    --specs=nosys.specs
    -z muldefs
    -T${LINKER_SCRIPT}
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE stdc++)

# --- Post-build: generate HEX and BIN, print size ---
set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Building ${HEX_FILE}\nBuilding ${BIN_FILE}"
)